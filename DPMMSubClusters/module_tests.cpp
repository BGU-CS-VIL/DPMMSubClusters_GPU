#include "module_tests.h"


#include "priors/niw.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"

#include <iostream>
#include <cmath>
#include <cstring>
#include <string>
#include "draw.h"
#include "moduleTypes.h"
#include "utils.h"

#include "distributions_util/logdet.h"

using namespace std;

double xray[300], yray[300], y1ray[300], y2ray[300], zmat[50][50];
string cl1, cl2;
int id_lis1, id_lis2, id_pbut;


ClusterIndexType module_tests::RandomMess()
{
	//double logpi = log(EIGEN_PI);

	//MatrixXd prior_psi(2,2);
	//	prior_psi(0, 0) = 1;
	//	prior_psi(0, 1) = 0;
	//	prior_psi(1, 0) = 0;
	//	prior_psi(1, 1) = 1;

	//	MatrixXd post_psi(2, 2);
	//	post_psi(0, 0) = 50.367256;
	//	post_psi(0, 1) = -9.859212;
	//	post_psi(1, 0) = -9.859212;
	//	post_psi(1, 1) = 118.11292;


	//	double raz2 = utils::log_multivariate_gamma(50000, 2);
	////raz2:981964.0

	//	double raz = -100000.0 * 2 * 0.5*logpi +
	//		utils::log_multivariate_gamma(100005.0 / 2, 2) -
	//		utils::log_multivariate_gamma(5.0 / 2, 2) +
	//		(5.0 / 2)*(2 * log(5.0) + logdet(prior_psi)) -
	//		(100005.0 / 2)*(2 * log(100005.0) + logdet(post_psi)) +
	//		(2 / 2)*(log(1.0 / 100001.0));



	////	return : -717561.3113786621
	////	L : 55052.0 R : 44948.0 B : 100000.0
	////	log_likihood_l : -324959.1143357295 log_likihood_r : -318303.59205078136 log_likihood : -717561.3113786621 alpha : 100000.0.3113786621 alpha : 100000.0







	//return 0;

	//Eigen::MatrixXi raz = Eigen::MatrixXi(2, 3);
	//int k = 0;
	//raz(0, 0) = k++;
	//raz(1, 0) = k++;
	//raz(0, 1) = k++;
	//raz(1, 1) = k++;
	//raz(0, 2) = k++;
	//raz(1, 2) = k++;

	//std::cout << raz.data() << std::endl;




	printf("Testing Module (Random mess)");
	srand(12345);
	data_generators data_generators;
	MatrixXd  x;
	std::vector<double> labels;
	float** tmean;
	float** tcov;
	int N = pow(10, 5);
	//int N =2;
	int D = 2;
	int numClusters = 2;
	int numIters = 100;

	data_generators.generate_gaussian_data(N, D, numClusters, 100.0, x, labels, tmean, tcov);
	draw::draw_labels("Expected", x, labels);

	niw_hyperparams hyper_params(1.0, VectorXd::Zero(D), 5, MatrixXd::Identity(D, D));

	dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);
	ModelInfo dp = dps.dp_parallel(&hyper_params, N, numIters, 1, true, false, 15, labels);

	for (DimensionsType i = 0; i < D; i++)
	{
		delete[] tmean[i];
	}
	for (DimensionsType i = 0; i < D; i++)
	{
		delete[] tcov[i];
	}
	return dp.dp_model->group.local_clusters.size();
}

ClusterIndexType module_tests::RandomMessHighDim()
{
	srand(12345);
	data_generators data_generators;
	MatrixXd  x;
	std::vector<double> labels;
	float** tmean;
	float** tcov;
	int N = pow(10, 5);
	//int D = 64;
	int D = 2;
	int numClusters = 20;
	int numIters = 200;
	int foundClusters = 0;

	data_generators.generate_gaussian_data(N, D, numClusters, 100.0, x, labels, tmean, tcov);
	for (DimensionsType i = 0; i < D; i++)
	{
		delete[] tmean[i];
	}
	for (DimensionsType i = 0; i < D; i++)
	{
		delete[] tcov[i];
	}

	for (size_t i = 0; i < 1; i++)
	{
		printf("\n\n%ld: Start ***************************************************************\n\n", i);
		niw_hyperparams hyper_params(1.0, VectorXd::Zero(D), 5, MatrixXd::Identity(D, D));

		dp_parallel_sampling_class dps(N, x, 0, prior_type::Gaussian);
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		ModelInfo dp = dps.dp_parallel(&hyper_params, N, numIters, 1, false, false, 15, labels);
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::string str = "Found: " + std::to_string(dp.dp_model->group.local_clusters.size()) + " clusters";
		std::cout << str << std::endl;
		std::cout << "Time:" << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[seconds]" << std::endl;
		std::cout << "Time:" << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[microseconds]" << std::endl;
		foundClusters = dp.dp_model->group.local_clusters.size();
		printf("\n\n%ld: End ***************************************************************\n\n", i);
	}
	
	return foundClusters;
}

void module_tests::TestingData(std::vector<double> &labels)
{
	printf("Testing data");
	srand(12345);
	data_generators data_generators;
	MatrixXd  x;
	float** tmean;
	float** tcov;
	int N = 50000;
	int D = 2;
	data_generators.generate_gaussian_data(N, D, 5, 30, x, labels, tmean, tcov);
	draw::draw_labels("Expected", x, labels);

	for (DimensionsType i = 0; i < D; i++)
	{
		delete[] tmean[i];
	}
	for (DimensionsType i = 0; i < D; i++)
	{
		delete[] tcov[i];
	}
}

void module_tests::ReadPnyFileIntoData(std::string path, std::string prefix, MatrixXd &mat)
{
	utils::load_data(path, prefix, mat);
}

ClusterIndexType module_tests::RunModuleFromFile(std::string path, std::string prefix)
{
	printf("Testing Module (Run Module From File)");
	int D = 2;
	int N = 10;//TBD
	unsigned long long random_seed = 0;
	MatrixXd x;

	dp_parallel_sampling_class dps(N, x, random_seed, prior_type::Gaussian);
	std::vector<double> gt;
	global_params globalParams(N, x, random_seed, prior_type::Gaussian);
	globalParams.initial_clusters = 1;
	globalParams.use_verbose = true;
	globalParams.should_save_model = false;
	globalParams.burnout_period = 20;
	globalParams.ground_truth = gt;
	globalParams.max_clusters = DBL_MAX;
	globalParams.data_path = path;
	globalParams.data_prefix = prefix;
	globalParams.hyper_params = new niw_hyperparams(1.0, VectorXd::Zero(D), 5, MatrixXd::Identity(D, D));


	ModelInfo dp = dps.dp_parallel(&globalParams, "");

	return dp.dp_model->group.local_clusters.size();
}

//void module_tests2::Run2()
//{
//	printf("Testing Module (Determinstic)");
//
//	data = create_data_for_test()
//		labels, clusters, weights = fit(data, 100.0, iters = 200, seed = 12345, burnout = 5)
//		bool result = true;
//			for (size_t j = 0; j < 250; j++)
//			{
//				if (data(i,j)!= data(i,0))
//				{
//					result = false;
//				}
//			}
//		//@test all(data[:, 1 : 250] . == data[:, 1])
//		//@test all(data[:, 251 : 500] . == data[:, 251])
//		//@test all(data[:, 501 : 750] . == data[:, 501])
//		//@test all(data[:, 751 : 1000] . == data[:, 751])
//		@test length(clusters) == 4
//		println(weights)
//		@test all(weights . >= 0.15)
//		labels_histogram = get_labels_histogram(labels)
//		for (k, v) in labels_histogram
//			@test v == 250
//			end
//			end
//
//}
//
//function create_data_for_test()
//data = zeros(Float32, 2, 1000)
//data[:, 1 : 250] . = [-1, -1]
//data[:, 251 : 500] . = [-1, 1]
//data[:, 501 : 750] . = [1, -1]
//data[:, 751 : 1000] . = [1, 1]
//return data
//end
//
//
