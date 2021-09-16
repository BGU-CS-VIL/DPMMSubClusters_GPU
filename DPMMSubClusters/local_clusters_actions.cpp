#include <map>
#include <random>
#include <numeric>
#include "local_clusters_actions.h"
#include "utils.h"
#include "global_params.h"
#include "shared_actions.h"
#include "distributions_util/dirichlet.h"
#include <iostream>
using namespace std;
#include "distributions_util/pdflib.hpp"
#include "draw.h"
// #include <ppl.h>
#include "check_time.h"

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

std::shared_ptr<local_cluster> local_clusters_actions::create_first_local_cluster(local_group &group)
{
	std::shared_ptr<local_cluster> cluster = std::make_shared<local_cluster>();
	cluster->cluster_params = std::make_shared<splittable_cluster_params>();
	cluster->cluster_params->cluster_params = std::make_shared<cluster_parameters>();

	cluster->cluster_params->cluster_params->suff_statistics = globalParams->pPrior->create_sufficient_statistics(group.model_hyperparams.distribution_hyper_params, group.model_hyperparams.distribution_hyper_params, Eigen::MatrixXd(0,0));
	std::shared_ptr<hyperparams> post = group.model_hyperparams.distribution_hyper_params;
	std::shared_ptr<distribution_sample> dist = globalParams->pPrior->sample_distribution(post, globalParams->gen, globalParams->cuda);
	cluster->cluster_params->cluster_params->prior_hyperparams = group.model_hyperparams.distribution_hyper_params->clone();
	cluster->cluster_params->cluster_params->distribution = dist;
	cluster->cluster_params->cluster_params->posterior_hyperparams = post->clone();
	cluster->cluster_params->cluster_params->suff_statistics->N = (int)group.points.cols();
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

	split_first_cluster_worker(group);
	return cluster;
}


std::shared_ptr<local_cluster> local_clusters_actions::create_outlier_local_cluster(local_group& group, std::shared_ptr<hyperparams>& outlier_params)
{
	std::shared_ptr<local_cluster> cluster = std::make_shared<local_cluster>();
	cluster->cluster_params = std::make_shared<splittable_cluster_params>();
	cluster->cluster_params->cluster_params = std::make_shared<cluster_parameters>();

	std::shared_ptr<sufficient_statistics> suff = globalParams->pPrior->create_sufficient_statistics(outlier_params, outlier_params, group.points);
	std::shared_ptr<hyperparams> post = globalParams->pPrior->calc_posterior(outlier_params, suff);
	std::shared_ptr<distribution_sample> dist = globalParams->pPrior->sample_distribution(post, globalParams->gen, globalParams->cuda);
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
	cluster->cluster_params->cluster_params->suff_statistics->N = (int)group.points.cols();

	globalParams->cuda->get_sub_labels_count(
		cluster->cluster_params->cluster_params_l->suff_statistics->N,
		cluster->cluster_params->cluster_params_r->suff_statistics->N);

	cluster->total_dim = group.model_hyperparams.total_dim;
	cluster->points_count = cluster->cluster_params->cluster_params->suff_statistics->N;
	cluster->l_count = cluster->cluster_params->cluster_params_l->suff_statistics->N;
	cluster->r_count = cluster->cluster_params->cluster_params_r->suff_statistics->N;
	return cluster;
}


void local_clusters_actions::sample_sub_clusters(local_group& group)
{
	CHECK_TIME("local_clusters_actions::sample_sub_clusters", globalParams->use_verbose);
	globalParams->cuda->create_subclusters_labels((int)globalParams->clusters_vector.size(), globalParams->clusters_vector, (int)group.points.rows());
}
				
void local_clusters_actions::sample_labels(local_group& group, bool bFinal, bool no_more_splits)
{
	CHECK_TIME("local_clusters_actions::sample_labels", globalParams->use_verbose);
	globalParams->cuda->create_clusters_labels((int)globalParams->clusters_vector.size(), globalParams->clusters_vector, globalParams->clusters_weights, bFinal);
}

void local_clusters_actions::update_splittable_cluster_params(std::shared_ptr<splittable_cluster_params>& splittable_cluser)
{
	CHECK_TIME("local_clusters_actions::update_splittable_cluster_params", globalParams->use_verbose);
	splittable_cluser->cluster_params->posterior_hyperparams = globalParams->pPrior->calc_posterior(splittable_cluser->cluster_params->prior_hyperparams, splittable_cluser->cluster_params->suff_statistics);
	splittable_cluser->cluster_params_l->posterior_hyperparams = globalParams->pPrior->calc_posterior(splittable_cluser->cluster_params_l->prior_hyperparams, splittable_cluser->cluster_params_l->suff_statistics);
	splittable_cluser->cluster_params_r->posterior_hyperparams = globalParams->pPrior->calc_posterior(splittable_cluser->cluster_params_r->prior_hyperparams, splittable_cluser->cluster_params_r->suff_statistics);
}

std::map<LabelType, std::shared_ptr<thin_suff_stats>> local_clusters_actions::create_suff_stats_dict_worker(MatrixXd& group_pts, std::shared_ptr<hyperparams>& hyper_params, LabelsType& indices)
{
	CHECK_TIME("local_clusters_actions::create_suff_stats_dict_worker", globalParams->use_verbose);

	return globalParams->cuda->create_sufficient_statistics(indices, hyper_params, hyper_params);
}

void local_clusters_actions::update_suff_stats_posterior(local_group &group)
{
	LabelsType indices;
	update_suff_stats_posterior(group, indices, false);
}

void local_clusters_actions::update_suff_stats_posterior(local_group& group, LabelsType& indices, bool indicesValid)
{
	CHECK_TIME("local_clusters_actions::update_suff_stats_posterior", globalParams->use_verbose);
	std::map<int, std::map<LabelType, std::shared_ptr<thin_suff_stats>>> workers_suff_dict;
	if (!indicesValid)
	{
		CHECK_TIME("local_clusters_actions:: update_suff_stats_posterior fill indices", globalParams->use_verbose);
		indices.resize(group.local_clusters.size());
		std::iota(std::begin(indices), std::end(indices), 0); // Fill with 0, 1, 2...
	}

	workers_suff_dict[0] = create_suff_stats_dict_worker(group.points,
		group.model_hyperparams.distribution_hyper_params,
		indices);

	std::vector<std::vector<std::shared_ptr<thin_suff_stats>>>	suff_stats_vectors(indices.size());
	{
		CHECK_TIME("local_clusters_actions:: update_suff_stats_posterior loop 1", globalParams->use_verbose);

		std::map<ClusterIndexType, ClusterIndexType> cluster_to_index;
		for (LabelType i = 0; i < indices.size(); i++)
		{
			cluster_to_index[indices[i]] = i;
		}

		for (std::map<int, std::map<LabelType, std::shared_ptr<thin_suff_stats>>>::const_iterator iter = workers_suff_dict.begin();
			iter != workers_suff_dict.end();
			++iter)
		{
			for (std::map<LabelType, std::shared_ptr<thin_suff_stats>>::const_iterator iter2 = iter->second.begin();
				iter2 != iter->second.end();
				++iter2)
			{
				suff_stats_vectors[cluster_to_index[iter2->first]].push_back(iter2->second);
			}
		}
	}

	{
		int sum = 0;
		CHECK_TIME("local_clusters_actions:: update_suff_stats_posterior loop 2", globalParams->use_verbose);
		for (LabelType index = 0; index < indices.size(); index++)
		{
			if (suff_stats_vectors[index].size() == 0)
			{
				continue;
			}

			std::shared_ptr<local_cluster> pCluster = group.local_clusters[indices[index]];
			pCluster->cluster_params->cluster_params->suff_statistics = suff_stats_vectors[index][0]->cluster_suff;
			pCluster->cluster_params->cluster_params_l->suff_statistics = suff_stats_vectors[index][0]->l_suff;
			pCluster->cluster_params->cluster_params_r->suff_statistics = suff_stats_vectors[index][0]->r_suff;

			for (LabelType i = 1; i < suff_stats_vectors[index].size(); i++)
			{
				globalParams->pPrior->aggregate_suff_stats(pCluster->cluster_params->cluster_params->suff_statistics, suff_stats_vectors[index][i]->cluster_suff, pCluster->cluster_params->cluster_params->suff_statistics);
				globalParams->pPrior->aggregate_suff_stats(pCluster->cluster_params->cluster_params_l->suff_statistics, suff_stats_vectors[index][i]->l_suff, pCluster->cluster_params->cluster_params_l->suff_statistics);
				globalParams->pPrior->aggregate_suff_stats(pCluster->cluster_params->cluster_params_r->suff_statistics, suff_stats_vectors[index][i]->r_suff, pCluster->cluster_params->cluster_params_r->suff_statistics);
			}

			pCluster->points_count = pCluster->cluster_params->cluster_params->suff_statistics->N;
			update_splittable_cluster_params(pCluster->cluster_params);
		}
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

void local_clusters_actions::split_cluster_local(local_group &group, std::shared_ptr<local_cluster>& cluster, ClusterIndexType index, ClusterIndexType new_index)
{
	shared_actions sa(globalParams);
	std::shared_ptr<local_cluster> l_split = cluster->clone();
	sa.create_splittable_from_params(cluster->cluster_params->cluster_params_r, group.model_hyperparams.alpha, l_split->cluster_params);
	sa.create_splittable_from_params(cluster->cluster_params->cluster_params_l, group.model_hyperparams.alpha, cluster->cluster_params);

	l_split->points_count = l_split->cluster_params->cluster_params->suff_statistics->N;
	cluster->points_count = cluster->cluster_params->cluster_params->suff_statistics->N;

	group.local_clusters[new_index] = l_split;
}
																										
void local_clusters_actions::merge_clusters_worker(LabelsType &indices, LabelsType &new_indices)
{
	for (LabelType i = 0; i < indices.size(); i++)
	{
//		printf("\n\n\n************** merge_clusters_worker ******************\n\n\n");

		globalParams->cuda->merge_clusters_worker(indices[i], new_indices[i]);
	}
}
																											
void local_clusters_actions::merge_clusters(local_group &group, ClusterIndexType index_l, ClusterIndexType index_r)
{
	shared_actions sa(globalParams);

	sa.merge_clusters_to_splittable(group.local_clusters[index_l]->cluster_params, group.local_clusters[index_r]->cluster_params->cluster_params, group.model_hyperparams.alpha);
	group.local_clusters[index_l]->points_count += group.local_clusters[index_r]->points_count;
	group.local_clusters[index_r]->points_count = 0;
	group.local_clusters[index_r]->cluster_params->cluster_params->suff_statistics->N = 0;
	group.local_clusters[index_r]->cluster_params->splittable = false;
}

																											
void local_clusters_actions::should_split_local(double& should_split, std::shared_ptr<splittable_cluster_params>& cluster_params, double alpha, bool bFinal)
{
	if (bFinal || cluster_params->cluster_params_l->suff_statistics->N == 0 || cluster_params->cluster_params_r->suff_statistics->N == 0)
	{
		should_split = 0;
		return;
	}
	std::shared_ptr<hyperparams> post = globalParams->pPrior->calc_posterior(cluster_params->cluster_params->prior_hyperparams, cluster_params->cluster_params->suff_statistics);
	std::shared_ptr<hyperparams> lpost = globalParams->pPrior->calc_posterior(cluster_params->cluster_params->prior_hyperparams, cluster_params->cluster_params_l->suff_statistics);
	std::shared_ptr<hyperparams> rpost = globalParams->pPrior->calc_posterior(cluster_params->cluster_params->prior_hyperparams, cluster_params->cluster_params_r->suff_statistics);

	double log_likihood_l = globalParams->pPrior->log_marginal_likelihood(cluster_params->cluster_params_l->prior_hyperparams, lpost, cluster_params->cluster_params_l->suff_statistics);
	double log_likihood_r = globalParams->pPrior->log_marginal_likelihood(cluster_params->cluster_params_r->prior_hyperparams, rpost, cluster_params->cluster_params_r->suff_statistics);
	double log_likihood = globalParams->pPrior->log_marginal_likelihood(cluster_params->cluster_params->prior_hyperparams, post, cluster_params->cluster_params->suff_statistics);

	double log_HR = log(alpha) +
		r8_gamma_log(cluster_params->cluster_params_l->suff_statistics->N) + log_likihood_l +
		r8_gamma_log(cluster_params->cluster_params_r->suff_statistics->N) + log_likihood_r -
		(r8_gamma_log(cluster_params->cluster_params->suff_statistics->N) + log_likihood);

	std::uniform_real_distribution<double> dist(0, 1);
	double a_random_double = dist(*globalParams->gen);

	if (log_HR > log(a_random_double))
	{
		should_split = 1;
	}
}
																													
void local_clusters_actions::check_and_split(local_group& group, bool bFinal, LabelsType& all_indices)
{
	CHECK_TIME("local_clusters_actions::check_and_split", globalParams->use_verbose);
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
			should_split_local(split_arr[index], group.local_clusters[index]->cluster_params, group.model_hyperparams.alpha, bFinal);
		}
	}
	ClusterIndexType new_index = (ClusterIndexType)group.local_clusters.size();
	LabelsType indices;
	LabelsType new_indices;

	group.local_clusters.resize(group.local_clusters.size() + std::accumulate(split_arr.begin(), split_arr.end(), 0));

	for (ClusterIndexType i = 0; i < split_arr.size(); i++)
	{
		if (split_arr[i] == 1)
		{
			indices.push_back(i);
			new_indices.push_back(new_index);
			split_cluster_local(group, group.local_clusters[i], i, new_index);
			
	//		printf("Split: %ld\n", new_index);
			++new_index;
		}
	}

	all_indices = indices;
	all_indices.insert(all_indices.end(), new_indices.begin(), new_indices.end());

	if (indices.size() > 0)
	{
		split_cluster_local_worker(indices, new_indices);
	}
}

void local_clusters_actions::check_and_merge(local_group& group, bool bFinal)
{
	CHECK_TIME("local_clusters_actions::check_and_merge", globalParams->use_verbose);
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

		for (ClusterIndexType j = i + 1; j < group.local_clusters.size(); j++)
		{
			if (group.local_clusters[i]->cluster_params->splittable == true &&
				group.local_clusters[j]->cluster_params->splittable == true &&
				group.local_clusters[i]->cluster_params->cluster_params->suff_statistics->N > 0 &&
				group.local_clusters[j]->cluster_params->cluster_params->suff_statistics->N > 0)
			{
				sa.should_merge(mergable, group.local_clusters[i]->cluster_params->cluster_params,
					group.local_clusters[j]->cluster_params->cluster_params, group.model_hyperparams.alpha, bFinal);
			}
			if (mergable == true)
			{
				merge_clusters(group, i, j);
				indices.push_back(i);
				new_indices.push_back(j);
			}
			mergable = false;
		}
	}

	merge_clusters_worker(indices, new_indices);
}

std::vector<double> local_clusters_actions::get_dirichlet_distribution(std::vector<double> &points_count)
{
	dirichlet_distribution<std::mt19937> d(points_count);
	return d(*globalParams->gen);
}

void local_clusters_actions::sample_clusters(local_group &group, bool first)
{
	CHECK_TIME("local_clusters_actions::sample_clusters", globalParams->use_verbose);

	shared_actions sa(globalParams);
	std::vector<double> points_count;

	{
		CHECK_TIME("local_clusters_actions:: Build sample_cluster_params", globalParams->use_verbose);
		for (ClusterIndexType i = 0; i < group.local_clusters.size(); i++)
		{
			if (globalParams->outlier_mod > 0 && i == 1)
			{
				continue;
			}
			sa.sample_cluster_params(group.local_clusters[i]->cluster_params, group.model_hyperparams.alpha, first);
			group.local_clusters[i]->points_count = group.local_clusters[i]->cluster_params->cluster_params->suff_statistics->N;
			points_count.push_back(group.local_clusters[i]->points_count);
			//printf("Cluster #%ld: count=%ld\n", i, group.local_clusters[i]->points_count);
		}
	}

	points_count.push_back(group.model_hyperparams.alpha);

	std::vector<double> dirichlet;
	{
		CHECK_TIME("local_clusters_actions:: get_dirichlet_distribution", globalParams->use_verbose);
		dirichlet = get_dirichlet_distribution(points_count);
	}

	group.weights.clear();
	{
		CHECK_TIME("local_clusters_actions:: Build group.weights", globalParams->use_verbose);
		for (ClusterIndexType i = 0; i < dirichlet.size() - 1; ++i)
		{
			group.weights.push_back(dirichlet[i] * (1 - globalParams->outlier_mod));
			//printf("Cluster #%ld: weight=%f\n", i, group.weights.back());
		}
	}
	if (globalParams->outlier_mod > 0)
	{
		group.weights.insert(group.weights.begin(), globalParams->outlier_mod);
	}
}

						
void local_clusters_actions::create_thin_cluster_params(std::vector<std::shared_ptr<local_cluster>> &clusters, std::vector<std::shared_ptr<thin_cluster_params>> &tcp)
{
	CHECK_TIME("local_clusters_actions::create_thin_cluster_params", globalParams->use_verbose);
	for (ClusterIndexType i = 0; i < clusters.size(); i++)
	{
		tcp.push_back(std::make_shared<thin_cluster_params>(clusters[i]->cluster_params->cluster_params->distribution->clone(),
			clusters[i]->cluster_params->cluster_params_l->distribution->clone(),
			clusters[i]->cluster_params->cluster_params_r->distribution->clone(),
			clusters[i]->cluster_params->lr_weights));
	}
}

void local_clusters_actions::remove_empty_clusters_worker(std::vector<PointType> &pts_count)
{
	PointType removed = 0;
	int sum = 0;

	for (PointType index = 0; index < pts_count.size(); index++)
	{
		sum += pts_count[index];
		//printf("Cluster #%ld count:%ld\n", index, pts_count[index]);
		if (pts_count[index] == 0)
		{
	//		printf("\n************** remove_empty_clusters_worker %ld******************\n", index);

			globalParams->cuda->remove_empty_clusters_worker(index - removed + 1);
			++removed;
		}
	//	printf("\n************** removed %ld ******************\n", removed);

	}
}

void local_clusters_actions::remove_empty_clusters(local_group& group)
{
	CHECK_TIME("local_clusters_actions::remove_empty_clusters", globalParams->use_verbose);
	std::vector<std::shared_ptr<local_cluster>> new_vec;
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
	}

	remove_empty_clusters_worker(pts_count);

	group.local_clusters = new_vec;
}

void local_clusters_actions::reset_bad_clusters_worker(LabelsType &indices, MatrixXd &group_points)
{
	for (LabelType i = 0; i < indices.size(); i++)
	{
		globalParams->cuda->reset_bad_clusters_worker(indices[i]);
	}
}

void local_clusters_actions::reset_bad_clusters(local_group& group)
{
	CHECK_TIME("local_clusters_actions::reset_bad_clusters", globalParams->use_verbose);
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

	reset_bad_clusters_worker(bad_clusters, group.points);
	update_suff_stats_posterior(group, bad_clusters, true);
}
															
void local_clusters_actions::broadcast_cluster_params(std::vector<std::shared_ptr<thin_cluster_params>>& params_vector, std::vector<double> &weights_vector)
{
	CHECK_TIME("local_clusters_actions::broadcast_cluster_params", globalParams->use_verbose);
	set_global_data(params_vector, weights_vector);
}											

void local_clusters_actions::set_global_data(const std::vector<std::shared_ptr<thin_cluster_params>> &params_vector, const std::vector<double> &weights_vector)
{
	globalParams->clusters_vector = params_vector;
	globalParams->clusters_weights = weights_vector;
}
																																																									
void local_clusters_actions::group_step(local_group &group, bool no_more_splits, bool bFinal, bool first)
{
	CHECK_TIME("local_clusters_actions::group_step", globalParams->use_verbose);
	sample_clusters(group, false);
	std::vector<std::shared_ptr<thin_cluster_params>> tcp;
	create_thin_cluster_params(group.local_clusters, tcp);
	broadcast_cluster_params(tcp, group.weights);
	sample_labels(group, (globalParams->hard_clustering ? true : bFinal), no_more_splits);

	sample_sub_clusters(group);
	update_suff_stats_posterior(group);
	reset_bad_clusters(group);
	if (no_more_splits == false)
	{
		LabelsType indices;
		check_and_split(group, bFinal, indices);
		update_suff_stats_posterior(group, indices, true);
		check_and_merge(group, bFinal);
	}
	remove_empty_clusters(group);

	if (globalParams->draw_labels)
	{
		std::shared_ptr<LabelsType> subLabels = std::make_shared<LabelsType>();
		globalParams->cuda->get_sub_labels(subLabels);
		draw::draw_labels("Sub labels", group.points, subLabels);

		std::shared_ptr<LabelsType> labels = std::make_shared<LabelsType>();
		globalParams->cuda->get_labels(labels);
		draw::draw_labels("Labels", group.points, labels);
	}
}