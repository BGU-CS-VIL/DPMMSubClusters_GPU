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
	~global_params();

	void init(std::string modelParamsFileName, prior_type priorType);
	void init(int numLabels, MatrixXd& all_data, unsigned long long randomSeed, prior_type priorType);
	void init_prior(prior_type priorType);
	void init_random(unsigned long long randomSeed);

	std::unique_ptr<prior> pPrior;

	//Data Loading specifics
	std::string data_path = "/path/to/data/";
	std::string data_prefix = "data_prefix"; //If the data file name is bob.npy, this should be 'bob'

	//Model Parameters
	int iterations = 100;
	bool hard_clustering = false;//Soft or hard assignments
	int initial_clusters = 1;
	int argmax_sample_stop = 5;//Change to hard assignment from soft at iterations - argmax_sample_stop
	int split_stop = 5;//Stop split / merge moves at  iterations - split_stop

	unsigned long long random_seed;
	std::unique_ptr<std::random_device> rd;
	std::unique_ptr<std::mt19937> gen;

	int max_split_iter = 20;
	int burnout_period = 20;
	double max_clusters = DBL_MAX;

	//Model hyperparams
	double alpha = 10.0;
	std::shared_ptr<hyperparams> hyper_params = NULL;
	double outlier_mod = 0.05;
	std::shared_ptr<hyperparams> outlier_hyper_params = NULL;

	//Saving specifics :
	bool enable_saving = true;
	int model_save_interval = 1000;
	std::string save_path = "/path/to/save/dir/";
	bool overwrite_prec = false;
	std::string save_file_prefix = "checkpoint_";

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

	LabelsType* ground_truth = NULL;
};