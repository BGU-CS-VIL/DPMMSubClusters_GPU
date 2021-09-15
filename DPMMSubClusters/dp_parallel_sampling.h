#pragma once
#include "global_params.h"
#include <chrono>
#include <vector>
#include <cfloat>
#include "ds.h"
#include "modelInfo.h"

using namespace Eigen;


class dp_parallel_sampling_class
{
public:
	dp_parallel_sampling_class(int numLabels, MatrixXd& all_data, unsigned long long randomSeed, prior_type priorType);
	dp_parallel_sampling_class(std::string modelDataFileName, std::string modelParamsFileName, prior_type priorType);

	ModelInfo dp_parallel(
		std::shared_ptr<hyperparams>& local_hyper_params,
		double alpha_param,
		IterationIndexType iters = 100,
		ClusterIndexType init_clusters = 1,
		bool verbose = true,
		bool draw_labels = false,
		bool save_model = false,
		int burnout = 15,
		std::shared_ptr<LabelsType> gt = nullptr,
		double max_clusters = DBL_MAX,
		double outlier_weight = 0,
		std::shared_ptr<hyperparams> outlier_params = nullptr);

	ModelInfo dp_parallel_from_file();

	ModelInfo init_and_run_model(MatrixXd& all_data);

	std::shared_ptr<dp_parallel_sampling> init_model(MatrixXd& all_data);
	void init_first_clusters(std::shared_ptr<dp_parallel_sampling>& dp_model, ClusterIndexType initial_cluster_count);
	ModelInfo run_model(std::shared_ptr<dp_parallel_sampling>& dp_model, int first_iter, const char* model_params = NULL, std::chrono::steady_clock::time_point prev_time = std::chrono::steady_clock::now());
	double calculate_posterior(std::shared_ptr<dp_parallel_sampling>& model);
	void save_model(std::shared_ptr<dp_parallel_sampling>& model, std::string path, std::string filename, long  iter, long long total_time, const char* global_params);

private:
	std::shared_ptr<global_params> globalParams;
};
