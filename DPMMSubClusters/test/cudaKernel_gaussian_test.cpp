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
#include "cudaKernel_gaussian.cuh"


namespace DPMMSubClustersTest
{
	TEST(cudaKernel_gaussian_test, sample_log_cat_array_less_prob_for_one)
	{
		int numLabels = pow(10, 5);
		int countOne = 0;
		int countTwo = 0;
		int countOther = 0;
		MatrixXd points(2, numLabels);

		LabelsType labels;
		cudaKernel* object = new cudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array(numLabels, 2);
		for (int i = 0; i < numLabels; i++)
		{
			log_likelihood_array(i, 0) = 0.4;
			log_likelihood_array(i, 1) = 0.6;
		}

		object->init(numLabels, points, NULL);

		object->sample_log_cat_array(log_likelihood_array, 0);
		object->get_labels(labels);

		for (int i = 0; i < numLabels; i++)
		{
			if (labels[i] == 1)
			{
				countOne++;
			}
			else if (labels[i] == 2)
			{
				countTwo++;
			}
			else
			{
				countOther++;
			}
		}

		EXPECT_TRUE(countOne < countTwo);
		EXPECT_EQ(45260, countOne);
		EXPECT_EQ(54740, countTwo);
		EXPECT_EQ(0, countOther);

		delete object;
	}

	TEST(cudaKernel_gaussian_test, sample_log_cat_array_less_prob_for_two)
	{
		int numLabels = pow(10, 5);
		int countOne = 0;
		int countTwo = 0;
		int countOther = 0;
		MatrixXd points(2, numLabels);
		LabelsType labels;
		cudaKernel* object = new cudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array(numLabels, 2);
		for (int i = 0; i < numLabels; i++)
		{
			log_likelihood_array(i, 0) = 0.6;
			log_likelihood_array(i, 1) = 0.4;
		}

		object->init(numLabels, points, NULL);

		object->sample_log_cat_array(log_likelihood_array, 0);
		object->get_labels(labels);

		for (int i = 0; i < numLabels; i++)
		{
			if (labels[i] == 1)
			{
				countOne++;
			}
			else if (labels[i] == 2)
			{
				countTwo++;
			}
			else
			{
				countOther++;
			}
		}

		EXPECT_TRUE(countOne > countTwo);
		EXPECT_EQ(55257, countOne);
		EXPECT_EQ(44743, countTwo);
		EXPECT_EQ(0, countOther);

		delete object;
	}

	TEST(cudaKernel_gaussian_test, sample_log_cat_array_with_seed)
	{
		int numLabels = pow(10, 5);
		int countOne1 = 0;
		int countTwo1 = 0;
		int countOther1 = 0;
		int countOne2 = 0;
		int countTwo2 = 0;
		int countOther2 = 0;
		MatrixXd points(2, numLabels);

		LabelsType labels1;
		LabelsType labels2;
		cudaKernel* object = new cudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array(numLabels, 2);
		for (int i = 0; i < numLabels; i++)
		{
			log_likelihood_array(i, 0) = 0.4;
			log_likelihood_array(i, 1) = 0.6;
		}

		object->init(numLabels, points, 12345);

		object->sample_log_cat_array(log_likelihood_array, 0);
		object->get_labels(labels1);
		object->sample_log_cat_array(log_likelihood_array, 0);
		object->get_labels(labels2);

		for (int i = 0; i < numLabels; i++)
		{
			if (labels1[i] == 1)
			{
				countOne1++;
			}
			else if (labels1[i] == 2)
			{
				countTwo1++;
			}
			else
			{
				countOther1++;
			}
		}

		for (int i = 0; i < numLabels; i++)
		{
			if (labels2[i] == 1)
			{
				countOne2++;
			}
			else if (labels2[i] == 2)
			{
				countTwo2++;
			}
			else
			{
				countOther2++;
			}
		}

		EXPECT_NE(countOne1, countOne2);
		EXPECT_NE(countTwo1, countTwo2);
		EXPECT_EQ(0, countOther1);
		EXPECT_EQ(0, countOther2);

		std::string str = "countOne1: " + std::to_string(countOne1) + ", countOne2: " + std::to_string(countOne2);
		std::cout << str << std::endl;
		str = "countTwo1: " + std::to_string(countTwo1) + ", countTwo2: " + std::to_string(countTwo2);
		std::cout << str << std::endl;

		delete object;
	}

	TEST(cudaKernel_gaussian_test, sample_log_cat_array_sub_cluster_less_prob_for_one_all_indices)
	{
		int numLabels = pow(10, 5);
		int countOne = 0;
		int countTwo = 0;
		int countOther = 0;
		MatrixXd points(2, numLabels);

		LabelsType labels;
		cudaKernel* object = new cudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array(numLabels, 2);
		LabelsType indices;
		for (int i = 0; i < numLabels; i++)
		{
			log_likelihood_array(i, 0) = 0.4;
			log_likelihood_array(i, 1) = 0.6;
			indices.push_back(i);
		}
		std::vector<double> lr_weights = { 0.5,0.5 };

		object->init(numLabels, points, NULL);

		object->sample_log_cat_array_sub_cluster(indices.data(), indices.size(), log_likelihood_array, lr_weights, 0);
		object->get_sub_labels(labels);

		for (int i = 0; i < numLabels; i++)
		{
			if (labels[i] == 1)
			{
				countOne++;
			}
			else if (labels[i] == 2)
			{
				countTwo++;
			}
			else
			{
				countOther++;
			}
		}

		EXPECT_TRUE(countOne < countTwo);
		EXPECT_EQ(45260, countOne);
		EXPECT_EQ(54740, countTwo);
		EXPECT_EQ(0, countOther);

		delete object;
	}

	TEST(cudaKernel_gaussian_test, sample_log_cat_array_sub_cluster_less_prob_for_one_few_indices)
	{
		int numLabels = pow(10, 5);
		int countOneIndex = 0;
		int countTwoIndex = 0;
		int countOtherIndex = 0;
		int countOneAll = 0;
		int countTwoAll = 0;
		int countOtherAll = 0;
		MatrixXd points(2, numLabels);

		LabelsType labels;
		cudaKernel* object = new cudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array_all(numLabels, 2);
		LabelsType indices;
		LabelsType allIndices;
		for (int i = 0; i < numLabels; i++)
		{
			log_likelihood_array_all(i, 0) = 0.1;
			log_likelihood_array_all(i, 1) = 0.9;
			if (i % 3 == 0)
			{
				indices.push_back(i);
			}
			allIndices.push_back(i);
		}

		Eigen::MatrixXd log_likelihood_array_index = log_likelihood_array_all;
		log_likelihood_array_index.conservativeResize(indices.size(), 2);
		std::vector<double> lr_weights = { 0.5,0.5 };

		object->init(numLabels, points, NULL);

		//First set values for sub labels
		object->sample_log_cat_array_sub_cluster(allIndices.data(), allIndices.size(), log_likelihood_array_all, lr_weights, 0);

		//Calculate per index
		object->sample_log_cat_array_sub_cluster(indices.data(), indices.size(), log_likelihood_array_index, lr_weights, 0);
		object->get_sub_labels(labels);

		for (int i = 0; i < numLabels; i++)
		{
			if (std::find(indices.begin(), indices.end(), i) != indices.end())
			{
				if (labels[i] == 1)
				{
					countOneIndex++;
				}
				else if (labels[i] == 2)
				{
					countTwoIndex++;
				}
				else
				{
					countOtherIndex++;
				}
			}

			if (labels[i] == 1)
			{
				countOneAll++;
			}
			else if (labels[i] == 2)
			{
				countTwoAll++;
			}
			else
			{
				countOtherAll++;
			}

		}

		EXPECT_TRUE(countOneIndex < countTwoIndex);
		EXPECT_TRUE(countOneAll < countTwoAll);
		EXPECT_EQ(10279, countOneIndex);
		EXPECT_EQ(23055, countTwoIndex);
		EXPECT_EQ(31207, countOneAll);
		EXPECT_EQ(68793, countTwoAll);
		EXPECT_EQ(0, countOtherIndex);
		EXPECT_EQ(0, countOtherAll);

		delete object;
	}

	TEST(cudaKernel_gaussian_test, sample_log_cat_array_sub_cluster_left_increases_prob_in_each_iter)
	{
		int numLabels = pow(10, 5);

		LabelsType labels;
		cudaKernel* object = new cudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array(numLabels, 2);
		LabelsType indices;
		std::vector<double> lr_weights = { 0.5,0.5 };
		MatrixXd points(2, numLabels);

		object->init(numLabels, points, NULL);
		double leftProb = 0.5;
		for (int i = 0; i < numLabels; i++)
		{
			indices.push_back(i);
		}
		int j = 0;
		while (leftProb < 1.0)
		{
			for (int i = 0; i < numLabels; i++)
			{
				log_likelihood_array(i, 0) = leftProb;
				log_likelihood_array(i, 1) = 1 - leftProb;
			}

			object->sample_log_cat_array_sub_cluster(indices.data(), indices.size(), log_likelihood_array, lr_weights, 0);

			object->get_sub_labels(labels);
			int countOne = 0;
			int countTwo = 0;
			int countOther = 0;
			for (int i = 0; i < numLabels; i++)
			{
				if (labels[i] == 1)
				{
					countOne++;
				}
				else if (labels[i] == 2)
				{
					countTwo++;
				}
				else
				{
					countOther++;
				}
			}
			double leftPerc = (double)countOne / (countOne + countTwo);
			std::string str = "leftPerc:" + std::to_string(leftPerc) + " leftProb:" + std::to_string(leftProb) + "\n";
			std::cout << str << std::endl;
			if (j == 0)
			{
				EXPECT_TRUE(leftPerc > leftProb - 0.03 && leftPerc < leftProb + 0.03);
			}
			else if (j == numLabels-1)
			{
				EXPECT_TRUE(leftPerc > leftProb - 0.23904 && leftPerc < leftProb + 0.23904);
			}
			EXPECT_EQ(0, countOther);
			leftProb += 0.05;
			++j;
		}

		delete object;
	}

	TEST(cudaKernel_gaussian_test, sample_log_cat_array_sub_cluster_even_left_odd_right)
	{
		int numLabels = pow(10, 5);
		int countOne = 0;
		int countTwo = 0;
		int countFalseOne = 0;
		int countFalseTwo = 0;
		MatrixXd points(2, numLabels);

		LabelsType labels;
		cudaKernel* object = new cudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array(numLabels, 2);
		LabelsType indices;
		std::vector<double> lr_weights = { 0.5,0.5 };

		for (int i = 0; i < numLabels; i++)
		{
			if (i % 2 == 0)
			{
				//Even will go left
				log_likelihood_array(i, 0) = 0.9;
				log_likelihood_array(i, 1) = 0.1;
			}
			else
			{
				//Odd will go right
				log_likelihood_array(i, 0) = 0.1;
				log_likelihood_array(i, 1) = 0.9;
			}
			indices.push_back(i);
		}

		object->init(numLabels, points, NULL);

		object->sample_log_cat_array_sub_cluster(indices.data(), indices.size(), log_likelihood_array, lr_weights, 0);
		object->get_sub_labels(labels);

		for (int i = 0; i < numLabels; i++)
		{
			if (i % 2 == 0)
			{
				if (labels[i] == 1)
				{
					countOne++;
				}
				else
				{
					countFalseOne++;
				}
			}
			else
			{
				if (labels[i] == 2)
				{
					countTwo++;
				}
				else
				{
					countFalseTwo++;
				}
			}
		}

		double leftPerc = (double)countOne / (countOne + countFalseOne);
		double rightPerc = (double)countTwo / (countTwo + countFalseTwo);
		std::string str = "leftPerc:" + std::to_string(leftPerc) + " rightPerc:" + std::to_string(rightPerc) + "\n";
		std::cout << str << std::endl;
		EXPECT_TRUE(leftPerc > 0.68);
		EXPECT_TRUE(rightPerc > 0.68);

		delete object;
	}

	TEST(cudaKernel_gaussian_test, sample_log_cat_array_sub_cluster_3_groups_of_prob)
	{
		int numLabels = pow(10, 5);
		int countOne = 0;
		int countTwo = 0;
		int countThree = 0;
		int countFalseOne = 0;
		int countFalseTwo = 0;
		int countFalseThree = 0;
		MatrixXd points(2, numLabels);

		LabelsType labels;
		cudaKernel* object = new cudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array(numLabels, 2);
		LabelsType indices;
		std::vector<double> lr_weights = { 0.5,0.5 };

		for (int i = 0; i < numLabels; i++)
		{
			if (i % 3 == 0)
			{
				log_likelihood_array(i, 0) = 0.9;
				log_likelihood_array(i, 1) = 0.1;
			}
			else if (i % 3 == 1)
			{
				log_likelihood_array(i, 0) = 0.6;
				log_likelihood_array(i, 1) = 0.4;
			}
			else
			{
				log_likelihood_array(i, 0) = 0.1;
				log_likelihood_array(i, 1) = 0.9;
			}
			indices.push_back(i);
		}

		object->init(numLabels, points, NULL);

		object->sample_log_cat_array_sub_cluster(indices.data(), indices.size(), log_likelihood_array, lr_weights, 0);
		object->get_sub_labels(labels);

		for (int i = 0; i < numLabels; i++)
		{
			if (i % 3 == 0)
			{
				if (labels[i] == 1)
				{
					countOne++;
				}
				else
				{
					countFalseOne++;
				}
			}
			else if (i % 3 == 1)
			{
				if (labels[i] == 1)
				{
					countTwo++;
				}
				else
				{
					countFalseTwo++;
				}
			}
			else
			{
				if (labels[i] == 2)
				{
					countThree++;
				}
				else
				{
					countFalseThree++;
				}
			}
		}

		double perc1 = (double)countOne / (countOne + countFalseOne);
		double perc2 = (double)countTwo / (countTwo + countFalseTwo);
		double perc3 = (double)countThree / (countThree + countFalseThree);
		std::string str = "perc1:" + std::to_string(perc1) + " perc2:" + std::to_string(perc2) + " perc3:" + std::to_string(perc3) + "\n";
		std::cout << str << std::endl;
		EXPECT_TRUE(perc1 > 0.68);
		EXPECT_TRUE(perc2 < 0.6);
		EXPECT_TRUE(perc2 > 0.5);
		EXPECT_TRUE(perc3 > 0.68);

		delete object;
	}

	TEST(cudaKernel_gaussian_test, naive_matrix_multiply_v3)
	{
		int numLabels = 10;
		MatrixXd points(2, numLabels);
		cudaStream_t stream;
		myCudaKernel_gaussian object;
		object.init(numLabels, points, NULL);
		int m = 3;
		int n = 2;
		int k = 3;
		MatrixXd A(m, n);
		A << 0.1, 0.2, 0.3, 0.4, 0.5, 0.6;
		MatrixXd B(n, k);
		B << 0.5, 0.6, 0.7, 0.8, 0.9, 0.10;
		
		double* d_A;
		object.allocate_in_device(A, d_A);
		double* d_B;
		object.allocate_in_device(B, d_B);
		double* d_C;
		object.allocate_in_device(m * k, d_C);
		object.create_stream(stream);

		object.my_naive_matrix_multiply(d_A, d_B, d_C, m, n, k, stream);
		object.release_stream(stream);
		MatrixXd C;
		object.copy_from_device(d_C, m, k, C);

		object.release_in_device(d_A);
		object.release_in_device(d_B);
		object.release_in_device(d_C);

		MatrixXd C_cpu = A * B;

		for (int i = 0; i < C_cpu.rows(); i++)
		{
			for (int j = 0; j < C_cpu.cols(); j++)
			{
				EXPECT_NEAR(C_cpu(i, j), C(i, j), 0.0001);
			}
		}
	}

	TEST(cudaKernel_gaussian_test, naive_matrix_multiply_v3_big_k)
	{
		int numLabels = pow(10, 1);
		MatrixXd points(2, numLabels);
		cudaStream_t stream;
		myCudaKernel_gaussian object;
		object.init(numLabels, points, NULL);
		int m = 3;
		int n = 2;
		int k = numLabels;
		MatrixXd A = MatrixXd::Random(m, n);
		MatrixXd B = MatrixXd::Random(n, k);

		double* d_A;
		object.allocate_in_device(A, d_A);
		double* d_B;
		object.allocate_in_device(B, d_B);
		double* d_C;
		object.allocate_in_device(m * k, d_C);
		object.create_stream(stream);

		object.my_naive_matrix_multiply(d_A, d_B, d_C, m, n, k, stream);
		object.release_stream(stream);
		MatrixXd C;
		object.copy_from_device(d_C, m, k, C);

		object.release_in_device(d_A);
		object.release_in_device(d_B);
		object.release_in_device(d_C);

		MatrixXd C_cpu = A * B;

		for (int i = 0; i < C_cpu.rows(); i++)
		{
			for (int j = 0; j < C_cpu.cols(); j++)
			{
				EXPECT_NEAR(C_cpu(i, j), C(i, j), 0.0001);
			}
		}
	}

	//TEST(cudaKernel_gaussian_test, dcolwise_dot)
	//{
	//	cudaKernel_gaussian object;
	//	MatrixXd A(3, 2);
	//	A << 0.1, 0.2, 0.3, 0.4, 0.5, 0.6;
	//	MatrixXd B(2, 3);
	//	B << 0.5, 0.6, 0.7, 0.8, 0.9, 0.10;
	//	Eigen::VectorXd r;

	//	object.init(0, NULL);
	//	object.dcolwise_dot(r, A, B);

	//	EXPECT_NEAR(0.55800000000000005, r(0), 0.0001);
	//	EXPECT_NEAR(0.11600000000000002, r(1), 0.0001);
	//}
}