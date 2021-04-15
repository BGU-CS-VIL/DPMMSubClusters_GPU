#include "module_tests.h"
#include "priors/niw.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include <string>
#include "draw.h"
#include "moduleTypes.h"
#include "utils.h"
#include "distributions_util/logdet.h"

using namespace std;

ClusterIndexType module_tests::RandomMess()
{
	printf("Testing Module (Random mess)");
	srand(12345);
	data_generators data_generators;
	MatrixXd  x;
	std::vector<int> labels;
	double** tmean;
	double** tcov;
	int N = (int)pow(10, 5);
	//int N =2;
	int D = 2;
	int numClusters = 2;
	int numIters = 100;

	data_generators.generate_gaussian_data(N, D, numClusters, 100.0, x, labels, tmean, tcov);
	draw::draw_labels("Expected", x, labels);

	std::shared_ptr<hyperparams> hyper_params = std::make_shared<niw_hyperparams>(1.0, VectorXd::Zero(D), 5, MatrixXd::Identity(D, D));

	dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);
	ModelInfo dp = dps.dp_parallel(hyper_params, N, numIters, 1, true, false, false, 15);

	for (DimensionsType i = 0; i < D; i++)
	{
		delete[] tmean[i];
	}
	for (DimensionsType i = 0; i < D; i++)
	{
		delete[] tcov[i];
	}
	return (ClusterIndexType)(dp.dp_model->group.local_clusters.size());
}

ClusterIndexType module_tests::RandomMessHighDim()
{
	srand(12345);
	data_generators data_generators;
	MatrixXd  x;
	LabelsType labels;
	double** tmean;
	double** tcov;
	int N = (int)pow(10, 5);
	int D = 16;
	//int D = 2;
	int numClusters = 20;
	int numIters = 200;
	int foundClusters = 0;

	data_generators.generate_gaussian_data(N, D, numClusters, 100.0, x, labels, tmean, tcov);
	for (DimensionsType i = 0; i < D; i++)
	{
		delete[] tmean[i];
	}
	for (DimensionsType i = 0; i < D; i++)
	{
		delete[] tcov[i];
	}

	std::shared_ptr<hyperparams> hyper_params = std::make_shared<niw_hyperparams>(1.0, VectorXd::Zero(D), D, MatrixXd::Identity(D, D));

	dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	ModelInfo dp = dps.dp_parallel(hyper_params, N, numIters, 1, true, false, false, 15);
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::string str = "Found: " + std::to_string(dp.dp_model->group.local_clusters.size()) + " clusters";
	std::cout << str << std::endl;
	std::cout << "Time:" << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[seconds]" << std::endl;
	foundClusters = (int)dp.dp_model->group.local_clusters.size();

	return foundClusters;
}

void module_tests::TestingData(LabelsType &labels)
{
	printf("Testing data");
	srand(12345);
	data_generators data_generators;
	MatrixXd  x;
	double** tmean;
	double** tcov;
	int N = 50000;
	int D = 2;
	data_generators.generate_gaussian_data(N, D, 5, 30, x, labels, tmean, tcov);
	draw::draw_labels("Expected", x, labels);

	for (DimensionsType i = 0; i < D; i++)
	{
		delete[] tmean[i];
	}
	for (DimensionsType i = 0; i < D; i++)
	{
		delete[] tcov[i];
	}
}

void module_tests::ReadPnyFileIntoData(std::string path, std::string prefix, MatrixXd &mat)
{
	utils::load_data(path, prefix, mat);
}

ClusterIndexType module_tests::RunModuleFromFile(std::string path, std::string prefix)
{
	printf("Testing Module (Run Module From File)");
	int D = 2;
	int N = 10;//TBD
	unsigned long long random_seed = 0;
	MatrixXd x;

	dp_parallel_sampling_class dps(N, x, random_seed, prior_type::Gaussian);
	std::vector<double> gt;
	std::shared_ptr<global_params> globalParams = std::make_shared<global_params>(N, x, random_seed, prior_type::Gaussian);
	globalParams->initial_clusters = 1;
	globalParams->use_verbose = true;
	globalParams->draw_labels = false;
	globalParams->should_save_model = false;
	globalParams->burnout_period = 20;
	globalParams->max_clusters = DBL_MAX;
	globalParams->data_path = path;
	globalParams->data_prefix = prefix;
	globalParams->hyper_params = std::make_shared<niw_hyperparams>(1.0, VectorXd::Zero(D), 5, MatrixXd::Identity(D, D));


	ModelInfo dp = dps.dp_parallel(globalParams, "");

	return (ClusterIndexType)dp.dp_model->group.local_clusters.size();
}

void module_tests::CheckMemoryLeak()
{
	srand(12345);
	data_generators data_generators;
	MatrixXd  x;
	LabelsType labels;
	double** tmean;
	double** tcov;
	int N = (int)pow(10, 5);
	int D = 2;
	int numClusters = 10;
	int numIters = 100;

	data_generators.generate_gaussian_data(N, D, numClusters, 100.0, x, labels, tmean, tcov);
	for (DimensionsType i = 0; i < D; i++)
	{
		delete[] tmean[i];
	}
	for (DimensionsType i = 0; i < D; i++)
	{
		delete[] tcov[i];
	}

	for (int i = 0; i < 100; i++)
	{
		std::shared_ptr<hyperparams> hyper_params = std::make_shared<niw_hyperparams>(1.0, VectorXd::Zero(D), 5, MatrixXd::Identity(D, D));

		dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		ModelInfo dp = dps.dp_parallel(hyper_params, N, numIters, 1, false, false, false, 15);
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

		std::string str = "Test " + std::to_string(i) + ": Found: " + std::to_string(dp.dp_model->group.local_clusters.size()) + " clusters";
		std::cout << str << std::endl;
		std::cout << "Time:" << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[seconds]" << std::endl;
	}
}