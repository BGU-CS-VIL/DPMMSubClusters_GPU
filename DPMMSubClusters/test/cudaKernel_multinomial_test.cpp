#include <locale>
#include <codecvt>
#include <string>
#include "pch.h"
#include "gtest/gtest.h"
#include "priors/niw.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"
#include "local_clusters_actions.h"
#include "myCudaKernel.h"
#include "myGen.h"
#include "cudaKernel_multinomial.cuh"


namespace DPMMSubClustersTest
{
	TEST(cudaKernel_multinomial_test, log_likelihood_labels)
	{
		myCudaKernel_multinomial cuda;
		multinomial_dist object;
		MatrixXd x(2, 10);
		x << 38.0, 42.0, 40.0, 36.0, 14.0, 11.0, 9.0, 8.0, 5.0, 8.0, 12.0, 8.0, 10.0, 14.0, 36.0, 39.0, 41.0, 42.0, 45.0, 42.0;
		VectorXd r(10);
		std::vector<double> alpha;
		alpha.push_back(-0.8658322);
		alpha.push_back(-0.54593706);

		std::shared_ptr<multinomial_dist> dist = std::make_shared<multinomial_dist>(alpha);

		cudaStream_t stream;

		double* d_r;
		cuda.init(10, x, NULL, true, 0);
		int deviceId = cuda.pick_first_device();
		cuda.allocate_in_device(10, d_r);
		cuda.create_stream(stream);

		cuda.my_log_likelihood_labels(d_r, dist, stream, deviceId);

		cuda.copy_from_device(d_r, 10, r);
		cuda.release_stream(stream);
		cuda.release_in_device(d_r);

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