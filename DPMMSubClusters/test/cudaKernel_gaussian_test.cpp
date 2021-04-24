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
		int numLabels = (int)pow(10, 5);
		int countOne = 0;
		int countTwo = 0;
		int countOther = 0;
		MatrixXd points(2, numLabels);

		LabelsType labels;
		myCudaKernel_gaussian* object = new myCudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array(numLabels, 2);
		for (int i = 0; i < numLabels; i++)
		{
			log_likelihood_array(i, 0) = 0.4;
			log_likelihood_array(i, 1) = 0.6;
		}

		object->init(numLabels, points, NULL);
		int deviceId = object->peak_first_device();
		double* d_r;
		object->allocate_in_device(log_likelihood_array, d_r);
		cudaStream_t stream;
		object->create_stream(stream);

		object->sample_log_cat_array(d_r, (int)log_likelihood_array.cols(), stream, deviceId);

		object->release_in_device(d_r);
		object->release_stream(stream);
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
		int numLabels = (int)pow(10, 5);
		int countOne = 0;
		int countTwo = 0;
		int countOther = 0;
		MatrixXd points(2, numLabels);
		LabelsType labels;
		myCudaKernel_gaussian* object = new myCudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array(numLabels, 2);
		for (int i = 0; i < numLabels; i++)
		{
			log_likelihood_array(i, 0) = 0.6;
			log_likelihood_array(i, 1) = 0.4;
		}

		object->init(numLabels, points, NULL);
		int deviceId = object->peak_first_device();
		double* d_r;
		object->allocate_in_device(log_likelihood_array, d_r);
		cudaStream_t stream;
		object->create_stream(stream);

		object->sample_log_cat_array(d_r, (int)log_likelihood_array.cols(), stream, deviceId);

		object->release_in_device(d_r);
		object->release_stream(stream);
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
		int numLabels = (int)pow(10, 5);
		int countOne1 = 0;
		int countTwo1 = 0;
		int countOther1 = 0;
		int countOne2 = 0;
		int countTwo2 = 0;
		int countOther2 = 0;
		MatrixXd points(2, numLabels);

		LabelsType labels1;
		LabelsType labels2;
		myCudaKernel_gaussian* object = new myCudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array(numLabels, 2);
		for (int i = 0; i < numLabels; i++)
		{
			log_likelihood_array(i, 0) = 0.4;
			log_likelihood_array(i, 1) = 0.6;
		}

		object->init(numLabels, points, 12345);
		int deviceId = object->peak_first_device();
		double* d_r;
		object->allocate_in_device(log_likelihood_array, d_r);
		cudaStream_t stream;
		object->create_stream(stream);
		object->sample_log_cat_array(d_r, (int)log_likelihood_array.cols(), stream, deviceId);
		object->release_in_device(d_r);
		object->release_stream(stream);
		object->get_labels(labels1);

		object->allocate_in_device(log_likelihood_array, d_r);
		object->create_stream(stream);
		object->sample_log_cat_array(d_r, (int)log_likelihood_array.cols(), stream, deviceId);
		object->release_in_device(d_r);
		object->release_stream(stream);
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
		int numLabels = (int)pow(10, 5);
		int countOne = 0;
		int countTwo = 0;
		int countOther = 0;
		MatrixXd points(2, numLabels);

		LabelsType labels;
		myCudaKernel_gaussian* object = new myCudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array(numLabels, 2);
		LabelsType indices;
		for (int i = 0; i < numLabels; i++)
		{
			log_likelihood_array(i, 0) = 0.4;
			log_likelihood_array(i, 1) = 0.6;
			indices.push_back(i);
		}
		Eigen::VectorXd lr_weights(2);
		lr_weights << 0.5, 0.5;

		object->init(numLabels, points, NULL);
		int deviceId = object->peak_first_device();
		cudaStream_t stream;

		object->create_stream(stream);
		int* d_indices;
		object->allocate_in_device(indices, d_indices);
		double* d_lr_weights;
		double* d_r;
		object->allocate_in_device(lr_weights, d_lr_weights);
		object->allocate_in_device(log_likelihood_array, d_r);

		object->sample_log_cat_array_sub_cluster(d_r, (int)indices.size(), d_indices, (int)indices.size(), d_lr_weights, stream, deviceId);

		object->release_in_device(d_indices);
		object->release_in_device(d_lr_weights);
		object->release_in_device(d_r);
		object->release_stream(stream);
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
		int numLabels = (int)pow(10, 5);
		int countOneIndex = 0;
		int countTwoIndex = 0;
		int countOtherIndex = 0;
		int countOneAll = 0;
		int countTwoAll = 0;
		int countOtherAll = 0;
		MatrixXd points(2, numLabels);

		LabelsType labels;
		myCudaKernel_gaussian* object = new myCudaKernel_gaussian();
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
		Eigen::VectorXd lr_weights(2);
		lr_weights << 0.5, 0.5;

		object->init(numLabels, points, NULL);

		//First set values for sub labels
		int deviceId = object->peak_first_device();
		cudaStream_t stream;

		object->create_stream(stream);
		int* d_allIndices;
		object->allocate_in_device(allIndices, d_allIndices);
		double* d_lr_weights;
		object->allocate_in_device(lr_weights, d_lr_weights);
		double* d_log_likelihood_array_all;
		object->allocate_in_device(log_likelihood_array_all, d_log_likelihood_array_all);

		object->sample_log_cat_array_sub_cluster(d_log_likelihood_array_all, (int)allIndices.size(), d_allIndices, (int)allIndices.size(), d_lr_weights, stream, deviceId);

		object->release_in_device(d_allIndices);
		object->release_in_device(d_log_likelihood_array_all);

		//Calculate per index
		int* d_indices;
		object->allocate_in_device(indices, d_indices);
		double* d_log_likelihood_array_index;
		object->allocate_in_device(log_likelihood_array_index, d_log_likelihood_array_index);

		object->sample_log_cat_array_sub_cluster(d_log_likelihood_array_index, (int)indices.size(), d_indices, (int)indices.size(), d_lr_weights, stream, deviceId);
		object->release_in_device(d_lr_weights);
		object->release_in_device(d_indices);
		object->release_in_device(d_log_likelihood_array_index);

		object->release_stream(stream);

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
		int numLabels = (int)pow(10, 5);

		LabelsType labels;
		myCudaKernel_gaussian* object = new myCudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array(numLabels, 2);
		LabelsType indices;
		Eigen::VectorXd lr_weights(2);
		lr_weights << 0.5, 0.5;
		MatrixXd points(2, numLabels);

		object->init(numLabels, points, NULL);
		int deviceId = object->peak_first_device();
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

			cudaStream_t stream;
			object->create_stream(stream);
			int* d_indices;
			object->allocate_in_device(indices, d_indices);
			double* d_lr_weights;
			object->allocate_in_device(lr_weights, d_lr_weights);
			double* d_log_likelihood_array;
			object->allocate_in_device(log_likelihood_array, d_log_likelihood_array);

			object->sample_log_cat_array_sub_cluster(d_log_likelihood_array, (int)indices.size(), d_indices, (int)indices.size(), d_lr_weights, stream, deviceId);

			object->release_in_device(d_indices);
			object->release_in_device(d_lr_weights);
			object->release_in_device(d_log_likelihood_array);
			object->release_stream(stream);

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
			else if (j == numLabels - 1)
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
		int numLabels = (int)pow(10, 5);
		int countOne = 0;
		int countTwo = 0;
		int countFalseOne = 0;
		int countFalseTwo = 0;
		MatrixXd points(2, numLabels);

		LabelsType labels;
		myCudaKernel_gaussian* object = new myCudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array(numLabels, 2);
		LabelsType indices;
		Eigen::VectorXd lr_weights(2);
		lr_weights << 0.5, 0.5;

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
		int deviceId = object->peak_first_device();
		cudaStream_t stream;
		object->create_stream(stream);
		int* d_indices;
		object->allocate_in_device(indices, d_indices);
		double* d_lr_weights;
		object->allocate_in_device(lr_weights, d_lr_weights);
		double* d_log_likelihood_array;
		object->allocate_in_device(log_likelihood_array, d_log_likelihood_array);

		object->sample_log_cat_array_sub_cluster(d_log_likelihood_array, (int)indices.size(), d_indices, (int)indices.size(), d_lr_weights, stream, deviceId);

		object->release_in_device(d_indices);
		object->release_in_device(d_lr_weights);
		object->release_in_device(d_log_likelihood_array);
		object->release_stream(stream);

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
		int numLabels = (int)pow(10, 5);
		int countOne = 0;
		int countTwo = 0;
		int countThree = 0;
		int countFalseOne = 0;
		int countFalseTwo = 0;
		int countFalseThree = 0;
		MatrixXd points(2, numLabels);

		LabelsType labels;
		myCudaKernel_gaussian* object = new myCudaKernel_gaussian();
		Eigen::MatrixXd log_likelihood_array(numLabels, 2);
		LabelsType indices;
		Eigen::VectorXd lr_weights(2);
		lr_weights << 0.5, 0.5;

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
		int deviceId = object->peak_first_device();
		cudaStream_t stream;
		object->create_stream(stream);
		int* d_indices;
		object->allocate_in_device(indices, d_indices);
		double* d_lr_weights;
		object->allocate_in_device(lr_weights, d_lr_weights);
		double* d_log_likelihood_array;
		object->allocate_in_device(log_likelihood_array, d_log_likelihood_array);

		object->sample_log_cat_array_sub_cluster(d_log_likelihood_array, (int)indices.size(), d_indices, (int)indices.size(), d_lr_weights, stream, deviceId);

		object->release_in_device(d_indices);
		object->release_in_device(d_lr_weights);
		object->release_in_device(d_log_likelihood_array);
		object->release_stream(stream);

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

	TEST(cudaKernel_gaussian_test, naive_matrix_multiply)
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

	TEST(cudaKernel_gaussian_test, naive_matrix_multiply_big_k)
	{
		int numLabels = (int)pow(10, 1);
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

	TEST(cudaKernel_gaussian_test, log_likelihood)
	{
		double weight = 0.5;
		MatrixXd x(2, 10);
		x << 7.111513, 6.5072656, 7.656911, 7.021403, 6.395694, -14.513991, -16.44812, -15.126435, 11.43946, 5.0940423, 2.884489, 3.4449763, 2.4345767, 1.152309, 3.076971, -5.085796, -5.293715, -5.330345, 9.191501, -5.4205728;
		myCudaKernel_gaussian cuda;
		cuda.init(10, x, 12345);
		VectorXd r(10);
		VectorXd mu(2);
		mu << -8.180686, -2.1489322;
		MatrixXd sigma(2, 2);
		sigma << 110.386635, 57.80892, 57.80892, 31.78561;
		MatrixXd invSigma(2, 2);
		invSigma << 0.19052099, -0.3465031, -0.3465031, 0.66165066;
		double logdetSigma = 5.117007f;
		LLT<MatrixXd, Upper> invChol;

		std::shared_ptr<mv_gaussian> gaussian = std::make_shared<mv_gaussian>(mu, sigma, invSigma, logdetSigma, invChol);
		cudaStream_t stream;
		int deviceId = cuda.peak_first_device();
		double* d_r;
		cuda.allocate_in_device(10, d_r);
		cuda.create_stream(stream);

		cuda.my_log_likelihood_labels(d_r, weight, gaussian, stream, deviceId);
		cuda.release_stream(stream);

		cuda.copy_from_device(d_r, 10, r);
		cuda.release_in_device(d_r);

		EXPECT_NEAR(-10.914704733523372, r(0), 0.0001);
		EXPECT_NEAR(-9.3608854551121343, r(1), 0.0001);
		EXPECT_NEAR(-12.618434473509506, r(2), 0.0001);
		EXPECT_NEAR(-15.158315554934889, r(3), 0.0001);
		EXPECT_NEAR(-9.8075570569084967, r(4), 0.0001);
		EXPECT_NEAR(-7.1568226940489517, r(5), 0.0001);
		EXPECT_NEAR(-7.701420836694135, r(6), 0.0001);
		EXPECT_NEAR(-7.2147277754819097, r(7), 0.0001);
		EXPECT_NEAR(-9.046606018598947, r(8), 0.0001);
		EXPECT_NEAR(-42.303768571037402, r(9), 0.0001);
	}

	TEST(cudaKernel_gaussian_test, log_likelihood_ignore_weight)
	{
		double weight = 1; //log(1) => 0 will ignore it part of the calculation
		MatrixXd x(2, 10);
		x << 7.111513, 6.5072656, 7.656911, 7.021403, 6.395694, -14.513991, -16.44812, -15.126435, 11.43946, 5.0940423, 2.884489, 3.4449763, 2.4345767, 1.152309, 3.076971, -5.085796, -5.293715, -5.330345, 9.191501, -5.4205728;
		myCudaKernel_gaussian cuda;
		cuda.init(10, x, 12345);
		VectorXd r(10);
		VectorXd mu(2);
		mu << -8.180686, -2.1489322;
		MatrixXd sigma(2, 2);
		sigma << 110.386635, 57.80892, 57.80892, 31.78561;
		MatrixXd invSigma(2, 2);
		invSigma << 0.19052099, -0.3465031, -0.3465031, 0.66165066;
		double logdetSigma = 5.117007f;
		LLT<MatrixXd, Upper> invChol;

		std::shared_ptr<mv_gaussian> gaussian = std::make_shared<mv_gaussian>(mu, sigma, invSigma, logdetSigma, invChol);

		cudaStream_t stream;
		int deviceId = cuda.peak_first_device();
		double* d_r;
		cuda.allocate_in_device(10, d_r);
		cuda.create_stream(stream);

		cuda.my_log_likelihood_labels(d_r, weight, gaussian, stream, deviceId);
		cuda.release_stream(stream);

		cuda.copy_from_device(d_r, 10, r);
		cuda.release_in_device(d_r);

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

	TEST(cudaKernel_gaussian_test, log_likelihood_sigma_is_identity)
	{
		double weight = 1; //log(1) => 0 will ignore it part of the calculation
		MatrixXd x(2, 10);
		x << 7.111513, 6.5072656, 7.656911, 7.021403, 6.395694, -14.513991, -16.44812, -15.126435, 11.43946, 5.0940423, 2.884489, 3.4449763, 2.4345767, 1.152309, 3.076971, -5.085796, -5.293715, -5.330345, 9.191501, -5.4205728;
		myCudaKernel_gaussian cuda;
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

		std::shared_ptr<mv_gaussian> gaussian = std::make_shared<mv_gaussian>(mu, sigma, invSigma, logdetSigma, invChol);

		cudaStream_t stream;
		int deviceId = cuda.peak_first_device();
		double* d_r1;
		cuda.allocate_in_device(10, d_r1);
		cuda.create_stream(stream);

		cuda.my_log_likelihood_labels(d_r1, weight, gaussian, stream, deviceId);

		cuda.copy_from_device(d_r1, 10, r1);
		cuda.release_stream(stream);
		cuda.release_in_device(d_r1);

		MatrixXd z = x.colwise() - gaussian->mu;
		r2 = (z.cwiseProduct(gaussian->invSigma * z)).colwise().sum();
		double scalar = -((gaussian->sigma.size() * log(2 * EIGEN_PI) + gaussian->logdetSigma) / 2);
		r2 = scalar * VectorXd::Ones(r2.size()) - r2 / 2;

		for (int i = 0; i < 10; i++)
		{
			EXPECT_NEAR(r2(i), r1(i), 0.0001);
		}
	}

	TEST(cudaKernel_gaussian_test, divide_points_by_mu_all)
	{
		MatrixXd x(2, 10);
		x << 7.111513, 6.5072656, 7.656911, 7.021403, 6.395694, -14.513991, -16.44812, -15.126435, 11.43946, 5.0940423, 2.884489, 3.4449763, 2.4345767, 1.152309, 3.076971, -5.085796, -5.293715, -5.330345, 9.191501, -5.4205728;
		myCudaKernel_gaussian cuda;
		cuda.init(10, x, 12345);
		VectorXd r(10);
		VectorXd mu(2);
		mu << -8.180686, -2.1489322;
		MatrixXd sigma(2, 2);
		sigma << 110.386635, 57.80892, 57.80892, 31.78561;
		MatrixXd invSigma(2, 2);
		invSigma << 0.19052099, -0.3465031, -0.3465031, 0.66165066;
		double logdetSigma = 5.117007f;
		LLT<MatrixXd, Upper> invChol;
		MatrixXd z1;

		mv_gaussian* gaussian = new mv_gaussian(mu, sigma, invSigma, logdetSigma, invChol);

		cudaStream_t stream;
		int deviceId = cuda.peak_first_device();
		double* d_z;
		cuda.allocate_in_device(2 * 10, d_z);
		cuda.create_stream(stream);

		cuda.my_divide_points_by_mu_all(2, gaussian, d_z, stream, deviceId);

		cuda.release_stream(stream);

		cuda.copy_from_device(d_z, 2, 10, z1);
		cuda.release_in_device(d_z);

		MatrixXd z2 = x.colwise() - gaussian->mu;

		delete gaussian;

		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 10; j++)
			{
				EXPECT_NEAR(z2(i, j), z1(i, j), 0.0001);
			}
		}
	}

	TEST(cudaKernel_gaussian_test, dcolwise_dot_all_labels)
	{
		myCudaKernel_gaussian cuda;
		MatrixXd x(2, 10);
		MatrixXd A(3, 3);
		A << 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9;
		MatrixXd B(3, 3);
		B << 0.5, 0.6, 0.7, 0.8, 0.9, 0.10, 0.11, 0.12, 0.13;
		VectorXd C1;

		cuda.init(0, x, NULL);
		cudaStream_t stream;
		int deviceId = cuda.peak_first_device();
		double* d_A;
		cuda.allocate_in_device(A, d_A);
		double* d_B;
		cuda.allocate_in_device(B, d_B);
		double* d_C;
		cuda.allocate_in_device((int)(A.rows() * B.cols()), d_C);

		cuda.create_stream(stream);
		double scalar = 0.2;
		double weight = 0.4;

		cuda.my_dcolwise_dot_all_labels((int)(A.rows() * B.cols()), (int)A.rows(), d_A, d_B, scalar, d_C, weight, stream);

		cuda.release_stream(stream);

		cuda.copy_from_device(d_C, (int)A.rows(), C1);
		cuda.release_in_device(d_A);
		cuda.release_in_device(d_B);
		cuda.release_in_device(d_C);

		VectorXd C2 = (A.cwiseProduct(B)).colwise().sum();
		C2 = scalar * VectorXd::Ones(C2.size()) - C2 / 2;
		C2 += log(weight) * VectorXd::Ones(C2.rows());

		for (int i = 0; i < C2.rows(); i++)
		{
			EXPECT_NEAR(C2(i), C1(i), 0.0001);
		}
	}

	TEST(cudaKernel_gaussian_test, mul_scalar_sum_A_AT)
	{
		myCudaKernel_gaussian cuda;
		MatrixXd A = MatrixXd::Random(5, 5);
		double scalar = 0.5;
		MatrixXd B1(5, 5);
		MatrixXd B2 = scalar * (A + A.transpose());
		MatrixXd x(5, 10);

		cuda.init(0, x, NULL);
		cudaStream_t stream;
		int deviceId = cuda.peak_first_device();
		double* d_A;
		cuda.allocate_in_device(A, d_A);
		double* d_B;
		cuda.allocate_in_device(A.size(), d_B);

		cuda.create_stream(stream);

		cuda.my_mul_scalar_sum_A_AT(d_A, d_B, 5, scalar, stream);

		cuda.release_stream(stream);

		cuda.copy_from_device(d_B, B1.rows(), B1.cols(), B1);
		cuda.release_in_device(d_A);
		cuda.release_in_device(d_B);

		for (int i = 0; i < B2.rows(); i++)
		{
			for (int j = 0; j < B2.cols(); j++)
			{
				ASSERT_EQ(B2(i, j), B1(i, j));
			}
		}
	}

	TEST(cudaKernel_gaussian_test, sum_rowwise)
	{
		int rows = 100;
		int cols = 76;
		myCudaKernel_gaussian cuda;
		MatrixXd A = MatrixXd::Random(rows, cols);
		VectorXd B1(rows);
		VectorXd B2 = A.rowwise().sum();
		MatrixXd x(rows, cols);

		cuda.init(0, x, NULL);
		cudaStream_t stream;
		int deviceId = cuda.peak_first_device();
		double* d_A;
		cuda.allocate_in_device(A, d_A);
		double* d_B;
		cuda.allocate_in_device(rows, d_B);

		cuda.create_stream(stream);

		cuda.my_sum_rowwise(d_A, d_B, rows, cols, stream);

		cuda.release_stream(stream);

		cuda.copy_from_device(d_B, B1.rows(), B1);
		cuda.release_in_device(d_A);
		cuda.release_in_device(d_B);

		for (int i = 0; i < B2.rows(); i++)
		{
			ASSERT_EQ(B2(i), B1(i));
		}
	}

	TEST(cudaKernel_gaussian_test, multiplie_matrix_by_transpose)
	{
		int rows = 100;
		int cols = 76;
		myCudaKernel_gaussian cuda;
		MatrixXd A = MatrixXd::Random(rows, cols);
		MatrixXd B1;
		MatrixXd B2 = A * A.transpose();
		MatrixXd x(rows, cols);

		cuda.init(0, x, NULL);
		cudaStream_t stream;
		int deviceId = cuda.peak_first_device();
		double* d_A;
		cuda.allocate_in_device(A, d_A);
		double* d_B;
		cuda.allocate_in_device(rows * rows, d_B);

		cuda.create_stream(stream);

		cuda.multiplie_matrix_by_transpose(d_A, d_B, rows, cols);

		cuda.release_stream(stream);

		cuda.copy_from_device(d_B, rows, rows, B1);
		cuda.release_in_device(d_A);
		cuda.release_in_device(d_B);

		for (int i = 0; i < B2.rows(); i++)
		{
			ASSERT_NEAR(B2(i), B1(i), 0.0001);
		}
	}
}