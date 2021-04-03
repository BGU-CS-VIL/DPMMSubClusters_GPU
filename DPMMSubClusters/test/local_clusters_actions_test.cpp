#include "pch.h"
#include "gtest/gtest.h"
#include "priors/niw.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"
#include "local_clusters_actions.h"
#include "myCudaKernel.h"
#include "myGen.h"


namespace DPMMSubClustersTest
{
	TEST(local_clusters_actions_test, update_suff_stats_posterior)
	{
		MatrixXd points(2, 10);
		points << 2.3863995, -4.672651, -2.3842435, -2.2775807, -0.695474, -2.408718, -3.7753334, -6.659004, 16.301395, -15.971787, 8.34453, -5.352129, -5.805547, -5.2975364, -5.7748003, -5.324071, -5.524739, 6.970031, -10.184864, -1.799381;
		myCudaKernel_gaussian* myCudaKernelObj = new myCudaKernel_gaussian();
		myCudaKernelObj->init(10, points, NULL);
		global_params* gp = new global_params(10, points, NULL, prior_type::Gaussian);
		gp->cuda = myCudaKernelObj;
		local_clusters_actions object(gp);
		VectorXd m(2);
		m << 0.0, 0.0;
		MatrixXd psi(2, 2);
		psi << 1.0, 0.0, 0.0, 1.0;
		niw* prior = new niw();
		std::vector<int> labels = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
		std::vector<int> sub_labels = { 1, 1, 2, 1, 2, 1, 2, 1, 2, 2 };
		myCudaKernelObj->set_labels(labels);
		myCudaKernelObj->set_sub_labels(sub_labels);

		//sufficient_statistics *resultBase;
		local_group group;
		niw_hyperparams niwHyperparams(1.0, m, 5.0, psi);
		group.model_hyperparams.distribution_hyper_params = &niwHyperparams;
		group.model_hyperparams.alpha = 10;
		group.model_hyperparams.total_dim = 10;
		group.points = points;

		local_cluster* localCluster = new local_cluster();
		localCluster->cluster_params = new splittable_cluster_params();
		//Both
		localCluster->cluster_params->cluster_params = new cluster_parameters();
		localCluster->cluster_params->cluster_params->prior_hyperparams = niwHyperparams.clone();
		mv_gaussian* mvGaussian = new mv_gaussian();
		mvGaussian->mu = VectorXd(2);
		mvGaussian->mu << -0.5753647, 0.8155203;
		mvGaussian->sigma = MatrixXd(2, 2);
		mvGaussian->sigma << 0.5354316, 0.3868427, 0.3868427, 2.543111;
		mvGaussian->invSigma = MatrixXd(2, 2);
		mvGaussian->invSigma << 2.0982506, -0.31917322, -0.31917322, 0.44176984;
		mvGaussian->logdetSigma = 0.19228421;
		//1.4485339889930742 -0.22034223288786076; 0.0 0.6270718778453257: LLT<MatrixXd, Upper> invChol;
		localCluster->cluster_params->cluster_params->distribution = mvGaussian;
		niw_sufficient_statistics* suff_statistics = new niw_sufficient_statistics();
		suff_statistics->N = 10.0;
		suff_statistics->points_sum = VectorXd(2);
		suff_statistics->points_sum << 0.0, 0.0;
		suff_statistics->S = MatrixXd(2, 2);
		suff_statistics->S << 0.0, 0.0, 0.0, 0.0;
		localCluster->cluster_params->cluster_params->suff_statistics = suff_statistics;
		localCluster->cluster_params->cluster_params->posterior_hyperparams = niwHyperparams.clone();
		//Left
		localCluster->cluster_params->cluster_params_l = new cluster_parameters();
		localCluster->cluster_params->cluster_params_l->prior_hyperparams = niwHyperparams.clone();
		mvGaussian = new mv_gaussian();
		mvGaussian->mu = VectorXd(2);
		mvGaussian->mu << -0.5753647, 0.8155203;
		mvGaussian->sigma = MatrixXd(2, 2);
		mvGaussian->sigma << 0.5354316, 0.3868427, 0.3868427, 2.543111;
		mvGaussian->invSigma = MatrixXd(2, 2);
		mvGaussian->invSigma << 2.0982506, -0.31917322, -0.31917322, 0.44176984;
		mvGaussian->logdetSigma = 0.19228421;
		//1.4485339889930742 -0.22034223288786076; 0.0 0.6270718778453257: LLT<MatrixXd, Upper> invChol;
		localCluster->cluster_params->cluster_params_l->distribution = mvGaussian;
		suff_statistics = new niw_sufficient_statistics();
		suff_statistics->N = 5.0;
		suff_statistics->points_sum = VectorXd(2);
		suff_statistics->points_sum << 0.0, 0.0;
		suff_statistics->S = MatrixXd(2, 2);
		suff_statistics->S << 0.0, 0.0, 0.0, 0.0;
		localCluster->cluster_params->cluster_params_l->suff_statistics = suff_statistics;
		localCluster->cluster_params->cluster_params_l->posterior_hyperparams = niwHyperparams.clone();
		//Right
		localCluster->cluster_params->cluster_params_r = new cluster_parameters();
		localCluster->cluster_params->cluster_params_r->prior_hyperparams = niwHyperparams.clone();
		mvGaussian = new mv_gaussian();
		mvGaussian->mu = VectorXd(2);
		mvGaussian->mu << -0.5753647, 0.8155203;
		mvGaussian->sigma = MatrixXd(2, 2);
		mvGaussian->sigma << 0.5354316, 0.3868427, 0.3868427, 2.543111;
		mvGaussian->invSigma = MatrixXd(2, 2);
		mvGaussian->invSigma << 2.0982506, -0.31917322, -0.31917322, 0.44176984;
		mvGaussian->logdetSigma = 0.19228421;
		//1.4485339889930742 -0.22034223288786076; 0.0 0.6270718778453257: LLT<MatrixXd, Upper> invChol;
		localCluster->cluster_params->cluster_params_r->distribution = mvGaussian;
		suff_statistics = new niw_sufficient_statistics();
		suff_statistics->N = 0.0;
		suff_statistics->points_sum = VectorXd(2);
		suff_statistics->points_sum << 0.0, 0.0;
		suff_statistics->S = MatrixXd(2, 2);
		suff_statistics->S << 0.0, 0.0, 0.0, 0.0;
		localCluster->cluster_params->cluster_params_r->suff_statistics = suff_statistics;
		localCluster->cluster_params->cluster_params_r->posterior_hyperparams = niwHyperparams.clone();
		localCluster->cluster_params->lr_weights = { 0.5,0.5 };
		localCluster->cluster_params->splittable = false;
		group.local_clusters.push_back(localCluster);

		object.update_suff_stats_posterior(prior, group);

		delete prior;

		//Both
		EXPECT_NEAR(10, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params->suff_statistics))->N, 0.0001);
		EXPECT_NEAR(-20.156998, localCluster->cluster_params->cluster_params->suff_statistics->points_sum(0), 0.0001);
		EXPECT_NEAR(-29.748505, localCluster->cluster_params->cluster_params->suff_statistics->points_sum(1), 0.0001);
		EXPECT_NEAR(624.1152, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params->suff_statistics))->S(0, 0), 0.0001);
		EXPECT_NEAR(-75.174065, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params->suff_statistics))->S(0, 1), 0.0001);
		EXPECT_NEAR(-75.174065, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params->suff_statistics))->S(1, 0), 0.0001);
		EXPECT_NEAR(407.81204, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params->suff_statistics))->S(1, 1), 0.0001);
		EXPECT_NEAR(11, ((niw_hyperparams*)localCluster->cluster_params->cluster_params->posterior_hyperparams)->k, 0.0001);
		EXPECT_NEAR(-1.8324543, ((niw_hyperparams*)localCluster->cluster_params->cluster_params->posterior_hyperparams)->m(0), 0.0001);
		EXPECT_NEAR(-2.7044096, ((niw_hyperparams*)localCluster->cluster_params->cluster_params->posterior_hyperparams)->m(1), 0.0001);
		EXPECT_NEAR(15.0, ((niw_hyperparams*)localCluster->cluster_params->cluster_params->posterior_hyperparams)->v, 0.0001);
		EXPECT_NEAR(39.47856, ((niw_hyperparams*)localCluster->cluster_params->cluster_params->posterior_hyperparams)->psi(0, 0), 0.0001);
		EXPECT_NEAR(-8.645789, ((niw_hyperparams*)localCluster->cluster_params->cluster_params->posterior_hyperparams)->psi(0, 1), 0.0001);
		EXPECT_NEAR(-8.645789, ((niw_hyperparams*)localCluster->cluster_params->cluster_params->posterior_hyperparams)->psi(1, 0), 0.0001);
		EXPECT_NEAR(22.157326, ((niw_hyperparams*)localCluster->cluster_params->cluster_params->posterior_hyperparams)->psi(1, 1), 0.0001);

		//Left
		EXPECT_NEAR(5, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params_l->suff_statistics))->N, 0.0001);
		EXPECT_NEAR(-13.631555, localCluster->cluster_params->cluster_params_l->suff_statistics->points_sum(0), 0.0001);
		EXPECT_NEAR(-0.6591754, localCluster->cluster_params->cluster_params_l->suff_statistics->points_sum(1), 0.0001);
		EXPECT_NEAR(82.8602, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params_l->suff_statistics))->S(0, 0), 0.0001);
		EXPECT_NEAR(23.398304, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params_l->suff_statistics))->S(0, 1), 0.0001);
		EXPECT_NEAR(23.398304, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params_l->suff_statistics))->S(1, 0), 0.0001);
		EXPECT_NEAR(203.26741, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params_l->suff_statistics))->S(1, 1), 0.0001);
		EXPECT_NEAR(6, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_l->posterior_hyperparams)->k, 0.0001);
		EXPECT_NEAR(-2.2719257, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_l->posterior_hyperparams)->m(0), 0.0001);
		EXPECT_NEAR(-0.109862566, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_l->posterior_hyperparams)->m(1), 0.0001);
		EXPECT_NEAR(10.0, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_l->posterior_hyperparams)->v, 0.0001);
		EXPECT_NEAR(5.689032, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_l->posterior_hyperparams)->psi(0, 0), 0.0001);
		EXPECT_NEAR(2.1900706, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_l->posterior_hyperparams)->psi(0, 1), 0.0001);
		EXPECT_NEAR(2.1900706, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_l->posterior_hyperparams)->psi(1, 0), 0.0001);
		EXPECT_NEAR(20.8195, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_l->posterior_hyperparams)->psi(1, 1), 0.0001);

		//Right
		EXPECT_NEAR(5, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params_r->suff_statistics))->N, 0.0001);
		EXPECT_NEAR(-6.525443, localCluster->cluster_params->cluster_params_r->suff_statistics->points_sum(0), 0.0001);
		EXPECT_NEAR(-29.08933, localCluster->cluster_params->cluster_params_r->suff_statistics->points_sum(1), 0.0001);
		EXPECT_NEAR(541.25494, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params_r->suff_statistics))->S(0, 0), 0.0001);
		EXPECT_NEAR(-98.572365, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params_r->suff_statistics))->S(0, 1), 0.0001);
		EXPECT_NEAR(-98.572365, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params_r->suff_statistics))->S(1, 0), 0.0001);
		EXPECT_NEAR(204.54466, ((niw_sufficient_statistics*)(localCluster->cluster_params->cluster_params_r->suff_statistics))->S(1, 1), 0.0001);
		EXPECT_NEAR(6, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_r->posterior_hyperparams)->k, 0.0001);
		EXPECT_NEAR(-1.0875739, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_r->posterior_hyperparams)->m(0), 0.0001);
		EXPECT_NEAR(-4.848222, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_r->posterior_hyperparams)->m(1), 0.0001);
		EXPECT_NEAR(10.0, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_r->posterior_hyperparams)->v, 0.0001);
		EXPECT_NEAR(53.915802, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_r->posterior_hyperparams)->psi(0, 0), 0.0001);
		EXPECT_NEAR(-13.020917, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_r->posterior_hyperparams)->psi(0, 1), 0.0001);
		EXPECT_NEAR(-13.020917, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_r->posterior_hyperparams)->psi(1, 0), 0.0001);
		EXPECT_NEAR(6.8513136, ((niw_hyperparams*)localCluster->cluster_params->cluster_params_r->posterior_hyperparams)->psi(1, 1), 0.0001);

		delete gp;
	}

	TEST(local_clusters_actions_test, should_split_local)
	{
		MatrixXd points;
		myCudaKernel_gaussian* myCudaKernelObj = new myCudaKernel_gaussian();
		global_params* gp = new global_params(10, points, NULL, prior_type::Gaussian);
		gp->gen = new myGen();
		gp->cuda = myCudaKernelObj;
		local_clusters_actions object(gp);
		VectorXd m(2);
		m << 0.0, 0.0;
		MatrixXd psi(2, 2);
		psi << 1.0, 0.0, 0.0, 1.0;
		niw* prior = new niw();
		niw_hyperparams niwHyperparams(1.0, m, 5.0, psi);

		splittable_cluster_params* cluster_params = new splittable_cluster_params();
		//Both
		cluster_params->cluster_params = new cluster_parameters();
		cluster_params->cluster_params->prior_hyperparams = niwHyperparams.clone();
		mv_gaussian* mvGaussian = new mv_gaussian();
		mvGaussian->mu = VectorXd(2);
		mvGaussian->mu << 2.1533558, -3.9149745;
		mvGaussian->sigma = MatrixXd(2, 2);
		mvGaussian->sigma << 55.271187, -33.318756, -33.318756, 82.92532;
		mvGaussian->invSigma = MatrixXd(2, 2);
		mvGaussian->invSigma << 0.023875484, 0.009592986, 0.009592986, 0.01591343;
		mvGaussian->logdetSigma = 8.152843;
		cluster_params->cluster_params->distribution = mvGaussian;
		niw_sufficient_statistics* suff_statistics = new niw_sufficient_statistics();
		suff_statistics->N = 10.0;
		suff_statistics->points_sum = VectorXd(2);
		suff_statistics->points_sum << 39.708576, -13.978705;
		suff_statistics->S = MatrixXd(2, 2);
		suff_statistics->S << 710.89166, -82.26995, -82.26995, 839.88275;
		cluster_params->cluster_params->suff_statistics = suff_statistics;
		VectorXd m1(2);
		m1 << 3.6098707, -1.2707914;
		MatrixXd psi1(2, 2);
		psi1 << 38.16992, -2.1205754, -2.1205754, 55.141247;
		niw_hyperparams niwHyperparams1(11.0, m1, 15.0, psi1);
		cluster_params->cluster_params->posterior_hyperparams = niwHyperparams1.clone();

		//Left
		cluster_params->cluster_params_l = new cluster_parameters();
		cluster_params->cluster_params_l->prior_hyperparams = niwHyperparams.clone();
		mvGaussian = new mv_gaussian();
		mvGaussian->mu = VectorXd(2);
		mvGaussian->mu << 9.126653, -2.767673;
		mvGaussian->sigma = MatrixXd(2, 2);
		mvGaussian->sigma << 20.320665, -22.958864, -22.958864, 63.56363;
		mvGaussian->invSigma = MatrixXd(2, 2);
		mvGaussian->invSigma << 0.08313907, 0.030029414, 0.030029414, 0.026578741;
		mvGaussian->logdetSigma = 6.6392817;

		cluster_params->cluster_params_l->distribution = mvGaussian;
		suff_statistics = new niw_sufficient_statistics();
		suff_statistics->N = 5.0;
		suff_statistics->points_sum = VectorXd(2);
		suff_statistics->points_sum << 53.091637, -8.826571;
		suff_statistics->S = MatrixXd(2, 2);
		suff_statistics->S << 646.82837, -198.42963, -198.42963, 459.69742;
		cluster_params->cluster_params_l->suff_statistics = suff_statistics;
		VectorXd m2(2);
		m2 << 8.848606, -1.4710952;
		MatrixXd psi2(2, 2);
		psi2 << 18.204138, -12.032678, -12.032678, 45.17127;
		niw_hyperparams niwHyperparams2(6.0, m2, 10.0, psi2);
		cluster_params->cluster_params_l->posterior_hyperparams = niwHyperparams2.clone();

		//Right
		cluster_params->cluster_params_r = new cluster_parameters();
		cluster_params->cluster_params_r->prior_hyperparams = niwHyperparams.clone();
		mvGaussian = new mv_gaussian();
		mvGaussian->mu = VectorXd(2);
		mvGaussian->mu << -2.7964401, -1.7621384;
		mvGaussian->sigma = MatrixXd(2, 2);
		mvGaussian->sigma << 10.129945, 37.860752, 37.860752, 150.83704;
		mvGaussian->invSigma = MatrixXd(2, 2);
		mvGaussian->invSigma << 1.5955796, -0.40049744, -0.40049744, 0.10715627;
		mvGaussian->logdetSigma = 4.548963;

		cluster_params->cluster_params_r->distribution = mvGaussian;
		suff_statistics = new niw_sufficient_statistics();
		suff_statistics->N = 5.0;
		suff_statistics->points_sum = VectorXd(2);
		suff_statistics->points_sum << -13.3830595, -5.152134;
		suff_statistics->S = MatrixXd(2, 2);
		suff_statistics->S << 64.0633, 116.15965, 116.15965, 380.18524;
		cluster_params->cluster_params_r->suff_statistics = suff_statistics;
		VectorXd m3(2);
		m3 << -2.23051, -0.858689;
		MatrixXd psi3(2, 2);
		psi3 << 3.921225, 10.466776, 10.466776, 38.07612;
		niw_hyperparams niwHyperparams3(6.0, m3, 10.0, psi3);
		cluster_params->cluster_params_r->posterior_hyperparams = niwHyperparams3.clone();

		double should_split;
		object.should_split_local(prior, should_split, cluster_params, 10.0, false);

		delete prior;
		delete cluster_params;
		delete gp;

		//log_HR : close to 0.5208818912506104
		EXPECT_EQ(1.0, should_split);
	}

	TEST(local_clusters_actions_test, create_suff_stats_dict_worker)
	{
		MatrixXd group_pts(2, 10);
		group_pts << 2.3863995, -4.672651, -2.3842435, -2.2775807, -0.695474, -2.408718, -3.7753334, -6.659004, 16.301395, -15.971787, 8.34453, -5.352129, -5.805547, -5.2975364, -5.7748003, -5.324071, -5.524739, 6.970031, -10.184864, -1.799381;
		std::map<LabelType, thin_suff_stats*> actual;
		myCudaKernel_gaussian* myCudaKernelObj = new myCudaKernel_gaussian();
		myCudaKernelObj->init(10, group_pts, NULL);
		global_params* gp = new global_params(10, group_pts, NULL, prior_type::Gaussian);
		gp->gen = new myGen();
		gp->cuda = myCudaKernelObj;
		local_clusters_actions object(gp);
		LabelsType indices;
		indices.push_back(0);
		VectorXd m(2);
		m << 0.0, 0.0;
		MatrixXd psi(2, 2);
		psi << 1.0, 0.0, 0.0, 1.0;
		std::vector<int> labels = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
		std::vector<int> sub_labels = { 1, 1, 2, 1, 2, 1, 2, 2, 2, 2 };
		niw_hyperparams niwHyperparams(1.0, m, 5.0, psi);

		myCudaKernelObj->set_labels(labels);
		myCudaKernelObj->set_sub_labels(sub_labels);

		actual = object.create_suff_stats_dict_worker(group_pts, &niwHyperparams, indices);
		EXPECT_EQ(4, actual[0]->l_suff->N);
		EXPECT_EQ(6, actual[0]->r_suff->N);
		EXPECT_NEAR(-20.1569, actual[0]->cluster_suff->points_sum(0), 0.001);
		EXPECT_NEAR(-29.7485, actual[0]->cluster_suff->points_sum(1), 0.001);
		EXPECT_NEAR(-6.97255, actual[0]->l_suff->points_sum(0), 0.001);
		EXPECT_NEAR(-7.62920, actual[0]->l_suff->points_sum(1), 0.001);
		EXPECT_NEAR(-13.1844, actual[0]->r_suff->points_sum(0), 0.001);
		EXPECT_NEAR(-22.1193, actual[0]->r_suff->points_sum(1), 0.001);
	}
}