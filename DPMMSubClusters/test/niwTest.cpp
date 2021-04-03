#include "CppUnitTest.h"
#include "priors\niw.h"
#include "Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"
#include "myNiw.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DPMMSubClustersTest
{
	
	TEST_CLASS(niw_unit_tests)
	{
	public:

		TEST_METHOD(CreateSufficientStatistics)
		{
			niw object;
			VectorXd m(2);
			m << 0.0, 0.0;
			MatrixXd psi(2,2);
			psi << 1.0, 0.0, 0.0, 1.0;
			niw_hyperparams *prior = new niw_hyperparams(1.0,m,5.0,psi);
			hyperparams *posterior = prior->clone();
			MatrixXd points(2, 10);
			points << -1.5580771, -1.8853954, -1.7104398, -0.8927666, 12.372295, 11.839552, 5.025442, 5.809215, -23.470015, -24.226057, 4.9351344, 4.256654, 5.218151, 4.0470524, 6.8685904, 7.0217624, 1.4795746, 1.9674032, -4.5789967, -5.0106354;

			sufficient_statistics *resultBase = object.create_sufficient_statistics(prior, posterior, points);
			niw_sufficient_statistics *result = (niw_sufficient_statistics*)resultBase;

			delete prior;
			delete posterior;

			Assert::AreEqual(10, result->N);
			Assert::AreEqual(-18.6962, result->points_sum(0), 0.0001);
			Assert::AreEqual(26.2046, result->points_sum(1), 0.0001);
			Assert::AreEqual(1499.6991, result->S(0, 0), 0.0001);
			Assert::AreEqual(387.5832, result->S(0, 1), 0.0001);
			Assert::AreEqual(387.5832, result->S(1, 0), 0.0001);
			Assert::AreEqual(234.69856, result->S(1, 1), 0.0001);

			delete result;
		}

		TEST_METHOD(calc_posterior)
		{
			niw object;
			VectorXd m(2);
			m << 0.0, 0.0;
			MatrixXd psi(2, 2);
			psi << 1.0, 0.0, 0.0, 1.0;
			niw_hyperparams *prior = new niw_hyperparams(1.0, m, 5.0, psi);
			VectorXd points_sum(2);
			points_sum << -16.383741, -24.094425;
			MatrixXd S(2, 2);
			S << 178.89845, 259.0389, 259.0389, 388.9143;

			sufficient_statistics *ss = new niw_sufficient_statistics(3, points_sum, S);
			hyperparams* posteriorBase = object.calc_posterior(prior, ss);
			niw_hyperparams *result = (niw_hyperparams*)posteriorBase;

			delete prior;
			delete ss;

			Assert::AreEqual(4.0, result->k);
			Assert::AreEqual(-4.0959353, result->m(0), 0.0001);
			Assert::AreEqual(-6.0236063, result->m(1), 0.0001);
			Assert::AreEqual(8.0, result->v, 0.0001);
			Assert::AreEqual(14.598964, result->psi(0, 0), 0.0001);
			Assert::AreEqual(20.043713, result->psi(0, 1), 0.0001);
			Assert::AreEqual(20.043713, result->psi(1, 0), 0.0001);
			Assert::AreEqual(31.097372, result->psi(1, 1), 0.0001);

			delete result;
		}

		TEST_METHOD(log_marginal_likelihood)
		{
			niw object;
			VectorXd m(2);
			m << 0.0, 0.0;
			MatrixXd psi(2, 2);
			psi << 1.0, 0.0, 0.0, 1.0;
			niw_hyperparams *prior = new niw_hyperparams(1.0, m, 5.0, psi);
			psi << 10.05719, 3.4761395, 3.4761395, 2.7664554;
			niw_hyperparams *posterior = new niw_hyperparams(10.0, m, 14.0, psi);
			VectorXd points_sum(2);
			MatrixXd S(2, 2);
			S << 178.89845, 259.0389, 259.0389, 388.9143;

			sufficient_statistics *ss = new niw_sufficient_statistics(9, points_sum, S);
			double result = object.log_marginal_likelihood(prior, posterior, ss);

			delete prior;
			delete posterior;
			delete ss;

			Assert::AreEqual(-48.84072476678828, result, 0.0001);
		}

		TEST_METHOD(sample_distribution)
		{
			myNiw object;
			VectorXd m(2);
			m << -1.7396386, -6.2887034;
			MatrixXd psi(2, 2);
			psi << 3.5362449, 0.26511383, 0.26511383, 12.226074;
			niw_hyperparams *prior = new niw_hyperparams(9.0, m, 13.0, psi);

			distibution_sample *resultBase = object.sample_distribution(prior);
			mv_gaussian *result = (mv_gaussian*)resultBase;

			delete prior;

			Assert::AreEqual(-2.6763540674250113, result->mu(0), 0.0001);
			Assert::AreEqual(-6.373344698686574, result->mu(1), 0.0001);
			Assert::AreEqual(8.402266651681197, result->sigma(0, 0), 0.0001);
			Assert::AreEqual(-0.09345429531760577, result->sigma(0, 1), 0.0001);
			Assert::AreEqual(-0.09345429531760577, result->sigma(1, 0), 0.0001);
			Assert::AreEqual(12.197528108693033, result->sigma(1, 1), 0.0001);
			Assert::AreEqual(0.11902564706733308, result->invSigma(0, 0), 0.0001);
			Assert::AreEqual(0.0009119436226977912, result->invSigma(0, 1), 0.0001);
			Assert::AreEqual(0.0009119436226977912, result->invSigma(1, 0), 0.0001);
			Assert::AreEqual(0.08199081126411832, result->invSigma(1, 1), 0.0001);
			Assert::AreEqual(4.629649604686476, result->logdetSigma, 0.0001);

			delete result;
		}
	};
}