#include "pch.h"
#include "gtest/gtest.h"
#include "CppUnitTest.h"
#include "priors/niw.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"
#include "utils.h"

namespace DPMMSubClustersTest
{
	TEST(utils_test, CreateSufficientStatistics)
	{
		utils object;
		VectorXd m(2);
		m << 0.0, 0.0;
		MatrixXd psi(2, 2);
		psi << 1.0, 0.0, 0.0, 1.0;
		niw_hyperparams* prior = new niw_hyperparams(1.0, m, 5.0, psi);
		hyperparams* posterior = prior->clone();
		MatrixXd points(2, 10);
		points << -1.5580771, -1.8853954, -1.7104398, -0.8927666, 12.372295, 11.839552, 5.025442, 5.809215, -23.470015, -24.226057, 4.9351344, 4.256654, 5.218151, 4.0470524, 6.8685904, 7.0217624, 1.4795746, 1.9674032, -4.5789967, -5.0106354;

		sufficient_statistics* resultBase;
		object.create_sufficient_statistics(prior, &resultBase, NULL, points);
		niw_sufficient_statistics* result = (niw_sufficient_statistics*)resultBase;

		delete prior;
		delete posterior;

		EXPECT_EQ(10, result->N);
		EXPECT_NEAR(-18.6962, result->points_sum(0), 0.0001);
		EXPECT_NEAR(26.2046, result->points_sum(1), 0.0001);
		EXPECT_NEAR(1499.6991, result->S(0, 0), 0.0001);
		EXPECT_NEAR(387.5832, result->S(0, 1), 0.0001);
		EXPECT_NEAR(387.5832, result->S(1, 0), 0.0001);
		EXPECT_NEAR(234.69856, result->S(1, 1), 0.0001);

		delete result;
	}

	TEST(utils_test, DcolwiseDot)
	{
		utils object;
		VectorXd r(10);
		MatrixXd a(2, 10);
		MatrixXd b(2, 10);
		a << 0.54004717, 0.37134647, 0.71275353, -0.15549898, 0.2896006, -0.48544192, 0.334553, -1.117856, 0.82345843, 0.3604319, -1.5424032, -0.6355591, -1.0986004, 0.5384531, -1.294733, 0.3186283, 0.86913586, 2.4165726, -1.8343678, -1.1392164;
		b << 0.24554309, 0.58407843, 1.2395499, 0.021466652, -0.3251773, -1.2639452, 1.9347299, -1.2664667, 0.87995815, 0.056602255, -0.30097237, 0.02137096, 0.10616897, 0.13734035, -0.41260248, -0.30309504, 0.793751, 0.20607561, -0.1809317, -0.25985837;
		object.dcolwise_dot(r, a, b);

		EXPECT_NEAR(0.5968256, r(0), 0.001);
		EXPECT_NEAR(0.20331295, r(1), 0.001);
		EXPECT_NEAR(0.76685625, r(2), 0.001);
		EXPECT_NEAR(0.070613295, r(3), 0.001);
		EXPECT_NEAR(0.44003853, r(4), 0.001);
		EXPECT_NEAR(0.51699734, r(5), 0.001);
		EXPECT_NEAR(1.3371472, r(6), 0.001);
		EXPECT_NEAR(1.9137242, r(7), 0.001);
		EXPECT_NEAR(1.0565042, r(8), 0.001);
		EXPECT_NEAR(0.31643617, r(9), 0.001);

	}
}