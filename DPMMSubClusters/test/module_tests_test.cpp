#include "pch.h"
#include "gtest/gtest.h"
#include "priors/niw.h"
#include "priors/multinomial_prior.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"
#include "module_tests.h"

namespace DPMMSubClustersTest
{
	TEST(module_tests_test, RandomMess)
	{
		module_tests mt;
		ClusterIndexType numClusters = mt.RandomMess();

		EXPECT_TRUE(numClusters > 1);

		std::string str = "Found: " + std::to_string(numClusters) + " clusters";
		std::cout << str << std::endl;
	}

	TEST(module_tests_test, TestingData)
	{
		module_tests mt;
		std::vector<double> labels;
		mt.TestingData(labels);

		EXPECT_EQ((size_t)50000, labels.size());
	}

	TEST(module_tests_test, RandomMess10Clusters)
	{
		srand(12345);
		data_generators data_generators;
		MatrixXd  x;
		std::vector<double> labels;
		float** tmean;
		float** tcov;
		int N = pow(10, 5);
		int D = 2;
		int numClusters = 10;
		int numIters = 100;

		data_generators.generate_gaussian_data(N, D, numClusters, 100.0, x, labels, tmean, tcov);

		niw_hyperparams hyper_params(1.0, VectorXd::Zero(D), 5, MatrixXd::Identity(D, D));

		dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		ModelInfo dp = dps.dp_parallel(&hyper_params, N, numIters, 1, true, true, false, 15, labels);
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

		for (DimensionsType i = 0; i < D; i++)
		{
			delete[] tmean[i];
		}
		for (DimensionsType i = 0; i < D; i++)
		{
			delete[] tcov[i];
		}

		EXPECT_TRUE(dp.dp_model->group.local_clusters.size() > 1);

		std::string str = "Found: " + std::to_string(dp.dp_model->group.local_clusters.size()) + " clusters";
		std::cout << str << std::endl;
		std::cout << "Time:" << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[seconds]" << std::endl;

		EXPECT_TRUE(numClusters > 6);
	}

	TEST(module_tests_test, DISABLED_ReadPnyFileIntoData)
	{
		module_tests mt;
		MatrixXd data;
		mt.ReadPnyFileIntoData("mnm_data.npy", "", data);

		EXPECT_EQ(100, (int)data.rows());
		EXPECT_EQ(1000, (int)data.cols());
		EXPECT_EQ(1, (int)data(0, 2));
	}

	TEST(module_tests_test, MultinomialModel)
	{
		srand(12345);
		data_generators data_generators;
		int  trials = 50;
		LabelsType labels;
		MatrixXd clusters;
		int N = pow(10, 5);
		int D = 2;
		int numClusters = 2;
		MatrixXd  x;
		VectorXd alpha(2);
		alpha << 2.0, 3.0;
		int numIters = 100;

		data_generators.generate_mnmm_data(N, D, numClusters, trials, x, labels, clusters);

		multinomial_hyper hyper_params(alpha);

		dp_parallel_sampling_class dps(N, x, 0, prior_type::Multinomial);
		ModelInfo dp = dps.dp_parallel(&hyper_params, 2.0, numIters, 1, true, true, false, 15);

		EXPECT_EQ(2, dp.dp_model->group.local_clusters.size());
	}

	//Good for testing performance for bottlenecks 
	TEST(module_tests_test, HighDim)
	{
		srand(12345);
		data_generators data_generators;
		MatrixXd  x;
		std::vector<double> labels;
		float** tmean;
		float** tcov;
		int N = pow(10, 5);
		int D = 32;
		int numClusters = 20;
		int numIters = 200;

		data_generators.generate_gaussian_data(N, D, numClusters, 100.0, x, labels, tmean, tcov);

		niw_hyperparams hyper_params(1.0, VectorXd::Zero(D), 32, MatrixXd::Identity(D, D));

		dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		ModelInfo dp = dps.dp_parallel(&hyper_params, N, numIters, 1, false, false, false, 15, labels);
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

		for (DimensionsType i = 0; i < D; i++)
		{
			delete[] tmean[i];
		}
		for (DimensionsType i = 0; i < D; i++)
		{
			delete[] tcov[i];
		}

		EXPECT_TRUE(dp.dp_model->group.local_clusters.size() > 6);

		std::string str = "Found: " + std::to_string(dp.dp_model->group.local_clusters.size()) + " clusters";
		std::cout << str << std::endl;
		std::cout << "Time:" << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[seconds]" << std::endl;
		std::cout << "Time:" << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[microseconds]" << std::endl;
	}

	TEST(module_tests_test, CompareTiming)
	{
		srand(12345);
		data_generators data_generators;
		MatrixXd  x;
		std::vector<double> labels;
		float** tmean;
		float** tcov;
		int N = pow(10, 5);
		int D = 2;
		int numClusters = 20;
		int numIters = 200;

		data_generators.generate_gaussian_data(N, D, numClusters, 100.0, x, labels, tmean, tcov);

		niw_hyperparams hyper_params(1.0, VectorXd::Zero(D), 5, MatrixXd::Identity(D, D));

		dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		ModelInfo dp = dps.dp_parallel(&hyper_params, N, numIters, 1, false, false, false, 15, labels);
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

		for (DimensionsType i = 0; i < D; i++)
		{
			delete[] tmean[i];
		}
		for (DimensionsType i = 0; i < D; i++)
		{
			delete[] tcov[i];
		}

		EXPECT_TRUE(dp.dp_model->group.local_clusters.size() > 1);

		std::string str = "Found: " + std::to_string(dp.dp_model->group.local_clusters.size()) + " clusters";
		std::cout << str << std::endl;
		std::cout << "Time:" << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[seconds]" << std::endl;
		EXPECT_TRUE(numClusters > 6);
	}
}