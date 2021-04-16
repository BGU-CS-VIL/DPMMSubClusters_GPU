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
		std::shared_ptr<niw_hyperparams> prior = std::make_shared<niw_hyperparams>(1.0, m, 5.0, psi);
		std::shared_ptr<hyperparams> posterior = prior->clone();
		MatrixXd points(2, 10);
		points << -1.5580771, -1.8853954, -1.7104398, -0.8927666, 12.372295, 11.839552, 5.025442, 5.809215, -23.470015, -24.226057, 4.9351344, 4.256654, 5.218151, 4.0470524, 6.8685904, 7.0217624, 1.4795746, 1.9674032, -4.5789967, -5.0106354;

		std::shared_ptr<sufficient_statistics> resultBase = object.create_sufficient_statistics(prior, posterior, points);
		niw_sufficient_statistics* result = dynamic_cast<niw_sufficient_statistics*>(resultBase.get());

		ASSERT_EQ(10, result->N);
		ASSERT_NEAR(-18.6962, result->points_sum(0), 0.0001);
		ASSERT_NEAR(26.2046, result->points_sum(1), 0.0001);
		ASSERT_NEAR(1499.6991, result->S(0, 0), 0.0001);
		ASSERT_NEAR(387.5832, result->S(0, 1), 0.0001);
		ASSERT_NEAR(387.5832, result->S(1, 0), 0.0001);
		ASSERT_NEAR(234.69856, result->S(1, 1), 0.0001);
	}

	TEST(niw_test, CalcPosterior)
	{
		niw object;
		VectorXd m(2);
		m << 0.0, 0.0;
		MatrixXd psi(2, 2);
		psi << 1.0, 0.0, 0.0, 1.0;
		std::shared_ptr<niw_hyperparams> prior = std::make_shared<niw_hyperparams>(1.0, m, 5.0, psi);
		VectorXd points_sum(2);
		points_sum << -350136.8831217289, 59746.60001641512;
		MatrixXd S(2, 2);
		S << 4.793410496828892e6, -343002.59694666235, -343002.59694666235, 206659.1114939735;

		std::shared_ptr<niw_sufficient_statistics> ss = std::make_shared<niw_sufficient_statistics>(37196, points_sum, S);
		std::shared_ptr<hyperparams> resultBase = object.calc_posterior(prior, ss);
		niw_hyperparams* result = dynamic_cast<niw_hyperparams*>(resultBase.get());

		ASSERT_NEAR(40.255979361037014, result->psi(0, 0), 0.0001);
		ASSERT_NEAR(5.897545600162242, result->psi(0, 1), 0.0001);
		ASSERT_NEAR(5.897545600162242, result->psi(1, 0), 0.0001);
		ASSERT_NEAR(2.975669253988425, result->psi(1, 1), 0.0001);
	}

	TEST(niw_test, CreateHyperparams)
	{
		std::string strJson = "{\"m\":[1.0,2.0],\"psi\":[[1.0,0.0],[0.0,1.0]],\"k\":1,\"v\":5.0}";
		Json::Value root;
		Json::Reader reader;
		reader.parse(strJson.c_str(), root);

		niw object;
		std::shared_ptr<hyperparams> resultBase = object.create_hyperparams(root);
		niw_hyperparams* result = dynamic_cast<niw_hyperparams*>(resultBase.get());

		ASSERT_EQ(2, result->m.size());
		ASSERT_EQ(1.0, result->m(0));
		ASSERT_EQ(2.0, result->m(1));
		ASSERT_EQ(2, result->psi.rows());
		ASSERT_EQ(2, result->psi.cols());
		ASSERT_EQ(1.0, result->psi(0, 0));
		ASSERT_EQ(0.0, result->psi(0, 1));
		ASSERT_EQ(0.0, result->psi(1, 0));
		ASSERT_EQ(1.0, result->psi(1, 1));
		ASSERT_EQ(1, result->k);
		ASSERT_EQ(5.0, result->v);
	}

	TEST(niw_test, CreateHyperparamsNoM)
	{
		std::string strJson = "{\"psi\":[[1.0,0.0],[0.0,1.0]],\"k\":1,\"v\":5.0}";
		Json::Value root;
		Json::Reader reader;
		reader.parse(strJson.c_str(), root);

		niw object;
		std::shared_ptr<hyperparams> resultBase = object.create_hyperparams(root);
		niw_hyperparams* result = dynamic_cast<niw_hyperparams*>(resultBase.get());

		ASSERT_EQ(0, result->m.size());
		ASSERT_EQ(2, result->psi.rows());
		ASSERT_EQ(2, result->psi.cols());
		ASSERT_EQ(1.0, result->psi(0, 0));
		ASSERT_EQ(0.0, result->psi(0, 1));
		ASSERT_EQ(0.0, result->psi(1, 0));
		ASSERT_EQ(1.0, result->psi(1, 1));
		ASSERT_EQ(1, result->k);
		ASSERT_EQ(5.0, result->v);
	}

	TEST(niw_test, CreateHyperparamsNoPsi)
	{
		std::string strJson = "{\"m\":[1.0,2.0],\"k\":1,\"v\":5.0}";
		Json::Value root;
		Json::Reader reader;
		reader.parse(strJson.c_str(), root);

		niw object;
		std::shared_ptr<hyperparams> resultBase = object.create_hyperparams(root);
		niw_hyperparams* result = dynamic_cast<niw_hyperparams*>(resultBase.get());

		ASSERT_EQ(2, result->m.size());
		ASSERT_EQ(1.0, result->m(0));
		ASSERT_EQ(2.0, result->m(1));
		ASSERT_EQ(0, result->psi.rows());
		ASSERT_EQ(0, result->psi.cols());
		ASSERT_EQ(1, result->k);
		ASSERT_EQ(5.0, result->v);
	}

	TEST(niw_test, CreateHyperparamsNoK)
	{
		std::string strJson = "{\"m\":[1.0,2.0],\"psi\":[[1.0,0.0],[0.0,1.0]],\"v\":5.0}";
		Json::Value root;
		Json::Reader reader;
		reader.parse(strJson.c_str(), root);

		niw object;
		std::shared_ptr<hyperparams> resultBase = object.create_hyperparams(root);
		niw_hyperparams* result = dynamic_cast<niw_hyperparams*>(resultBase.get());

		ASSERT_EQ(2, result->m.size());
		ASSERT_EQ(1.0, result->m(0));
		ASSERT_EQ(2.0, result->m(1));
		ASSERT_EQ(2, result->psi.rows());
		ASSERT_EQ(2, result->psi.cols());
		ASSERT_EQ(1.0, result->psi(0, 0));
		ASSERT_EQ(0.0, result->psi(0, 1));
		ASSERT_EQ(0.0, result->psi(1, 0));
		ASSERT_EQ(1.0, result->psi(1, 1));
		ASSERT_EQ(0, result->k);
		ASSERT_EQ(5.0, result->v);
	}

	TEST(niw_test, CreateHyperparamsNoV)
	{
		std::string strJson = "{\"m\":[1.0,2.0],\"psi\":[[1.0,0.0],[0.0,1.0]],\"k\":1}";
		Json::Value root;
		Json::Reader reader;
		reader.parse(strJson.c_str(), root);

		niw object;
		std::shared_ptr<hyperparams> resultBase = object.create_hyperparams(root);
		niw_hyperparams* result = dynamic_cast<niw_hyperparams*>(resultBase.get());

		ASSERT_EQ(2, result->m.size());
		ASSERT_EQ(1.0, result->m(0));
		ASSERT_EQ(2.0, result->m(1));
		ASSERT_EQ(2, result->psi.rows());
		ASSERT_EQ(2, result->psi.cols());
		ASSERT_EQ(1.0, result->psi(0, 0));
		ASSERT_EQ(0.0, result->psi(0, 1));
		ASSERT_EQ(0.0, result->psi(1, 0));
		ASSERT_EQ(1.0, result->psi(1, 1));
		ASSERT_EQ(1, result->k);
		ASSERT_EQ(0.0, result->v);
	}
}