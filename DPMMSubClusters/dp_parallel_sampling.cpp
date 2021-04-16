#include <random>
#include <iostream>
#include <fstream>
#include <numeric>
using namespace std;
#include "ds.h"
#include "dp_parallel_sampling.h"
#include "local_clusters_actions.h"
#include "global_params.h"
#include "distributions_util/pdflib.hpp"
#include "priors/niw.h"
#include "priors/multinomial_prior.h"
#include "draw.h"
#include "utils.h"

dp_parallel_sampling_class::dp_parallel_sampling_class(int numLabels, MatrixXd& all_data, unsigned long long randomSeed, prior_type priorType)
{
	globalParams = std::make_shared<global_params>();
	globalParams->init(numLabels, all_data, randomSeed, priorType);
}

dp_parallel_sampling_class::dp_parallel_sampling_class(std::string modelDataFileName, std::string modelParamsFileName, prior_type priorType)
{
	globalParams = std::make_shared<global_params>();

	utils::load_data(modelDataFileName, globalParams->points);
	globalParams->init(modelParamsFileName, priorType);
}

//
//init_model()
//
//Initialize the model, loading the data from external `npy` files, specified in the params file.
//All prior data as been included previously, and is globaly accessed by the function.
//
//Returns an `dp_parallel_sampling` (e.g.the main data structure) with the configured parameters and data.

//
//init_model(all_data)
//
//Initialize the model, from `all_data`, should be `Dimensions X Samples`, type `Float32`
//All prior data as been included previously, and is globally accessed by the function.
//
//Returns an `dp_parallel_sampling` (e.g.the main data structure) with the configured parameters and data.
std::shared_ptr<dp_parallel_sampling> dp_parallel_sampling_class::init_model(MatrixXd& all_data)
{
	std::shared_ptr<dp_parallel_sampling> dps = std::make_shared<dp_parallel_sampling>();

	dps->group.points = all_data;
	dps->model_hyperparams.distribution_hyper_params = globalParams->hyper_params;
	dps->model_hyperparams.alpha = globalParams->alpha;
	dps->model_hyperparams.total_dim = (DimensionsType)dps->group.points.cols();
	dps->group.model_hyperparams = dps->model_hyperparams;

	globalParams->cuda->sample_labels(globalParams->initial_clusters, globalParams->outlier_mod);
	globalParams->cuda->sample_sub_labels();

	LabelsType subLabels;
	globalParams->cuda->get_sub_labels(subLabels);

	return dps;
}


//Initialize the first clusters in the model, according to the number defined by initial_cluster_count
void dp_parallel_sampling_class::init_first_clusters(std::shared_ptr<dp_parallel_sampling>& dp_model, ClusterIndexType initial_cluster_count)
{
	local_clusters_actions local_clusters_actions(globalParams);
	if (globalParams->outlier_mod > 0)
	{
		std::shared_ptr<local_cluster> lc = local_clusters_actions.create_outlier_local_cluster(dp_model->group, globalParams->outlier_hyper_params);
		dp_model->group.local_clusters.push_back(lc);
	}

	for (ClusterIndexType i = 0; i < initial_cluster_count; i++)
	{
		std::shared_ptr<local_cluster> lc = local_clusters_actions.create_first_local_cluster(dp_model->group);
		dp_model->group.local_clusters.push_back(lc);
	}

	local_clusters_actions.update_suff_stats_posterior(dp_model->group);
	local_clusters_actions.sample_clusters(dp_model->group, false);
	std::vector<std::shared_ptr<thin_cluster_params>> tcp;
	local_clusters_actions.create_thin_cluster_params(dp_model->group.local_clusters, tcp);
	std::vector<double> weights_vector;
	weights_vector.push_back(1.0);
	local_clusters_actions.broadcast_cluster_params(tcp, weights_vector);
}


ModelInfo dp_parallel_sampling_class::dp_parallel(
	std::shared_ptr<hyperparams>& local_hyper_params,
	double alpha_param,
	IterationIndexType iters,
	ClusterIndexType init_clusters,
	bool verbose,
	bool draw_labels,
	bool save_model,
	int burnout,
	double max_clusters,
	double outlier_weight,
	std::shared_ptr<hyperparams> outlier_params)
{
	globalParams->iterations = iters;
	globalParams->hyper_params = local_hyper_params;
	globalParams->initial_clusters = init_clusters;
	globalParams->alpha = alpha_param;
	globalParams->use_verbose = verbose;
	globalParams->draw_labels = draw_labels;
	globalParams->should_save_model = save_model;
	globalParams->burnout_period = burnout;
	globalParams->max_num_of_clusters = max_clusters;
	globalParams->outlier_mod = outlier_weight;
	globalParams->outlier_hyper_params = outlier_params;

	return init_and_run_model(globalParams->points);
}

ModelInfo dp_parallel_sampling_class::dp_parallel_from_file()
{
	return init_and_run_model(globalParams->points);
}

ModelInfo dp_parallel_sampling_class::init_and_run_model(MatrixXd& all_data)
{
	std::shared_ptr<dp_parallel_sampling> dp_model = init_model(all_data);

	init_first_clusters(dp_model, globalParams->initial_clusters);
	return run_model(dp_model, 1);
}

ModelInfo dp_parallel_sampling_class::run_model(std::shared_ptr<dp_parallel_sampling>& dp_model, int first_iter, const char* model_params, std::chrono::steady_clock::time_point prev_time)
{
	ModelInfo	modelInfo;
	std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
	modelInfo.dp_model = dp_model;
	std::vector<std::string> v_score_history;
	int cur_parr_count = 10;

	//For all GPUs
		//TODO - send only part dp_model.group.labels. only the amount of each GPU
	set_parr_worker(modelInfo.dp_model->group.num_labels(), cur_parr_count);

	for (int i = first_iter; i < globalParams->iterations; ++i)
	{
		bool final = false;
		bool no_more_splits = false;
		if (i >= globalParams->iterations - globalParams->argmax_sample_stop) //We assume the cluters k has been setteled by now, and a low probability random split can do dmg
		{
			final = true;
		}
		if (i >= globalParams->iterations - globalParams->split_stop || (modelInfo.dp_model->group.local_clusters.size()) >= globalParams->max_num_of_clusters)
		{
			no_more_splits = true;
		}

		prev_time = std::chrono::steady_clock::now();
		local_clusters_actions lca(globalParams);
		lca.group_step(modelInfo.dp_model->group, no_more_splits, final, i == 1);

		const double iter_time = (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - prev_time).count()) / 1000.0;
		modelInfo.iter_count.push_back(iter_time);

		modelInfo.cluster_count_history.push_back((ClusterIndexType)modelInfo.dp_model->group.local_clusters.size());
		
		if (globalParams->use_verbose)
		{
			modelInfo.likelihood_history.push_back(calculate_posterior(modelInfo.dp_model));
			printf("Iteration: %ld || Clusters count: %ld\n", i, modelInfo.cluster_count_history.back());
			//printf("Iteration: %ld || Clusters count: %ld || Log posterior: %f || Vi score: %s || NMI score: %s || Iter Time: %f  || Total time: %f\n",
			//	i,
			//	modelInfo.cluster_count_history.back(),
			//	modelInfo.likelihood_history.back(),
			////	v_score_history.back().c_str(),
			//	"TBD",
			//	//modelInfo.nmi_score_history.back().c_str(),
			//	"TBD",
			//	iter_time,
			//	std::accumulate(modelInfo.iter_count.begin(), modelInfo.iter_count.end(),
			//		decltype(modelInfo.iter_count)::value_type(0)));
		}
		else
		{
			modelInfo.likelihood_history.push_back(1);
		}
		if (i % globalParams->model_save_interval == 0 && globalParams->should_save_model)
		{
			printf("Saving Model:\n");
			//TODO - print time @time
			long long time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time).count();
			save_model(modelInfo.dp_model, globalParams->save_path, globalParams->save_file_prefix, i, time, model_params);
		}
	}

//	globalParams->clusterInfos.print();

	return modelInfo;
}

void dp_parallel_sampling_class::save_model(std::shared_ptr<dp_parallel_sampling>& model,std::string path, std::string filename,long  iter, long long total_time, const char * global_params)
{
	std::string fileName = path + filename + "_" + std::to_string(iter) + ".jld2";
	model_hyper_params hyperparams = model->model_hyperparams;
	pts_less_group* group = ds::create_pts_less_group(model->group);
	std::ofstream myfile(fileName);
	myfile << group << hyperparams << iter << total_time << global_params;
	myfile.close();
}


double dp_parallel_sampling_class::calculate_posterior(std::shared_ptr<dp_parallel_sampling>& model)
{
	//TODO - should be logabsgamma <= abs
	double log_posterior = r8_gamma_log(model->model_hyperparams.alpha) - r8_gamma_log(model->group.points.cols() + model->model_hyperparams.alpha);
	for (ClusterIndexType i = 0; i < model->group.local_clusters.size(); ++i)
	{
		std::shared_ptr<local_cluster> cluster = model->group.local_clusters[i];
		if (cluster->cluster_params->cluster_params->suff_statistics->N == 0)
		{
			continue;
		}

		log_posterior += globalParams->pPrior->log_marginal_likelihood(cluster->cluster_params->cluster_params->prior_hyperparams,
			cluster->cluster_params->cluster_params->posterior_hyperparams,
			cluster->cluster_params->cluster_params->suff_statistics);

		log_posterior += log(model->model_hyperparams.alpha) + r8_gamma_log(cluster->cluster_params->cluster_params->suff_statistics->N);
	}
	return log_posterior;
}

	
void dp_parallel_sampling_class::set_parr_worker(LabelType numLabels, int cluster_count)
{
	globalParams->glob_parr = MatrixXd(numLabels, cluster_count);
}