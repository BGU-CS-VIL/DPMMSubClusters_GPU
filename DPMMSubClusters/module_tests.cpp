#include "module_tests.h"
#include "priors/niw.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include "draw.h"
#include "moduleTypes.h"
#include "utils.h"
#include "distributions_util/logdet.h"
#include "niw_hyperparams.h"

using namespace std;

ClusterIndexType module_tests::RandomMess()
{
	printf("Testing Module (Random mess)");
	srand(12345);
	data_generators data_generators;
	MatrixXd  x;
	std::shared_ptr<LabelsType> labels = std::make_shared<LabelsType>();
	double** tmean;
	double** tcov;
	int N = (int)pow(10, 5);
	int D = 2;
	int numClusters = 2;
	int numIters = 100;

	data_generators.generate_gaussian_data(N, D, numClusters, 100.0, x, labels, tmean, tcov);
	draw::draw_labels("Expected", x, labels);

	std::shared_ptr<hyperparams> hyper_params = std::make_shared<niw_hyperparams>(1.0, VectorXd::Zero(D), 5, MatrixXd::Identity(D, D));

	dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);
	ModelInfo dp = dps.dp_parallel(hyper_params, N, numIters, 1, true, false, false, 15, labels);

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
	std::shared_ptr<LabelsType> labels = std::make_shared<LabelsType>();
	double** tmean;
	double** tcov;
	int N = (int)pow(10, 5);
	int D = 2;
	int numClusters = 10;
	int numIters = 200;
	int foundClusters = 0;
	std::string fileName = "RandomMessHighDim_" + std::to_string(N) + "_" + std::to_string(D) + "_" + std::to_string(numClusters);

	struct stat buffer;
	if (stat((fileName + ".npy").c_str(), &buffer) == 0)
	{
		utils::load_data_model(fileName, x);
		if (stat((fileName + ".labels").c_str(), &buffer) == 0)
		{
			utils::load_data_labels(fileName, labels);
		}
	}
	else
	{
		data_generators.generate_gaussian_data(N, D, numClusters, 100.0, x, labels, tmean, tcov);
		for (DimensionsType i = 0; i < D; i++)
		{
			delete[] tmean[i];
		}
		for (DimensionsType i = 0; i < D; i++)
		{
			delete[] tcov[i];
		}
	}
	
	std::vector<int> count(numClusters);
	for (size_t i = 0; i < N; i++)
	{
		count[(*labels)[i] - 1] += 1;
	}

	std::shared_ptr<hyperparams> hyper_params = std::make_shared<niw_hyperparams>(1.0, VectorXd::Zero(D), D, MatrixXd::Identity(D, D));

	dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	ModelInfo dp = dps.dp_parallel(hyper_params, N, numIters, 1, true, false, false, 15, labels);
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	printf("Found: %lld clusters\n", dp.dp_model->group.local_clusters.size());
	printf("Time:%lld[seconds]\n", std::chrono::duration_cast<std::chrono::seconds>(end - begin).count());

	foundClusters = (int)dp.dp_model->group.local_clusters.size();

	return foundClusters;
}

void module_tests::TestingData(std::shared_ptr<LabelsType> &labels)
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

void module_tests::CheckMemoryLeak()
{
	srand(12345);
	data_generators data_generators;
	MatrixXd  x;
	std::shared_ptr<LabelsType> labels = std::make_shared<LabelsType>();
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

		printf("Test %ld: Found: %lld clusters\n", i, dp.dp_model->group.local_clusters.size());
		printf("Time:%lld[seconds]\n", std::chrono::duration_cast<std::chrono::seconds>(end - begin).count());
	}
}

ClusterIndexType module_tests::RandomMessAllParams()
{
	printf("Testing Module (Random mess all params)");
	srand(12345);
	data_generators data_generators;
	MatrixXd  x;
	std::shared_ptr<LabelsType> labels = std::make_shared<LabelsType>();
	double** tmean;
	double** tcov;
	int N = 10000;
	int D = 2;
	int numClusters = 2;
	int numIters = 100;

	data_generators.generate_gaussian_data(N, D, numClusters, 100.0, x, labels, tmean, tcov);
	draw::draw_labels("Expected", x, labels);

	std::shared_ptr<hyperparams> hyper_params = std::make_shared<niw_hyperparams>(1.0, VectorXd::Zero(D), 5, MatrixXd::Identity(D, D));

	dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);
	ModelInfo dp = dps.dp_parallel(hyper_params, N, numIters, 1, true, false, false, 15, nullptr, 10, 0.05, std::make_shared<niw_hyperparams>(1.0, VectorXd::Zero(2), 5, MatrixXd::Identity(2, 2)));

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