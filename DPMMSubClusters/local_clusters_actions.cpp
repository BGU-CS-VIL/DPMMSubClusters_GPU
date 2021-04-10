#include <map>
#include <random>
#include <numeric>
#include "local_clusters_actions.h"
#include "utils.h"
#include "global_params.h"
#include "shared_actions.h"
#include "distributions_util/dirichlet.h"
#include <iostream>
#include "distributions_util/pdflib.hpp"
#include "draw.h"
#include "clusterInfo.h"
#include <ppl.h>

template<typename Func>
struct lambda_as_visitor_wrapper : Func {
	lambda_as_visitor_wrapper(const Func& f) : Func(f) {}
	template<typename S, typename I>
	void init(const S& v, I i, I j) { return Func::operator()(v, i, j); }
};

template<typename Mat, typename Func>
void visit_lambda(const Mat& m, const Func& f)
{
	lambda_as_visitor_wrapper<Func> visitor(f);
	m.visit(visitor);
}

local_cluster* local_clusters_actions::create_first_local_cluster(local_group &group, prior **pPrior)
{
	local_cluster *cluster = new local_cluster();
	cluster->cluster_params = new splittable_cluster_params();
	cluster->cluster_params->cluster_params = new cluster_parameters();

	*pPrior = utils::create_sufficient_statistics(group.model_hyperparams.distribution_hyper_params, &(cluster->cluster_params->cluster_params->suff_statistics), globalParams);
	hyperparams *post = group.model_hyperparams.distribution_hyper_params;
	distribution_sample* dist = (*pPrior)->sample_distribution(post, globalParams->gen);
	cluster->cluster_params->cluster_params->prior_hyperparams = group.model_hyperparams.distribution_hyper_params->clone();
	cluster->cluster_params->cluster_params->distribution = dist;
	cluster->cluster_params->cluster_params->posterior_hyperparams = post->clone();
	cluster->cluster_params->cluster_params->suff_statistics->N = group.points.cols();
	cluster->cluster_params->cluster_params_l = cluster->cluster_params->cluster_params->clone();
	cluster->cluster_params->cluster_params_r = cluster->cluster_params->cluster_params->clone();
	cluster->cluster_params->lr_weights.push_back(0.5);
	cluster->cluster_params->lr_weights.push_back(0.5);
	cluster->cluster_params->splittable = false;
	cluster->cluster_params->logsublikelihood_hist = Logsublikelihood_hist(globalParams->burnout_period + 5, std::make_pair(false, 0));
	cluster->cluster_params->cluster_params_l->suff_statistics->N = 0;
	cluster->cluster_params->cluster_params_r->suff_statistics->N = 0;
	globalParams->cuda->get_sub_labels_count(
		cluster->cluster_params->cluster_params_l->suff_statistics->N,
		cluster->cluster_params->cluster_params_r->suff_statistics->N);

	cluster->total_dim = group.model_hyperparams.total_dim;
	cluster->points_count = cluster->cluster_params->cluster_params->suff_statistics->N;
	cluster->l_count = cluster->cluster_params->cluster_params_l->suff_statistics->N;
	cluster->r_count = cluster->cluster_params->cluster_params_r->suff_statistics->N;

	////call for each GPU
	split_first_cluster_worker(group);
	return cluster;
}


local_cluster* local_clusters_actions::create_outlier_local_cluster(local_group &group, hyperparams* outlier_params, prior *prior)
{
	local_cluster *cluster = new local_cluster();
	cluster->cluster_params = new splittable_cluster_params();
	cluster->cluster_params->cluster_params = new cluster_parameters();

	sufficient_statistics *suff = prior->create_sufficient_statistics(outlier_params, outlier_params, group.points);
	hyperparams *post = prior->calc_posterior(outlier_params, suff);
	distribution_sample *dist = prior->sample_distribution(post, globalParams->gen);
	cluster->cluster_params->cluster_params->prior_hyperparams = outlier_params;
	cluster->cluster_params->cluster_params->distribution = dist;
	cluster->cluster_params->cluster_params->suff_statistics = suff;
	cluster->cluster_params->cluster_params->posterior_hyperparams = outlier_params;
	cluster->cluster_params->cluster_params->posterior_hyperparams = post;
	cluster->cluster_params->cluster_params_l = cluster->cluster_params->cluster_params->clone();
	cluster->cluster_params->cluster_params_r = cluster->cluster_params->cluster_params->clone();
	cluster->cluster_params->lr_weights.push_back(0.5);
	cluster->cluster_params->lr_weights.push_back(0.5);
	cluster->cluster_params->splittable = false;
	cluster->cluster_params->logsublikelihood_hist = Logsublikelihood_hist(globalParams->burnout_period + 5, std::make_pair(false, 0));
	cluster->cluster_params->cluster_params->suff_statistics->N = group.points.cols();

	globalParams->cuda->get_sub_labels_count(
		cluster->cluster_params->cluster_params_l->suff_statistics->N,
		cluster->cluster_params->cluster_params_r->suff_statistics->N);
	
	cluster->total_dim = group.model_hyperparams.total_dim;
	cluster->points_count = cluster->cluster_params->cluster_params->suff_statistics->N;
	cluster->l_count = cluster->cluster_params->cluster_params_l->suff_statistics->N;
	cluster->r_count = cluster->cluster_params->cluster_params_r->suff_statistics->N;
	return cluster;
}


void local_clusters_actions::sample_sub_clusters(prior* pPrior, local_group &group)
{
//TODO-	asynch;
	//for each GPU
	globalParams->cuda->create_subclusters_labels(globalParams->clusters_vector.size(), globalParams->clusters_vector, group.points.rows());
}
				
void local_clusters_actions::sample_labels(prior* pPrior, local_group &group, bool bFinal, bool no_more_splits)
{
	globalParams->cuda->create_clusters_labels(globalParams->clusters_vector.size(), globalParams->clusters_vector, globalParams->clusters_weights, bFinal);
}


void local_clusters_actions::update_splittable_cluster_params(prior *pPrior, splittable_cluster_params* &splittable_cluser)
{
	splittable_cluser->cluster_params->posterior_hyperparams = pPrior->calc_posterior(splittable_cluser->cluster_params->prior_hyperparams, splittable_cluser->cluster_params->suff_statistics);
	splittable_cluser->cluster_params_l->posterior_hyperparams = pPrior->calc_posterior(splittable_cluser->cluster_params_l->prior_hyperparams, splittable_cluser->cluster_params_l->suff_statistics);
	splittable_cluser->cluster_params_r->posterior_hyperparams = pPrior->calc_posterior(splittable_cluser->cluster_params_r->prior_hyperparams, splittable_cluser->cluster_params_r->suff_statistics);
}

std::map<LabelType, thin_suff_stats*> local_clusters_actions::create_suff_stats_dict_worker(MatrixXd &group_pts, hyperparams* hyper_params, LabelsType &indices)
{
	std::map<LabelType, thin_suff_stats*> suff_stats_dict;
	
	for (LabelType index = 0; index < indices.size(); index++)
	{
		LabelType indicesLabelsSize = 0;

		MatrixXd *pts;
		MatrixXd *pts1;
		MatrixXd *pts2;

		globalParams->cuda->create_suff_stats_dict_worker(indices[index] + 1,
			indicesLabelsSize,
			group_pts,
			pts,
			pts1,
			pts2);
		
		thin_suff_stats *tss = new thin_suff_stats();
		prior* cpl_suff = utils::create_sufficient_statistics(hyper_params, &(tss->l_suff), globalParams, *pts1);
		delete cpl_suff;
		prior* cpr_suff = utils::create_sufficient_statistics(hyper_params, &(tss->r_suff), globalParams, *pts2);
		delete cpr_suff;
		prior* cp_suff = utils::create_sufficient_statistics(hyper_params, &(tss->cluster_suff), globalParams, *pts);
		delete cp_suff;

		suff_stats_dict[index] = tss;

		delete pts;
		delete pts1;
		delete pts2;
	}
	
	return suff_stats_dict;
}

//std::map<LabelType, thin_suff_stats*> local_clusters_actions::create_suff_stats_dict_worker(MatrixXd& group_pts, hyperparams* hyper_params, LabelsType& indices)
//{
//	std::map<LabelType, thin_suff_stats*> suff_stats_dict;
//
//	LabelsType group_labels;
//	LabelsType group_sublabels;
//	globalParams->cuda->get_labels(group_labels);
//	globalParams->cuda->get_sub_labels(group_sublabels);
//
//	for (long long index = 0; index < indices.size(); index++)
//	{
//		std::vector<long long> indicesLabels;
//		long long th = indices[index] + 1;
//
//		for (size_t i = 0; i < group_labels.size(); i++)
//		{
//			if (group_labels[i] == th)
//			{
//				indicesLabels.push_back(i);
//			}
//		}
//				
//		MatrixXd pts(group_pts.rows(), indicesLabels.size());
//		MatrixXd pts1(group_pts.rows(), indicesLabels.size());
//		MatrixXd pts2(group_pts.rows(), indicesLabels.size());
//		long long j1 = 0;
//		long long j2 = 0;
//
//
//		for (long long j = 0; j < indicesLabels.size(); j++)
//		{
//			if (group_sublabels[indicesLabels[j]] == 1)
//			{
//				for (long long i = 0; i < group_pts.rows(); i++)
//				{
//					double pt = group_pts(i, indicesLabels[j]);
//					pts(i, j) = pt;
//					pts1(i, j1) = pt;
//				}
//				++j1;
//			}
//			else if (group_sublabels[indicesLabels[j]] == 2)
//			{
//				for (long long i = 0; i < group_pts.rows(); i++)
//				{
//					double pt = group_pts(i, indicesLabels[j]);
//					pts(i, j) = pt;
//					pts2(i, j2) = pt;
//				}
//				++j2;
//			}
//		}
//		pts1.conservativeResize(group_pts.rows(), j1);
//		pts2.conservativeResize(group_pts.rows(), j2);
//
//		//TODO - check if pts needed to be updated after pts1 and pts2 been updated
//		thin_suff_stats* tss = new thin_suff_stats();
//		prior* cpl_suff = utils::create_sufficient_statistics(hyper_params, &(tss->l_suff), globalParams, pts1);
//		prior* cpr_suff = utils::create_sufficient_statistics(hyper_params, &(tss->r_suff), globalParams, pts2);
//		prior* cp_suff = utils::create_sufficient_statistics(hyper_params, &(tss->cluster_suff), globalParams, pts);
//		suff_stats_dict[index] = tss;
//		//	if (tss->cluster_suff->N == 0)
//		{
//			{
//				//		int raz7 = 7;
//	//			std::cout << "1.01*****************  " << th << std::endl;
//	//			utils::saveToFile(group_labels, "create_suff_stats_dict_worker_group_labels");
//			}
//		}
//
//	}
//
//	return suff_stats_dict;
//}

/*
												function create_suff_stats_dict_node_leader(group_pts, group_labels, group_sublabels, hyper_params, proc_ids, indices)
												leader_suff_dict = Dict()
												workers_suff_dict = Dict()
												if indices == nothing
													indices = collect(1:length(clusters_vector))
													end
													for i in proc_ids
														workers_suff_dict[i] = remotecall(create_suff_stats_dict_worker, i, group_pts,
															group_labels,
															group_sublabels,
															hyper_params,
															indices)
														end
														suff_stats_vectors = [[] for i = 1:length(indices)]
														cluster_to_index = Dict([indices[i] = > i for i = 1:length(indices)])
														workers_suff_dict_fetched = Dict([k = > fetch(v) for (k, v) in workers_suff_dict])
														for (k, v) in workers_suff_dict_fetched
															for (cluster, suff) in v
																push!(suff_stats_vectors[cluster_to_index[cluster]], suff)
																end
																end
																for (k, v) in enumerate(suff_stats_vectors)
																	if length(v) > 0
																		cp_suff = reduce(aggregate_suff_stats, [x.cluster_suff for x in v])
																		cpl_suff = reduce(aggregate_suff_stats, [x.l_suff for x in v])
																		cpr_suff = reduce(aggregate_suff_stats, [x.r_suff for x in v])
																		cluster_index = indices[k]
																		leader_suff_dict[cluster_index] = thin_suff_stats(cp_suff, cpl_suff, cpr_suff)
																		end
																		end

																		return leader_suff_dict
																		end

																		*/
void local_clusters_actions::update_suff_stats_posterior(prior *pPrior, local_group &group)
{
	LabelsType indices;
	update_suff_stats_posterior(pPrior, group, indices, false);
}

void local_clusters_actions::update_suff_stats_posterior(prior *pPrior, local_group &group, LabelsType &indices, bool indicesValid)
{
	std::map<int, std::map<LabelType, thin_suff_stats*>> workers_suff_dict;
	if (!indicesValid)
	{
		indices.resize(group.local_clusters.size());
		std::iota(std::begin(indices), std::end(indices), 0); // Fill with 0, 1, 2...
//		indices.fill(0); // Fill with 0, 1, 2...

	}

	//for each GPU
		//	for i in(nworkers() == 0 ? procs() : workers())
	{
		//niw_hyperparams* pNiw_hyperparams = (niw_hyperparams*)group.model_hyperparams.distribution_hyper_params;
		//std::cout << "BUG?? k:" << pNiw_hyperparams->k << " v:" << pNiw_hyperparams->v << std::endl;
		//pNiw_hyperparams = (niw_hyperparams*)group.local_clusters[0]->cluster_params->cluster_params->prior_hyperparams;
		//std::cout << "BUG2?? k:" << pNiw_hyperparams->k << " v:" << pNiw_hyperparams->v << std::endl;
		//pNiw_hyperparams = (niw_hyperparams*)group.local_clusters[0]->cluster_params->cluster_params->posterior_hyperparams;
		//std::cout << "BUG3?? k:" << pNiw_hyperparams->k << " v:" << pNiw_hyperparams->v << std::endl;

		workers_suff_dict[0] = create_suff_stats_dict_worker(group.points, 
			group.model_hyperparams.distribution_hyper_params,
			//Raz - todo if we want to use group. we need to copy back what? to it after what?
			//group.local_clusters[0]->cluster_params->cluster_params->hyperparams->hyper_params,
			indices);
	}
	
	std::vector<std::vector<thin_suff_stats*>>	suff_stats_vectors(indices.size());
	std::map<ClusterIndexType, ClusterIndexType> cluster_to_index;
	for (LabelType i = 0; i < indices.size(); i++)
	{
		cluster_to_index[indices[i]] = i;

	}

	//TODO-	synch;

	for (std::map<int, std::map<LabelType, thin_suff_stats*>>::const_iterator iter = workers_suff_dict.begin();
		iter != workers_suff_dict.end();
		++iter)
	{
		for (std::map<LabelType, thin_suff_stats*>::const_iterator iter2 = iter->second.begin();
			iter2 != iter->second.end();
			++iter2)
		{
			suff_stats_vectors[cluster_to_index[iter2->first]].push_back(iter2->second);
			//if (iter2->second->cluster_suff->N == 0)
			//{
			//	std::cout << "1.03*****************  " << cluster_to_index[iter2->first] << std::endl;
			//}
			
		}
	}

	for (LabelType index = 0; index < indices.size(); index++)
	{
		if (suff_stats_vectors[index].size() == 0)
		{
			continue;
		}
		local_cluster *pCluster = group.local_clusters[indices[index]];
		//if (pCluster->points_count == 0)
		//{
		//	std::cout << "1.0*****************  " << pCluster->points_count << std::endl;
		//	std::cout << "index:" << index << " indices[index]:" << indices[index] << std::endl;

		//}
		pCluster->cluster_params->cluster_params->suff_statistics = suff_stats_vectors[index][0]->cluster_suff;

		pCluster->cluster_params->cluster_params_l->suff_statistics = suff_stats_vectors[index][0]->l_suff;
		pCluster->cluster_params->cluster_params_r->suff_statistics = suff_stats_vectors[index][0]->r_suff;

		for (LabelType i = 1; i < suff_stats_vectors[index].size(); i++)
		{
			pPrior->aggregate_suff_stats(pCluster->cluster_params->cluster_params->suff_statistics, suff_stats_vectors[index][i]->cluster_suff, pCluster->cluster_params->cluster_params->suff_statistics);
			pPrior->aggregate_suff_stats(pCluster->cluster_params->cluster_params_l->suff_statistics, suff_stats_vectors[index][i]->l_suff, pCluster->cluster_params->cluster_params_l->suff_statistics);
			pPrior->aggregate_suff_stats(pCluster->cluster_params->cluster_params_r->suff_statistics, suff_stats_vectors[index][i]->r_suff, pCluster->cluster_params->cluster_params_r->suff_statistics);
		}
		pCluster->points_count = pCluster->cluster_params->cluster_params->suff_statistics->N;
		//if (pCluster->points_count == 0)
		//{
		//	std::cout << "1*****************  " << pCluster->points_count << std::endl;
		//	std::cout << "index:" << index << " indices[index]:" << indices[index] << " " << suff_stats_vectors[index].size() << std::endl;

		//}
		update_splittable_cluster_params(pPrior, pCluster->cluster_params);
	}
}
																									
void local_clusters_actions::split_first_cluster_worker(local_group &group)
{
	globalParams->cuda->sample_sub_labels();
}

void local_clusters_actions::split_cluster_local_worker(LabelsType &indices, LabelsType &new_indices)
{
	for (LabelType i = 0; i < indices.size(); i++)
	{
		globalParams->cuda->split_cluster_local_worker(indices[i], new_indices[i]);
	}
}

void local_clusters_actions::split_cluster_local(prior *pPrior, local_group &group, local_cluster* &cluster, ClusterIndexType index, ClusterIndexType new_index)
{
	shared_actions sa(globalParams);
	local_cluster *l_split = cluster->clone();
	sa.create_splittable_from_params(pPrior, cluster->cluster_params->cluster_params_r, group.model_hyperparams.alpha, l_split->cluster_params);
	sa.create_splittable_from_params(pPrior, cluster->cluster_params->cluster_params_l, group.model_hyperparams.alpha, cluster->cluster_params);

	l_split->points_count = l_split->cluster_params->cluster_params->suff_statistics->N;
	cluster->points_count = cluster->cluster_params->cluster_params->suff_statistics->N;

	group.local_clusters[new_index] = l_split;
}
																										
void local_clusters_actions::merge_clusters_worker(LabelsType &indices, LabelsType &new_indices)
{
	for (LabelType i = 0; i < indices.size(); i++)
	{
		globalParams->cuda->merge_clusters_worker(indices[i], new_indices[i]);
	}
}
																											
void local_clusters_actions::merge_clusters(prior *pPrior, local_group &group, ClusterIndexType index_l, ClusterIndexType index_r)
{
	shared_actions sa(globalParams);

//	globalParams->clusterInfos.merge(index_l, index_r, group.local_clusters[index_l]->points_count, group.local_clusters[index_r]->points_count);

	sa.merge_clusters_to_splittable(pPrior, group.local_clusters[index_l]->cluster_params, group.local_clusters[index_r]->cluster_params->cluster_params, group.model_hyperparams.alpha);
	group.local_clusters[index_l]->points_count += group.local_clusters[index_r]->points_count;
	group.local_clusters[index_r]->points_count = 0;
	group.local_clusters[index_r]->cluster_params->cluster_params->suff_statistics->N = 0;
	group.local_clusters[index_r]->cluster_params->splittable = false;
}

																											
void local_clusters_actions::should_split_local(prior *pPrior, double &should_split, splittable_cluster_params* &cluster_params, double alpha, bool bFinal)
{
	if (bFinal || cluster_params->cluster_params_l->suff_statistics->N == 0 || cluster_params->cluster_params_r->suff_statistics->N == 0)
	{
		should_split = 0;
		return;
	}
	hyperparams *post = pPrior->calc_posterior(cluster_params->cluster_params->prior_hyperparams, cluster_params->cluster_params->suff_statistics);
	hyperparams *lpost = pPrior->calc_posterior(cluster_params->cluster_params->prior_hyperparams, cluster_params->cluster_params_l->suff_statistics);
	hyperparams *rpost = pPrior->calc_posterior(cluster_params->cluster_params->prior_hyperparams, cluster_params->cluster_params_r->suff_statistics);


	double log_likihood_l = pPrior->log_marginal_likelihood(cluster_params->cluster_params_l->prior_hyperparams, lpost, cluster_params->cluster_params_l->suff_statistics);
	double log_likihood_r = pPrior->log_marginal_likelihood(cluster_params->cluster_params_r->prior_hyperparams, rpost, cluster_params->cluster_params_r->suff_statistics);
	double log_likihood = pPrior->log_marginal_likelihood(cluster_params->cluster_params->prior_hyperparams, post, cluster_params->cluster_params->suff_statistics);





	{
		/*niw_hyperparams* prior = (niw_hyperparams*)cluster_params->cluster_params->prior_hyperparams;
		niw_hyperparams* poster = (niw_hyperparams*)post;
		cluster_params->cluster_params->suff_statistics->N = 100000.0;
		cluster_params->cluster_params->suff_statistics->points_sum << 550604.2, 391225.7 ;
			
		prior->k = 1.0;
		prior->m << 0.0, 0.0;
		prior->v=5.0;
		prior->psi << 1.0, 0.0, 0.0, 1.0;

		poster->k = 100001.0;
		poster->m << 5.5059867, 3.9122179;
		poster->v = 100005.0f;
		poster->psi << 45.465008, - 33.081253, -33.081253, 149.29265;

		double log_likihood2 = pPrior->log_marginal_likelihood(prior, post, cluster_params->cluster_params->suff_statistics);
		std::cout << "log_likihood2:" << log_likihood2 << std::endl;*/
		//-716187.5613786621
	}








	//TODO - should be logabsgamma <= abs
	double log_HR = log(alpha) +
		r8_gamma_log(cluster_params->cluster_params_l->suff_statistics->N) + log_likihood_l +
		r8_gamma_log(cluster_params->cluster_params_r->suff_statistics->N) + log_likihood_r -
		(r8_gamma_log(cluster_params->cluster_params->suff_statistics->N) + log_likihood);

	std::uniform_real_distribution<double> dist(0, 1);
	double a_random_double = dist(*globalParams->gen);
	//niw_sufficient_statistics* pSuffL = (niw_sufficient_statistics*)(cluster_params->cluster_params_l->suff_statistics);
	//niw_sufficient_statistics* pSuffR = (niw_sufficient_statistics*)(cluster_params->cluster_params_r->suff_statistics);
	//niw_sufficient_statistics* pSuffB = (niw_sufficient_statistics*)(cluster_params->cluster_params->suff_statistics);
	//std::cout << "a_random_double:" << (a_random_double) << " log_HR:" << log_HR << ">? log:" << log(a_random_double) << std::endl;
	//std::cout << "N=> L:" << pSuffL->N << " R:" << pSuffR->N << " B:" << pSuffB->N << std::endl;
	//std::cout << "log_likihood_l:" << log_likihood_l << " log_likihood_r:" << log_likihood_r << " log_likihood:" << log_likihood << " alpha:" << alpha << std::endl;
	//std::cout << "points_sum=>" << std::endl;
	//std::cout << "L:\n" << pSuffL->points_sum << std::endl;
	//std::cout << "R:\n" << pSuffR->points_sum << std::endl;
	//std::cout << "B:\n" << pSuffB->points_sum << std::endl;
	//std::cout << "S=>" << std::endl;
	//std::cout << "L:\n" << pSuffL->S << std::endl;
	//std::cout << "R:\n" << pSuffR->S << std::endl;
	//std::cout << "B:\n" << pSuffB->S << std::endl;
	//	std::cout << "v=> L:" << pSuffL->v << " R:" << pSuffR->v << " B:" << pSuffB->v << std::endl;

	if (log_HR > log(a_random_double))
	{
		should_split = 1;
		std::cout << "************** should_split ***************" << std::endl;
	}
}
																													
void local_clusters_actions::check_and_split(prior *pPrior, local_group &group, bool bFinal, LabelsType &all_indices)
{
	std::vector<double> split_arr = std::vector<double>(group.local_clusters.size(), 0);

	for (ClusterIndexType index = 0; index < group.local_clusters.size(); index++)
	{
		if (globalParams->outlier_mod > 0 && index == 1)
		{
			continue;
		}

		if (group.local_clusters[index]->cluster_params->splittable == true &&
			group.local_clusters[index]->cluster_params->cluster_params->suff_statistics->N > 1)
		{
			should_split_local(pPrior, split_arr[index], group.local_clusters[index]->cluster_params, group.model_hyperparams.alpha, bFinal);
		}
	}
	ClusterIndexType new_index = group.local_clusters.size();
	LabelsType indices;
	LabelsType new_indices;

	group.local_clusters.resize(group.local_clusters.size() + std::accumulate(split_arr.begin(), split_arr.end(), 0));

	for (ClusterIndexType i = 0; i < split_arr.size(); i++)
	{
		if (split_arr[i] == 1)
		{
			indices.push_back(i);
			new_indices.push_back(new_index);
			split_cluster_local(pPrior, group, group.local_clusters[i], i, new_index);
			++new_index;
		}
	}

	all_indices = indices;
	all_indices.insert(all_indices.end(), new_indices.begin(), new_indices.end());
//	all_indices += new_indices;

	for (size_t i = 0; i < indices.size(); i++)
	{
//		globalParams->clusterInfos.split(indices[i], new_indices[i], group.local_clusters[i]->points_count);
	}

	if (indices.size() > 0)
	{
		//TODO-	asynch;
				//for each GPU
		{
			split_cluster_local_worker(indices, new_indices);
		}
	}
}

void local_clusters_actions::check_and_merge(prior *pPrior, local_group &group, bool bFinal)
{
	bool mergable = false;
	LabelsType indices;
	LabelsType new_indices;
	shared_actions sa(globalParams);
	for (ClusterIndexType i = 0; i < group.local_clusters.size(); i++)
	{
		if (globalParams->outlier_mod > 0 && i == 0)
		{
			continue;
		}

		for (ClusterIndexType j = i+1; j < group.local_clusters.size(); j++)
		{
			if (group.local_clusters[i]->cluster_params->splittable == true &&
				group.local_clusters[j]->cluster_params->splittable == true &&
				group.local_clusters[i]->cluster_params->cluster_params->suff_statistics->N > 0 &&
				group.local_clusters[j]->cluster_params->cluster_params->suff_statistics->N > 0)
			{
				sa.should_merge(pPrior, mergable, group.local_clusters[i]->cluster_params->cluster_params,
					group.local_clusters[j]->cluster_params->cluster_params, group.model_hyperparams.alpha, bFinal);
			}
			if (mergable == true)
			{
				merge_clusters(pPrior, group, i, j);
				indices.push_back(i);
				new_indices.push_back(j);
			}
			mergable = false;
		}
	}

	//TODO-	asynch;
		//For each GPU
	{
		merge_clusters_worker(indices, new_indices);
	}
}

std::vector<double> local_clusters_actions::get_dirichlet_distribution(std::vector<double> &points_count)
{
	dirichlet_distribution<std::mt19937> d(points_count);
	return d(*globalParams->gen);
}

void local_clusters_actions::sample_clusters(local_group &group, bool first, prior *pPrior)
{
	shared_actions sa(globalParams);
	std::vector<double> points_count;
	//local_workers = procs(1)[2:end]
	for (ClusterIndexType i = 0; i < group.local_clusters.size(); i++)
	{
		if (globalParams->outlier_mod > 0 && i == 1)
		{
			continue;
		}
		sa.sample_cluster_params(group.local_clusters[i]->cluster_params, group.model_hyperparams.alpha, first, pPrior);
		group.local_clusters[i]->points_count = group.local_clusters[i]->cluster_params->cluster_params->suff_statistics->N;
		points_count.push_back(group.local_clusters[i]->points_count);
	}
	points_count.push_back(group.model_hyperparams.alpha);

	std::vector<double> dirichlet = get_dirichlet_distribution(points_count);
	group.weights.clear();
	for (ClusterIndexType i = 0; i < dirichlet.size() - 1; ++i)
	{
		group.weights.push_back(dirichlet[i] * (1 - globalParams->outlier_mod));
	}

	if (globalParams->outlier_mod > 0)
	{
		group.weights.insert(group.weights.begin(), globalParams->outlier_mod);
	}
}

						
void local_clusters_actions::create_thin_cluster_params(std::vector<local_cluster*> &clusters, std::vector<thin_cluster_params*> &tcp)
{
	for (ClusterIndexType i = 0; i < clusters.size(); i++)
	{
		tcp.push_back(new thin_cluster_params(clusters[i]->cluster_params->cluster_params->distribution->clone(),
			clusters[i]->cluster_params->cluster_params_l->distribution->clone(),
			clusters[i]->cluster_params->cluster_params_r->distribution->clone(),
			clusters[i]->cluster_params->lr_weights));
	}
}

void local_clusters_actions::remove_empty_clusters_worker(std::vector<PointType> &pts_count)
{
	PointType removed = 0;

	for (PointType index = 0; index < pts_count.size(); index++)
	{
		if (pts_count[index] == 0)
		{
			globalParams->cuda->remove_empty_clusters_worker(index - removed + 1);
			++removed;
		}
	}
}

void local_clusters_actions::remove_empty_clusters(local_group &group)
{
	std::vector<local_cluster*> new_vec;
	std::vector<PointType> pts_count;

	for (ClusterIndexType index = 0; index < group.local_clusters.size(); index++)
	{
		pts_count.push_back(group.local_clusters[index]->points_count);
		if (group.local_clusters[index]->points_count > 0 ||
			(globalParams->outlier_mod > 0 && index == 0) ||
			(globalParams->outlier_mod > 0 && index == 1 && group.local_clusters.size() == 2))
		{
			new_vec.push_back(group.local_clusters[index]);
		}
		else 
		{
			delete group.local_clusters[index];
//			globalParams->clusterInfos.remove(index, group.local_clusters[index]->points_count);
		}
	}

	//for each GPU
	{
		remove_empty_clusters_worker(pts_count);
	}

	group.local_clusters = new_vec;
}

void local_clusters_actions::reset_bad_clusters_worker(LabelsType &indices, MatrixXd &group_points)
{
	for (LabelType i = 0; i < indices.size(); i++)
	{
		globalParams->cuda->reset_bad_clusters_worker(indices[i]);
	}
}
												/*
												function reset_splitted_clusters!(group::local_group, bad_clusters::Vector{ Int64 })

												for i in bad_clusters
													group.local_clusters[i].cluster_params.logsublikelihood_hist = ones(burnout_period + 5)*-Inf
													end
													for i in(nworkers() == 0 ? procs() : workers())
														@spawnat i reset_bad_clusters_worker!(bad_clusters, group.points, group.labels, group.labels_subcluster)
														end
														update_suff_stats_posterior!(group, bad_clusters)
														end
														*/
void local_clusters_actions::reset_bad_clusters(prior *pPrior, local_group &group)
{
	LabelsType bad_clusters;
	bad_clusters.reserve(group.local_clusters.size());

	for (ClusterIndexType i = 0; i < group.local_clusters.size(); i++)
	{
		if (group.local_clusters[i]->cluster_params->cluster_params_l->suff_statistics->N == 0 ||
			group.local_clusters[i]->cluster_params->cluster_params_r->suff_statistics->N == 0)
		{
			bad_clusters.push_back(i);
			group.local_clusters[i]->cluster_params->logsublikelihood_hist = Logsublikelihood_hist(globalParams->burnout_period + 5, std::make_pair(false, 0));
			group.local_clusters[i]->cluster_params->splittable = false;
		}
	}

	//For each GPU
	{
		reset_bad_clusters_worker(bad_clusters, group.points);
	}
	update_suff_stats_posterior(pPrior, group, bad_clusters, true);
}
															
void local_clusters_actions::broadcast_cluster_params(std::vector<thin_cluster_params*> &params_vector, std::vector<double> &weights_vector)
{
	//TODO - For each GPU
//	@sync for (k, v) in leader_dict
//		@spawnat k broadcast_to_node(params_vector, weights_vector, v)
	set_global_data(params_vector, weights_vector);
}
																
															
//	#Below code is to prevent some bug with deseraliztion
//void local_clusters_actions::broadcast_to_node(params_vector, weights_vector, proc_ids)
//{
	//TODO - Set global data for each GPU
	//Verify that all without error
	/*refs = Dict()
		@sync for i in proc_ids
	{ refs[i] = @spawnat i set_global_data(params_vector, weights_vector)
	}
		alltrue = false
		responses = Dict([k = > fetch(v) for (k, v) in refs])
		for (k, v) in responses
		{ while v == false
		{	v = remotecall_fetch(set_global_data, k, params_vector, weights_vector)
		}
		}*/
//}

bool local_clusters_actions::set_global_data(std::vector<thin_cluster_params*> &params_vector, std::vector<double> &weights_vector)
{
	bool succ = true;

	globalParams->clusters_vector = params_vector;
	globalParams->clusters_weights = weights_vector;

	return succ;
}
																																																									
void local_clusters_actions::group_step(prior *pPrior, local_group &group, bool no_more_splits, bool bFinal, bool first)
{
//	static int i = 0;
//	++i;

	sample_clusters(group, false, pPrior);
	std::vector<thin_cluster_params*> tcp;
	create_thin_cluster_params(group.local_clusters, tcp);
	broadcast_cluster_params(tcp, group.weights);
	sample_labels(pPrior, group, (globalParams->hard_clustering ? true : bFinal), no_more_splits);

	sample_sub_clusters(pPrior, group);
	update_suff_stats_posterior(pPrior, group);
	reset_bad_clusters(pPrior, group);
	if (no_more_splits == false)
	{
		LabelsType indices;
		check_and_split(pPrior, group, bFinal, indices);
		update_suff_stats_posterior(pPrior, group, indices, true);
		check_and_merge(pPrior, group, bFinal);
	}
	remove_empty_clusters(group);

	if (globalParams->use_verbose)
	{
		LabelsType subLabels;
		globalParams->cuda->get_sub_labels(subLabels);
		draw::draw_labels("Sub labels", group.points, subLabels);

		LabelsType labels;
		globalParams->cuda->get_labels(labels);
		draw::draw_labels("Labels", group.points, labels);
	}
}