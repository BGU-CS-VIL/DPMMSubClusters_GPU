#include "pch.h"
#include "gtest/gtest.h"
#include "Eigen/Dense"
#include "utils.h"

namespace DPMMSubClustersTest
{
	TEST(utils_test, load_data_swap_true)
	{
		MatrixXd mat_out;
		utils::load_data_model("mnm_data", mat_out, true);

		ASSERT_EQ(100, mat_out.rows());
		ASSERT_EQ(1000, mat_out.cols());
		ASSERT_EQ(0, mat_out(0, 0));
		ASSERT_EQ(1, mat_out(2, 500));
		ASSERT_EQ(1, mat_out(0, 2));
	}

	TEST(utils_test, load_data_swap_false)
	{
		MatrixXd mat_out;
		utils::load_data_model("mnm_data", mat_out, false);

		ASSERT_EQ(1000, mat_out.rows());
		ASSERT_EQ(100, mat_out.cols());
		ASSERT_EQ(0, mat_out(0, 0));
		ASSERT_EQ(1, mat_out(500, 2));
		ASSERT_EQ(3, mat_out(0, 99));
	}

	TEST(utils_test, load_data_save_load_to_compare)
	{
		MatrixXd mat;
		utils::load_data_model("mnm_data", mat, true);
		utils::save_data("mnm_data_saved", mat);
		mat.resize(0, 0);
		utils::load_data_model("mnm_data_saved", mat, true);

		ASSERT_EQ(100, mat.rows());
		ASSERT_EQ(1000, mat.cols());
		ASSERT_EQ(0, mat(0, 0));
		ASSERT_EQ(0, mat(1, 1));
		ASSERT_EQ(2, mat(99, 999));
	}
}