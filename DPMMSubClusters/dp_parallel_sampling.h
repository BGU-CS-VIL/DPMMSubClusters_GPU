#pragma once
#include <chrono>
#include <vector>
#include "Eigen/Dense"
#include "ds.h"
#include "global_params.h"

using namespace Eigen;

struct ModelInfo
{
	ModelInfo() : dp_model(NULL) {}
	~ModelInfo()
	{
		if (dp_model != NULL)
		{
			delete dp_model;
			dp_model = NULL;
		}
	}

	//	- `dp_model` The DPMM model inferred
	dp_parallel_sampling *dp_model;
	//`iter_count` Timing for each iteration
	std::vector<double> iter_count;
	//	- `nmi_score_history` NMI score per iteration(if gt suppled)
	std::vector<std::string> nmi_score_history;
	//	- `likelihood_history` Log likelihood per iteration.
	std::vector<double> likelihood_history;

	//	- `cluster_count_history` Cluster counts per iteration.
	std::vector<ClusterIndexType> cluster_count_history;
};

class dp_parallel_sampling_class
{
public:
	dp_parallel_sampling_class(int numLabels, MatrixXd& all_data, unsigned long long randomSeed, prior_type priorType)
	{
		globalParams = new global_params(numLabels, all_data, randomSeed, priorType);
	}

	~dp_parallel_sampling_class()
	{
		if (globalParams != NULL)
		{
			delete globalParams;
			globalParams = NULL;
		}
	}

	ModelInfo dp_parallel(global_params *globalParamsIn, std::string model_params);

	ModelInfo dp_parallel(
		hyperparams* local_hyper_params,
		double alpha_param,
		IterationIndexType iters = 100,
		ClusterIndexType init_clusters = 1,
		bool verbose = true,
		bool draw_labels = false,
		bool save_model = false,
		int burnout = 15,
		std::vector<double> gt = std::vector<double> {},
		double max_clusters = DBL_MAX,
		double outlier_weight = 0,
		hyperparams* outlier_params = NULL);
	
	dp_parallel_sampling* init_model_from_file();
	dp_parallel_sampling* init_model_from_data(MatrixXd &all_data);
	void init_first_clusters(dp_parallel_sampling* &dp_model, ClusterIndexType initial_cluster_count, prior **pPrior);
	void set_parr_worker(LabelType numLabels, int cluster_count);
	ModelInfo run_model(prior *pPrior, dp_parallel_sampling* &dp_model, int first_iter, const char* model_params = NULL, std::chrono::steady_clock::time_point prev_time = std::chrono::steady_clock::now());
	double calculate_posterior(prior *pPrior, dp_parallel_sampling* &model);
	void save_model(dp_parallel_sampling* &model, std::string path, std::string filename, long  iter, long long total_time, const char * global_params);
	
private:
	global_params *globalParams;
}; 
