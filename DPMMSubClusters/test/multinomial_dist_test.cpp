#include "pch.h"
#include "gtest/gtest.h"
#include "priors/niw.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"
#include "distributions/multinomial_dist.h"
#include "cudaKernel_multinomial.cuh"

namespace DPMMSubClustersTest
{

	TEST(multinomial_dist_test, log_likelihood)
	{
		cudaKernel_multinomial cuda;
		multinomial_dist object;
		MatrixXd x(2, 10);
		x << 38.0, 42.0, 40.0, 36.0, 14.0, 11.0, 9.0, 8.0, 5.0, 8.0, 12.0, 8.0, 10.0, 14.0, 36.0, 39.0, 41.0, 42.0, 45.0, 42.0;
		VectorXd r(10);
		std::vector<double> alpha;
		alpha.push_back(-0.8658322);
		alpha.push_back(-0.54593706);
		multinomial_dist* dist = new multinomial_dist(alpha);

		cuda.log_likelihood(r, x, dist);

		delete dist;

		EXPECT_NEAR(-39.452866, r(0), 0.0001);
		EXPECT_NEAR(-40.73245, r(1), 0.0001);
		EXPECT_NEAR(-40.09266, r(2), 0.0001);
		EXPECT_NEAR(-38.81308, r(3), 0.0001);
		EXPECT_NEAR(-31.775385, r(4), 0.0001);
		EXPECT_NEAR(-30.8157, r(5), 0.0001);
		EXPECT_NEAR(-30.175909, r(6), 0.0001);
		EXPECT_NEAR(-29.856014, r(7), 0.0001);
		EXPECT_NEAR(-28.896328, r(8), 0.0001);
		EXPECT_NEAR(-29.856014, r(9), 0.0001);

	}
}