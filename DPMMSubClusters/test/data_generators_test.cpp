#include "pch.h"
#include "gtest/gtest.h"
#include "priors/niw.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"

namespace DPMMSubClustersTest
{
	TEST(data_generators_test, generate_mnmm_data)
	{
		srand(12345);
		data_generators data_generators;
		int  trials = 100;
		LabelsType labels;
		MatrixXd clusters;
		int N = 10;
		int D = 4;
		int numClusters = 2;
		MatrixXd  x;

		data_generators.generate_mnmm_data(N, D, numClusters, trials, x, labels, clusters);

		EXPECT_EQ(D, x.rows());
		EXPECT_EQ(N, x.cols());
		EXPECT_EQ(N, labels.size());
		for (LabelsType::iterator iter = labels.begin(); iter < labels.end(); iter++)
		{
			EXPECT_TRUE(*iter == 1 || *iter == 2);
		}
		EXPECT_EQ(D, clusters.rows());
		EXPECT_EQ(numClusters, clusters.cols());
		ASSERT_NEAR(1.0, clusters.col(0).sum(), 0.001);
		ASSERT_NEAR(1.0, clusters.col(1).sum(), 0.001);

	}
}
