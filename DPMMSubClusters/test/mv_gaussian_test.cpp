#include "pch.h"
#include "gtest/gtest.h"
#include "priors/niw.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"
#include "myNiw.h"
#include "cudaKernel_gaussian.cuh"

namespace DPMMSubClustersTest
{
	TEST(mv_gaussian_unit_tests, log_likelihood)
	{
		MatrixXd x(2, 10);
		x << 7.111513, 6.5072656, 7.656911, 7.021403, 6.395694, -14.513991, -16.44812, -15.126435, 11.43946, 5.0940423, 2.884489, 3.4449763, 2.4345767, 1.152309, 3.076971, -5.085796, -5.293715, -5.330345, 9.191501, -5.4205728;
		cudaKernel_gaussian cuda;
		cuda.init(10, x, 12345);
		mv_gaussian object;
		VectorXd r(10);
		VectorXd mu(2);
		mu << -8.180686, -2.1489322;
		MatrixXd sigma(2, 2);
		sigma << 110.386635, 57.80892, 57.80892, 31.78561;
		MatrixXd invSigma(2, 2);
		invSigma << 0.19052099, -0.3465031, -0.3465031, 0.66165066;
		double logdetSigma = 5.117007f;
		LLT<MatrixXd, Upper> invChol;

		mv_gaussian* gaussian = new mv_gaussian(mu, sigma, invSigma, logdetSigma, invChol);

		cuda.log_likelihood(r, x, gaussian);

		delete gaussian;

		EXPECT_NEAR(-10.221556, r(0), 0.0001);
		EXPECT_NEAR(-8.667738, r(1), 0.0001);
		EXPECT_NEAR(-11.925285, r(2), 0.0001);
		EXPECT_NEAR(-14.465169, r(3), 0.0001);
		EXPECT_NEAR(-9.114409, r(4), 0.0001);
		EXPECT_NEAR(-6.4636755, r(5), 0.0001);
		EXPECT_NEAR(-7.0082736, r(6), 0.0001);
		EXPECT_NEAR(-6.52158, r(7), 0.0001);
		EXPECT_NEAR(-8.3534565, r(8), 0.0001);
		EXPECT_NEAR(-41.610622, r(9), 0.0001);
	}

	TEST(mv_gaussian_unit_tests, log_likelihood_sigma_is_identity)
	{
		MatrixXd x(2, 10);
		x << 7.111513, 6.5072656, 7.656911, 7.021403, 6.395694, -14.513991, -16.44812, -15.126435, 11.43946, 5.0940423, 2.884489, 3.4449763, 2.4345767, 1.152309, 3.076971, -5.085796, -5.293715, -5.330345, 9.191501, -5.4205728;
		cudaKernel_gaussian cuda;
		cuda.init(10, x, 12345);
		mv_gaussian object;
		VectorXd r1(10);
		VectorXd r2(10);
		VectorXd mu(2);
		mu << -8.180686, -2.1489322;
		MatrixXd sigma(2, 2);
		sigma << 1, 0, 0, 1;
		MatrixXd invSigma(2, 2);
		invSigma << 1, 0, 0, 1;
		double logdetSigma = 5.117007f;
		LLT<MatrixXd, Upper> invChol;

		mv_gaussian* gaussian = new mv_gaussian(mu, sigma, invSigma, logdetSigma, invChol);

		cuda.log_likelihood(r1, x, gaussian);

		MatrixXd z = x.colwise() - gaussian->mu;
		r2 = (z.cwiseProduct(gaussian->invSigma * z)).colwise().sum();
		double scalar = -((gaussian->sigma.size() * log(2 * EIGEN_PI) + gaussian->logdetSigma) / 2);
		r2 = scalar * VectorXd::Ones(r2.size()) - r2 / 2;

		delete gaussian;

		for (int i = 0; i < 10; i++)
		{
			EXPECT_NEAR(r2(i), r1(i), 0.0001);
		}
	}
}