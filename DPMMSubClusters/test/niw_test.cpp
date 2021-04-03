#include "pch.h"
#include "gtest/gtest.h"
#include "priors/niw.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"
//#include "myNiw.h"

namespace DPMMSubClustersTest
{
	TEST(niw_test, CreateSufficientStatistics)
	{
		niw object;
		VectorXd m(2);
		m << 0.0, 0.0;
		MatrixXd psi(2, 2);
		psi << 1.0, 0.0, 0.0, 1.0;
		niw_hyperparams* prior = new niw_hyperparams(1.0, m, 5.0, psi);
		hyperparams* posterior = prior->clone();
		MatrixXd points(2, 10);
		points << -1.5580771, -1.8853954, -1.7104398, -0.8927666, 12.372295, 11.839552, 5.025442, 5.809215, -23.470015, -24.226057, 4.9351344, 4.256654, 5.218151, 4.0470524, 6.8685904, 7.0217624, 1.4795746, 1.9674032, -4.5789967, -5.0106354;

		sufficient_statistics* resultBase = object.create_sufficient_statistics(prior, posterior, points);
		niw_sufficient_statistics* result = (niw_sufficient_statistics*)resultBase;

		delete prior;
		delete posterior;

		ASSERT_EQ(10, result->N);
		ASSERT_NEAR(-18.6962, result->points_sum(0), 0.0001);
		ASSERT_NEAR(26.2046, result->points_sum(1), 0.0001);
		ASSERT_NEAR(1499.6991, result->S(0, 0), 0.0001);
		ASSERT_NEAR(387.5832, result->S(0, 1), 0.0001);
		ASSERT_NEAR(387.5832, result->S(1, 0), 0.0001);
		ASSERT_NEAR(234.69856, result->S(1, 1), 0.0001);

		delete result;
	}
}