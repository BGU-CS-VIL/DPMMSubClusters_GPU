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
#include "clusterInfo.h"
#include "utils.h"

/*
init_model()

Initialize the model, loading the data from external `npy` files, specified in the params file.
All prior data as been included previously, and is globaly accessed by the function.

Returns an `dp_parallel_sampling` (e.g.the main data structure) with the configured parameters and data.
*/

dp_parallel_sampling* dp_parallel_sampling_class::init_model_from_file()
{
	dp_parallel_sampling *dps = new dp_parallel_sampling();

	std::chrono::steady_clock::time_point begin;

	if (globalParams->use_verbose)
	{
		begin = std::chrono::steady_clock::now();
	}

	//TODO - split the data and copy the subset data to each GPUs.
	//data that was returned is reference metadata to each subset that exists in the GPU. Do I need it?
	utils::load_data(globalParams->data_path, globalParams->data_prefix, dps->group.points);
	utils::saveToFile(dps->group.points, "ReadPnyFileIntoData");

	if (globalParams->use_verbose)
	{
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::cout << "Loading and distributing data took = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[ms]" << std::endl;
	}

	dps->model_hyperparams.distribution_hyper_params = globalParams->hyper_params;
	dps->model_hyperparams.alpha = globalParams->alpha;
	dps->model_hyperparams.total_dim = dps->group.points.cols();
	dps->group.model_hyperparams = dps->model_hyperparams;

	//Each GPU needs to calculate that:

	std::uniform_int_distribution<LabelType> uni(1, globalParams->initial_clusters); // guaranteed unbiased

	//TODO
//	dps->group.labels.resize(dps->group.points.cols(), 1);
//	for (PointType i = 0; i < dps->group.points.cols(); i++)
//	{
//		dps->group.labels[i] = uni(rng) + ((globalParams->outlier_mod > 0) ? 1 : 0);
//	}

	//Also for each GPU
	//std::uniform_int_distribution<LabelType> uni2(1, 2); // guaranteed unbiased
	//dps->group.labels_subcluster.resize(dps->group.points.cols(), 1);
	//for (PointType i = 0; i < dps->group.points.cols(); i++)
	//{
	//	dps->group.labels_subcluster[i] = uni2(rng);
	//}
	//draw::CreatePng("draw\\firstCluster.png", dps->group.points, dps->group.labels_subcluster);

	return dps;
}

/*
init_model(all_data)

Initialize the model, from `all_data`, should be `Dimensions X Samples`, type `Float32`
All prior data as been included previously, and is globaly accessed by the function.

Returns an `dp_parallel_sampling` (e.g.the main data structure) with the configured parameters and data.
*/



dp_parallel_sampling* dp_parallel_sampling_class::init_model_from_data(MatrixXd &all_data)
{
	dp_parallel_sampling *dps = new dp_parallel_sampling();
	
	std::chrono::steady_clock::time_point begin;

	if (globalParams->use_verbose)
	{
		begin = std::chrono::steady_clock::now();
	}

	//TODO - split the data and copy the subset data to each GPUs.
	//data that was returned is reference metadata to each subset that exists in the GPU. Do I need it?
	//data = distribute(all_data);
	dps->group.points = all_data;

	if (globalParams->use_verbose)
	{
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::cout << "Loading and distributing data took = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[ms]" << std::endl;
	}

	dps->model_hyperparams.distribution_hyper_params = globalParams->hyper_params;
	dps->model_hyperparams.alpha = globalParams->alpha;
	dps->model_hyperparams.total_dim = dps->group.points.cols();
	dps->group.model_hyperparams = dps->model_hyperparams;

	//Each GPU needs to calculate that:
	
	//std::uniform_int_distribution<LabelType> uni(1, globalParams->initial_clusters); // guaranteed unbiased
	//dps->group.labels.resize(dps->group.points.cols(), 1);
	//for (PointType i = 0; i < dps->group.points.cols(); i++)
	//{
	//	dps->group.labels[i] = uni(rng) + ((globalParams->outlier_mod > 0) ? 1 : 0);
	//}
	globalParams->cuda->sample_labels(globalParams->initial_clusters, globalParams->outlier_mod);
	globalParams->cuda->sample_sub_labels();

	LabelsType subLabels;
	globalParams->cuda->get_sub_labels(subLabels);

//	draw::CreatePng("firstCluster", dps->group.points, subLabels);

	return dps;
}


/*
init_first_clusters!(dp_model::dp_parallel_sampling, initial_cluster_count::Int64))

Initialize the first clusters in the model, according to the number defined by initial_cluster_count

Mutates the model.
*/


void dp_parallel_sampling_class::init_first_clusters(dp_parallel_sampling* &dp_model, ClusterIndexType initial_cluster_count, prior **pPrior)
{
	local_clusters_actions local_clusters_actions(globalParams);
	if (globalParams->outlier_mod > 0)
	{
		local_cluster *lc = local_clusters_actions.create_outlier_local_cluster(dp_model->group, globalParams->outlier_hyper_params,*pPrior);
		dp_model->group.local_clusters.push_back(lc);
	}

	for (ClusterIndexType i = 0; i < initial_cluster_count; i++)
	{
		local_cluster *lc = local_clusters_actions.create_first_local_cluster(dp_model->group, pPrior);
		dp_model->group.local_clusters.push_back(lc);
	}
	//synch all GPU call - TODO

	local_clusters_actions.update_suff_stats_posterior(*pPrior, dp_model->group);
	local_clusters_actions.sample_clusters(dp_model->group, false, *pPrior);
	std::vector<thin_cluster_params*> tcp;
	local_clusters_actions.create_thin_cluster_params(dp_model->group.local_clusters, tcp);
	std::vector<double> weights_vector;
	weights_vector.push_back(1.0);
	local_clusters_actions.broadcast_cluster_params(tcp, weights_vector);
}


/*
dp_parallel(all_data::AbstractArray{ Float32,2 },
	local_hyper_params::distribution_hyper_params,
	alpha_param::Float32,
	iters::Int64 = 100,
	init_clusters::Int64 = 1,
	seed = nothing,
	verbose = true,
	save_model = false,
	burnout = 15,
	gt = nothing,
	max_clusters = Inf,
	outlier_weight = 0,
	outlier_params = nothing)

	Run the model.
	# Args and Kwargs
	- `all_data::AbstractArray{ Float32,2 }` a `DxN` array containing the data
	- `local_hyper_params::distribution_hyper_params` the prior hyperparams
	- `alpha_param::Float32` the concetration parameter
	- `iters::Int64` number of iterations to run the model
	- `init_clusters::Int64` number of initial clusters
	- `seed` define a random seed to be used in all workers, if used must be preceeded with `@everywhere using random`.
	- `verbose` will perform prints on every iteration.
	- `save_model` will save a checkpoint every 25 iterations.
	- `burnout` how long to wait after creating a cluster, and allowing it to split / merge
	- `gt` Ground truth, when supplied, will perform NMI and VI analysis on every iteration.
	- `max_clusters` limit the number of cluster
	- `outlier_weight` constant weight of an extra non - spliting component
	- `outlier_params` hyperparams for an extra non - spliting component

	# Return values
	dp_model, iter_count, nmi_score_history, liklihood_history, cluster_count_history
	- `dp_model` The DPMM model inferred
	- `iter_count` Timing for each iteration
	- `nmi_score_history` NMI score per iteration(if gt suppled)
	- `likelihood_history` Log likelihood per iteration.
	- `cluster_count_history` Cluster counts per iteration.
	*/
	
	
ModelInfo dp_parallel_sampling_class::dp_parallel(
	hyperparams* local_hyper_params,
	double alpha_param,
	IterationIndexType iters,
	ClusterIndexType init_clusters,
	bool verbose,
	bool save_model,
	int burnout,
	std::vector<double> gt,
	double max_clusters,
	double outlier_weight,
	hyperparams* outlier_params)
{
	globalParams->iterations = iters;
	globalParams->hyper_params = local_hyper_params;
	globalParams->initial_clusters = init_clusters;
	globalParams->alpha = alpha_param;
	globalParams->use_verbose = verbose;
	globalParams->should_save_model = save_model;
	globalParams->burnout_period = burnout;
	globalParams->max_num_of_clusters = max_clusters;
	globalParams->outlier_mod = outlier_weight;
	globalParams->outlier_hyper_params = outlier_params;
	dp_parallel_sampling *dp_model = init_model_from_data(globalParams->points);
	prior *pPrior = NULL;
	//	leader_dict = get_node_leaders_dict();
	init_first_clusters(dp_model, globalParams->initial_clusters, &pPrior);
	if (globalParams->use_verbose)
	{
		printf("Node Leaders:\n");
		//		printf(leader_dict);
		printf("\n");
	}
	globalParams->ground_truth = gt;
	return run_model(pPrior, dp_model, 1);
}

		

		/*
		fit(all_data::AbstractArray{ Float32,2 }, local_hyper_params::distribution_hyper_params, alpha_param::Float32;
iters::Int64 = 100, init_clusters::Int64 = 1, seed = nothing, verbose = true, save_model = false, burnout = 20, gt = nothing, max_clusters = Inf, outlier_weight = 0, outlier_params = nothing)

Run the model(basic mode).
# Args and Kwargs
- `all_data::AbstractArray{ Float32,2 }` a `DxN` array containing the data
- `local_hyper_params::distribution_hyper_params` the prior hyperparams
- `alpha_param::Float32` the concetration parameter
- `iters::Int64` number of iterations to run the model
- `init_clusters::Int64` number of initial clusters
- `seed` define a random seed to be used in all workers, if used must be preceeded with `@everywhere using random`.
- `verbose` will perform prints on every iteration.
- `save_model` will save a checkpoint every 25 iterations.
- `burnout` how long to wait after creating a cluster, and allowing it to split / merge
- `gt` Ground truth, when supplied, will perform NMI and VI analysis on every iteration.
- `max_clusters` limit the number of cluster
- `outlier_weight` constant weight of an extra non - spliting component
- `outlier_params` hyperparams for an extra non - spliting component

# Return Values
- `labels` Labels assignments
- `clusters` Cluster parameters
- `weights` The cluster weights, does not sum to `1`, but to `1` minus the weight of all uninstanistaed clusters.
- `iter_count` Timing for each iteration
- `nmi_score_history` NMI score per iteration(if gt suppled)
- `likelihood_history` Log likelihood per iteration.
- `cluster_count_history` Cluster counts per iteration.
- `sub_labels` Sub labels assignments

# Example:
```julia
julia > x, y, clusters = generate_gaussian_data(10000, 2, 6, 100.0)
...

julia > hyper_params = DPMMSubClusters.niw_hyperparams(1.0,
	zeros(2),
	5,
	[1 0; 0 1])
	DPMMSubClusters.niw_hyperparams(1.0f0, Float32[0.0, 0.0], 5.0f0, Float32[1.0 0.0; 0.0 1.0])

	julia > ret_values = fit(x, hyper_params, 10.0, iters = 100, verbose = false)

	...

	julia > unique(ret_values[1])
	6 - element Array{ Int64,1 }:
3
6
1
2
5
4
```
*/
/*
function fit(all_data::AbstractArray{ Float32,2 }, local_hyper_params::distribution_hyper_params, alpha_param::Float32;
iters::Int64 = 100, init_clusters::Int64 = 1, seed = nothing, verbose = true, save_model = false, burnout = 20, gt = nothing, max_clusters = Inf, outlier_weight = 0, outlier_params = nothing)
dp_model, iter_count, nmi_score_history, liklihood_history, cluster_count_history = dp_parallel(all_data, local_hyper_params, alpha_param, iters, init_clusters, seed, verbose, save_model, burnout, gt, max_clusters, outlier_weight, outlier_params)
return Array(dp_model.group.labels), [x.cluster_params.cluster_params.distribution for x in dp_model.group.local_clusters], dp_model.group.weights, iter_count, nmi_score_history, liklihood_history, cluster_count_history, Array(dp_model.group.labels_subcluster)
end

*/
/*
fit(all_data::AbstractArray{ Float32,2 }, alpha_param::Float32;
iters::Int64 = 100, init_clusters::Int64 = 1, seed = nothing, verbose = true, save_model = false, burnout = 20, gt = nothing, max_clusters = Inf, outlier_weight = 0, outlier_params = nothing)


Run the model(basic mode) with default `NIW` prior.
# Args and Kwargs
- `all_data::AbstractArray{ Float32,2 }` a `DxN` array containing the data
- `alpha_param::Float32` the concetration parameter
- `iters::Int64` number of iterations to run the model
- `init_clusters::Int64` number of initial clusters
- `seed` define a random seed to be used in all workers, if used must be preceeded with `@everywhere using random`.
- `verbose` will perform prints on every iteration.
- `save_model` will save a checkpoint every 25 iterations.
- `burnout` how long to wait after creating a cluster, and allowing it to split / merge
- `gt` Ground truth, when supplied, will perform NMI and VI analysis on every iteration.
- `outlier_weight` constant weight of an extra non - spliting component
- `outlier_params` hyperparams for an extra non - spliting component

# Return Values
- `labels` Labels assignments
- `clusters` Cluster parameters
- `weights` The cluster weights, does not sum to `1`, but to `1` minus the weight of all uninstanistaed clusters.
- `iter_count` Timing for each iteration
- `nmi_score_history` NMI score per iteration(if gt suppled)
- `likelihood_history` Log likelihood per iteration.
- `cluster_count_history` Cluster counts per iteration.
- `sub_labels` Sub labels assignments

# Example:
```julia
julia > x, y, clusters = generate_gaussian_data(10000, 2, 6, 100.0)
...

julia > ret_values = fit(x, 10.0, iters = 100, verbose = false)

...

julia > unique(ret_values[1])
6 - element Array{ Int64,1 }:
3
6
1
2
5
4
```
"""
function fit(all_data::AbstractArray{ Float32,2 }, alpha_param::Float32;
iters::Int64 = 100, init_clusters::Int64 = 1, seed = nothing, verbose = true, save_model = false, burnout = 20, gt = nothing, max_clusters = Inf, outlier_weight = 0, outlier_params = nothing)
data_dim = size(all_data, 1)
cov_mat = Matrix{ Float32 }(I, data_dim, data_dim)
local_hyper_params = niw_hyperparams(1, zeros(Float32, data_dim), data_dim + 3, cov_mat)
dp_model, iter_count, nmi_score_history, liklihood_history, cluster_count_history = dp_parallel(all_data, local_hyper_params, alpha_param, iters, init_clusters, seed, verbose, save_model, burnout, gt, max_clusters, outlier_weight, outlier_params)
return Array(dp_model.group.labels), [x.cluster_params.cluster_params.distribution for x in dp_model.group.local_clusters], dp_model.group.weights, iter_count, nmi_score_history, liklihood_history, cluster_count_history, Array(dp_model.group.labels_subcluster)
end

fit(all_data::AbstractArray, alpha_param;
iters = 100, init_clusters = 1,
seed = nothing, verbose = true,
save_model = false, burnout = 20, gt = nothing, max_clusters = Inf, outlier_weight = 0, outlier_params = nothing) =
fit(Float32.(all_data), Float32(alpha_param), iters = Int64(iters),
	init_clusters = Int64(init_clusters), seed = seed, verbose = verbose,
	save_model = save_model, burnout = burnout, gt = gt, max_clusters = max_clusters, outlier_weight = outlier_weight, outlier_params = outlier_params)

	fit(all_data::AbstractArray, local_hyper_params::distribution_hyper_params, alpha_param;
iters = 100, init_clusters::Number = 1,
seed = nothing, verbose = true,
save_model = false, burnout = 20, gt = nothing, max_clusters = Inf, outlier_weight = 0, outlier_params = nothing) =
fit(Float32.(all_data), local_hyper_params, Float32(alpha_param), iters = Int64(iters),
	init_clusters = Int64(init_clusters), seed = seed, verbose = verbose,
	save_model = save_model, burnout = burnout, gt = gt, max_clusters = max_clusters, outlier_weight = outlier_weight, outlier_params = outlier_params)




	*/

/*
	dp_parallel(model_params::String; verbose = true, save_model = true, burnout = 5, gt = nothing)

	Run the model in advanced mode.
	# Args and Kwargs
	- `model_params::String` A path to a parameters file(see below)
	- `verbose` will perform prints on every iteration.
	- `save_model` will save a checkpoint every `X` iterations, where `X` is specified in the parameter file.
	- `burnout` how long to wait after creating a cluster, and allowing it to split / merge
	- `gt` Ground truth, when supplied, will perform NMI and VI analysis on every iteration.

	# Return values
	dp_model, iter_count, nmi_score_history, liklihood_history, cluster_count_history
	- `dp_model` The DPMM model inferred
	- `iter_count` Timing for each iteration
	- `nmi_score_history` NMI score per iteration(if gt suppled)
	- `likelihood_history` Log likelihood per iteration.
	- `cluster_count_history` Cluster counts per iteration.
	*/
ModelInfo dp_parallel_sampling_class::dp_parallel(global_params *globalParamsIn, std::string model_params)
{
	if (globalParams != NULL)
	{
		delete globalParams;
	}
	globalParams = globalParamsIn;
	dp_parallel_sampling* dp_model = init_model_from_file();
	
	//TODO - not in Julia:
	globalParams->outlier_mod = 0;
	prior *pPrior = NULL;
	init_first_clusters(dp_model, globalParams->initial_clusters, &pPrior);
	if (globalParams->use_verbose)
	{
		printf("Node Leaders:");
	}
	return run_model(pPrior, dp_model, 1, model_params.c_str());
}

ModelInfo dp_parallel_sampling_class::run_model(prior *pPrior, dp_parallel_sampling* &dp_model, int first_iter, const char* model_params, std::chrono::steady_clock::time_point prev_time)
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
		lca.group_step(pPrior, modelInfo.dp_model->group, no_more_splits, final, i == 1);

		double iter_time = (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - prev_time).count()) / 1000.0;
		modelInfo.iter_count.push_back(iter_time);

		modelInfo.cluster_count_history.push_back(modelInfo.dp_model->group.local_clusters.size());
		//std::string fileName = "run_model" + std::to_string(i) + ".png";

		//draw::CreatePng(fileName.c_str(), modelInfo.dp_model.group.points, modelInfo.dp_model.group.labels);
		if (globalParams->ground_truth.size() > 0)
		{
			//TODO - OR - what does it mean?
			//v_score_history.push_back(varinfo(Int.(ground_truth), modelInfo.dp_model.group.labels.col(0))));
			//push!(modelInfo.nmi_score_history, mutualinfo(Int.(ground_truth), modelInfo.dp_model.group.labels.col(0), normed = true));
		}
		else
		{
			v_score_history.push_back("no gt");
			modelInfo.nmi_score_history.push_back("no gt");
		}
		if (globalParams->use_verbose)
		{
			modelInfo.likelihood_history.push_back(calculate_posterior(pPrior, modelInfo.dp_model));
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

void dp_parallel_sampling_class::save_model(dp_parallel_sampling* &model,std::string path, std::string filename,long  iter, long long total_time, const char * global_params)
{
	std::string fileName = path + filename + "_" + std::to_string(iter) + ".jld2";
	model_hyper_params hyperparams = model->model_hyperparams;
	pts_less_group* group = ds::create_pts_less_group(model->group);
	std::ofstream myfile(fileName);
	myfile << group << hyperparams << iter << total_time << global_params;
	myfile.close();
}


double dp_parallel_sampling_class::calculate_posterior(prior *pPrior, dp_parallel_sampling* &model)
{
	//TODO - should be logabsgamma <= abs
	double log_posterior = r8_gamma_log(model->model_hyperparams.alpha) - r8_gamma_log(model->group.points.cols() + model->model_hyperparams.alpha);
	for (ClusterIndexType i = 0; i < model->group.local_clusters.size(); ++i)
	{
		local_cluster *cluster = model->group.local_clusters[i];
		if (cluster->cluster_params->cluster_params->suff_statistics->N == 0)
		{
			continue;
		}

		log_posterior += pPrior->log_marginal_likelihood(cluster->cluster_params->cluster_params->prior_hyperparams,
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
/*
	"""
	cluster_statistics(points, labels, clusters)

	Provide avg statsitcs of probabiliy and likelihood for given points, labels and clusters

	# Args and Kwargs
	- `points` a `DxN` array containing the data
	- `labels` points labels
	- `clusters` vector of clusters distributions


	# Return values
	avg_ll, avg_prob
	- `avg_ll` each cluster avg point ll
	- `avg_prob` each cluster avg point prob


	# Example:
```julia
julia > dp = run_model_from_checkpoint("checkpoint__50.jld2")
Loading Model :
1.073261 seconds(2.27 M allocations : 113.221 MiB, 2.60% gc time)
Including params
Loading data :
0.000881 seconds(10.02 k allocations : 378.313 KiB)
Creating model :
Node Leaders :
Dict{ Any,Any }(2 = > Any[2, 3])
Running model :
...
```
"""
function cluster_statistics(points, labels, clusters)
parr = zeros(Float32, length(labels), length(clusters))
tic = time()
for (k, cluster) in enumerate(clusters)
log_likelihood!(reshape((@view parr[:, k]), :, 1), points, cluster)
end
log_likelihood_array = copy(parr)
log_likelihood_array[isnan.(log_likelihood_array)] . = -Inf #Numerical errors arent fun
max_log_prob_arr = maximum(log_likelihood_array, dims = 2)
log_likelihood_array . -= max_log_prob_arr
map!(exp, log_likelihood_array, log_likelihood_array)
# println("lsample log cat2" * string(log_likelihood_array))
sum_prob_arr = sum(log_likelihood_array, dims = [2])
log_likelihood_array . /= sum_prob_arr
avg_ll = zeros(length(clusters))
avg_prob = zeros(length(clusters))
for i = 1:length(clusters)
avg_ll[i] = sum(parr[labels . == i, i]) / sum(labels . == i)
avg_prob[i] = sum(log_likelihood_array[labels . == i, i]) / sum(labels . == i)
end
return avg_ll, avg_prob
end
*/