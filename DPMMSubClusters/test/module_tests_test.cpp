#include "pch.h"
#include "gtest/gtest.h"
#include "priors/niw.h"
#include "priors/multinomial_prior.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"
#include "module_tests.h"
#include "priors/niw_hyperparams.h"
#include "priors/niw_sufficient_statistics.h"
#include "priors/multinomial_hyper.h"
#include "priors/multinomial_sufficient_statistics.h"

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
		LabelsType labels;
		mt.TestingData(labels);

		EXPECT_EQ((size_t)50000, labels.size());
	}

	TEST(module_tests_test, RandomMess10Clusters)
	{
		int actualNumClusters = 0;
		const int Tries = 3;
		int i = 0;
		do {
			++i;
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
			//		utils::save_data("niw_data_2D", x);
			std::shared_ptr<hyperparams> hyper_params = std::make_shared<niw_hyperparams>(1.0, VectorXd::Zero(D), 5, MatrixXd::Identity(D, D));

			dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);

			std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
			ModelInfo dp = dps.dp_parallel(hyper_params, N, numIters, 1, true, true, false, 15);
			std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

			for (DimensionsType i = 0; i < D; i++)
			{
				delete[] tmean[i];
			}
			for (DimensionsType i = 0; i < D; i++)
			{
				delete[] tcov[i];
			}

			actualNumClusters = dp.dp_model->group.local_clusters.size();

			std::string str = "Found: " + std::to_string(dp.dp_model->group.local_clusters.size()) + " clusters";
			std::cout << str << std::endl;
			std::cout << "Time:" << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[seconds]" << std::endl;

		} while (actualNumClusters <= 6 && i < Tries);
		EXPECT_TRUE(actualNumClusters > 6);
	}

	TEST(module_tests_test, MultinomialModel)
	{
		srand(12345);
		data_generators data_generators;
		int  trials = 50;
		LabelsType labels;
		MatrixXd clusters;
		int N = (int)pow(10, 5);
		int D = 2;
		int numClusters = 2;
		MatrixXd  x;
		VectorXd alpha(2);
		alpha << 2.0, 3.0;
		int numIters = 100;

		data_generators.generate_mnmm_data(N, D, numClusters, trials, x, labels, clusters);

		std::shared_ptr<hyperparams> hyper_params = std::make_shared<multinomial_hyper>(alpha);

		dp_parallel_sampling_class dps(N, x, 0, prior_type::Multinomial);
		ModelInfo dp = dps.dp_parallel(hyper_params, 2.0, numIters, 1, true, true, false, 15);

		EXPECT_EQ(2, dp.dp_model->group.local_clusters.size());
	}

	//Good for testing performance for bottlenecks 
	TEST(module_tests_test, DISABLED_HighDim)
	{
		srand(12345);
		data_generators data_generators;
		MatrixXd  x;
		LabelsType labels;
		double** tmean;
		double** tcov;
		int N = (int)pow(10, 5);
		int D = 32;
		int numClusters = 20;
		int numIters = 200;

		data_generators.generate_gaussian_data(N, D, numClusters, 100.0, x, labels, tmean, tcov);

		std::shared_ptr<hyperparams> hyper_params = std::make_shared<niw_hyperparams>(1.0, VectorXd::Zero(D), 32, MatrixXd::Identity(D, D));

		dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		ModelInfo dp = dps.dp_parallel(hyper_params, N, numIters, 1, false, false, false, 15);
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

	TEST(module_tests_test, DISABLED_VeryHighDim)
	{
		srand(12345);
		data_generators data_generators;
		MatrixXd  x;
		LabelsType labels;
		double** tmean;
		double** tcov;
		int N = (int)pow(10, 5);
		int D = 1000;
		int numClusters = 3;
		int numIters = 100;

		data_generators.generate_gaussian_data(N, D, numClusters, 100.0, x, labels, tmean, tcov);

		std::shared_ptr<hyperparams> hyper_params = std::make_shared<niw_hyperparams>(1.0, VectorXd::Zero(D), 1000, MatrixXd::Identity(D, D));

		dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		ModelInfo dp = dps.dp_parallel(hyper_params, N, numIters, 1, false, false, false, 15);
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

		ASSERT_FALSE(true);
	}

	TEST(module_tests_test, CompareTiming)
	{
		srand(12345);
		data_generators data_generators;
		MatrixXd  x;
		LabelsType labels;
		double** tmean;
		double** tcov;
		int N = (int)pow(10, 5);
		int D = 2;
		int numClusters = 20;
		int numIters = 200;

		data_generators.generate_gaussian_data(N, D, numClusters, 100.0, x, labels, tmean, tcov);

		std::shared_ptr<hyperparams> hyper_params = std::make_shared<niw_hyperparams>(1.0, VectorXd::Zero(D), 5, MatrixXd::Identity(D, D));

		dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		ModelInfo dp = dps.dp_parallel(hyper_params, N, numIters, 1, false, false, false, 15);
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

		for (DimensionsType i = 0; i < D; i++)
		{
			delete[] tmean[i];
		}
		for (DimensionsType i = 0; i < D; i++)
		{
			delete[] tcov[i];
		}

		std::string str = "Found: " + std::to_string(dp.dp_model->group.local_clusters.size()) + " clusters";
		std::cout << str << std::endl;
		std::cout << "Time:" << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[seconds]" << std::endl;
		EXPECT_TRUE(dp.dp_model->group.local_clusters.size() > 6);
	}

	TEST(module_tests_test, RunModuleFromFile1)
	{
		printf("Testing Module (Run Module From File)");

		dp_parallel_sampling_class dps("mnm_data", "ModuleTestFile1.json", prior_type::Multinomial);
		ModelInfo dp = dps.dp_parallel_from_file();

		EXPECT_TRUE(dp.dp_model->group.local_clusters.size() > 6);
	}

	TEST(module_tests_test, RunModuleFromFile2WithOutlier_hyper)
	{
		printf("Testing Module (Run Module From File)");

		dp_parallel_sampling_class dps("niw_data_2D", "ModuleTestFile2.json", prior_type::Gaussian);
		ModelInfo dp = dps.dp_parallel_from_file();

		EXPECT_TRUE(dp.dp_model->group.local_clusters.size() > 1);
	}

	TEST(module_tests_test, RunModuleFromFile3WithoutOutlier_hyper)
	{
		printf("Testing Module (Run Module From File)");

		dp_parallel_sampling_class dps("niw_data_2D", "ModuleTestFile3.json", prior_type::Gaussian);
		ModelInfo dp = dps.dp_parallel_from_file();

		EXPECT_TRUE(dp.dp_model->group.local_clusters.size() > 6);
	}
}