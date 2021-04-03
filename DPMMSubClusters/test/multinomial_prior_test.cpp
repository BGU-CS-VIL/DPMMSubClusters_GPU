#include "pch.h"
#include "gtest/gtest.h"
#include "priors/multinomial_prior.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"
#include "distributions/multinomial_dist.h"
#include "myCudaKernel.h"

namespace DPMMSubClustersTest
{
	TEST(multinomial_prior_test, CreateSufficientStatistics)
	{
		MatrixXd points(2, 10);
		points << -1.5580771, -1.8853954, -1.7104398, -0.8927666, 12.372295, 11.839552, 5.025442, 5.809215, -23.470015, -24.226057, 4.9351344, 4.256654, 5.218151, 4.0470524, 6.8685904, 7.0217624, 1.4795746, 1.9674032, -4.5789967, -5.0106354;
		myCudaKernel_multinomial* myCudaKernelObj = new myCudaKernel_multinomial();
		global_params* gp = new global_params(10, points, NULL, prior_type::Multinomial);
		gp->cuda = myCudaKernelObj;
		multinomial_prior object;
		VectorXd alpha(2);
		alpha << 0.0, 0.0;
		multinomial_hyper* prior = new multinomial_hyper(alpha);
		hyperparams* posterior = prior->clone();

		sufficient_statistics* resultBase = object.create_sufficient_statistics(prior, posterior, points);
		multinomial_sufficient_statistics* result = (multinomial_sufficient_statistics*)resultBase;

		delete prior;
		delete posterior;

		ASSERT_EQ(10, result->N);
		ASSERT_NEAR(-18.6962, result->points_sum(0), 0.0001);
		ASSERT_NEAR(26.2046, result->points_sum(1), 0.0001);

		delete resultBase;
	}

	TEST(multinomial_prior_test, SampleDistribution)
	{
		MatrixXd points;
		myCudaKernel_multinomial* myCudaKernelObj = new myCudaKernel_multinomial();
		global_params* gp = new global_params(10, points, NULL, prior_type::Multinomial);
		gp->cuda = myCudaKernelObj;
		multinomial_prior object;
		VectorXd alpha(2);
		alpha << 23601.0, 904.0;
		multinomial_hyper* prior = new multinomial_hyper(alpha);

		distribution_sample* distribution_sampleBase = object.sample_distribution(prior, gp->gen);
		multinomial_dist* distribution_sample = (multinomial_dist*)distribution_sampleBase;

		delete prior;

		ASSERT_NEAR(-0.03687498683001788, distribution_sample->alpha[0], 0.1);
		ASSERT_NEAR(-3.3186026586706987, distribution_sample->alpha[1], 0.1);

		delete distribution_sampleBase;
	}

	TEST(multinomial_prior_test, LogMarginalLikelihood)
	{
		MatrixXd points;
		myCudaKernel_multinomial* myCudaKernelObj = new myCudaKernel_multinomial();
		global_params* gp = new global_params(10, points, NULL, prior_type::Multinomial);
		gp->cuda = myCudaKernelObj;
		multinomial_prior object;
		VectorXd alpha_prior(2);
		alpha_prior << 2.0, 3.0;
		multinomial_hyper* prior = new multinomial_hyper(alpha_prior);
		VectorXd alpha_posterior(2);
		alpha_posterior << 1791.0, 24064.0;
		multinomial_hyper* posterior = new multinomial_hyper(alpha_posterior);
		VectorXd points_sum(2);
		points_sum << 1789.0, 24061.0;

		multinomial_sufficient_statistics* ss = new multinomial_sufficient_statistics(517.0, points_sum);

		double result = object.log_marginal_likelihood(prior, posterior, ss);

		delete prior;
		delete posterior;

		ASSERT_NEAR(-6509.2656, result, 0.01);
	}

	TEST(multinomial_prior_test, AggregateSuffStats)
	{
		MatrixXd points;
		myCudaKernel_multinomial* myCudaKernelObj = new myCudaKernel_multinomial();
		global_params* gp = new global_params(10, points, NULL, prior_type::Multinomial);
		gp->cuda = myCudaKernelObj;
		multinomial_prior object;
		VectorXd points_l_sum(2);
		points_l_sum << 3119.0, 22031.0;
		VectorXd points_r_sum(2);
		points_r_sum << 20849.0, 4001.0;
		VectorXd points_sum(2);

		sufficient_statistics* suff_l = new multinomial_sufficient_statistics(10, points_l_sum);
		sufficient_statistics* suff_r = new multinomial_sufficient_statistics(12, points_r_sum);
		sufficient_statistics* suff_out = new multinomial_sufficient_statistics(0, points_sum);
		object.aggregate_suff_stats(suff_l, suff_r, suff_out);

		ASSERT_EQ(22, suff_out->N);
		ASSERT_NEAR(23968.0, suff_out->points_sum(0), 0.001);
		ASSERT_NEAR(26032.0, suff_out->points_sum(1), 0.001);

		delete suff_l;
		delete suff_r;
		delete suff_out;
	}
}