#pragma once
#include <random>
#include <string>
#include <vector>
#include "Eigen/Dense"
#include "ds.h"
#include "cudaKernel.cuh"

using namespace Eigen;

class prior;

enum prior_type
{
	Gaussian,
	Multinomial
};

class global_params
{
public:
	global_params(int numLabels, MatrixXd& all_data, unsigned long long randomSeed, prior_type priorType);
	~global_params();

	std::unique_ptr<prior> pPrior;

	//Data Loading specifics
	std::string data_path;
	std::string data_prefix;


	//Model Parameters
	int iterations;
	bool hard_clustering;
	int initial_clusters;
	int argmax_sample_stop;
	int split_stop;

	unsigned long long random_seed;
	std::unique_ptr<std::random_device> rd;

	std::unique_ptr<std::mt19937> gen;

	int max_split_iter;
	int burnout_period;
	double max_clusters;

	//Model hyperparams
	double alpha;
	std::shared_ptr<hyperparams> hyper_params;
	double outlier_mod;
	std::shared_ptr<hyperparams> outlier_hyper_params;

	//Saving specifics :
	bool enable_saving;
	int model_save_interval;
	std::string save_path;
	bool overwrite_prec;
	std::string save_file_prefix;

	bool use_verbose;
	bool draw_labels;
	bool should_save_model;
	double max_num_of_clusters;
	MatrixXd glob_parr;

	std::vector<std::shared_ptr<thin_cluster_params>> clusters_vector;
	std::vector<double> clusters_weights;

	std::unique_ptr<cudaKernel> cuda;
	int numLabels;
	MatrixXd points;
};