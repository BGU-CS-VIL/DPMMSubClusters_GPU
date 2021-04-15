#include <iostream>
#include <fstream>
#include <ctime>
#include <random>
#include <chrono>
#include "utils.h"
#include "priors/niw.h"
#include "priors/multinomial_prior.h"
//#include <ppl.h>
#include "cudaKernel.cuh"


#include "distributions_util/pdflib.hpp"


// We expects the data to be in npy format, return a dict of{ group: items }, each file is a different group
void utils::load_data(std::string path, std::string prefix, Eigen::MatrixXd& mat_out, bool swapDimension)
{
	//std::string fullPath = prefix + path;
	//cnpy::NpyArray npy_data = cnpy::npy_load(fullPath);

	//int data_row = npy_data.shape[1];
	//int data_col = npy_data.shape[0];

	//float* data = npy_data.data<float>();
	//mat_out.resize(data_row, data_col);
	//int l = 0;
	//for (size_t i = 0; i < data_row; i++)
	//{
	//	for (size_t j = 0; j < data_col; j++)
	//	{
	//		mat_out(i, j) = data[l];
	//		l++;
	//	}
	//}

//	float* ptr = static_cast<float *>(malloc(data_row * data_col * sizeof(float)));
//	memcpy(ptr, npy_data.data<float>(), data_row * data_col * sizeof(float));
//	 new (&mat_out) Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>>(reinterpret_cast<double *>(ptr), data_col, data_row);


	if (swapDimension)
	{
//		mat_out.adjointInPlace();
	}
}

void utils::saveToFile(const LabelsType & mat, const char * filePrefixName)
{
	ofstream myfile;
	static int iter = 0;
	++iter;
	std::string str = filePrefixName + std::to_string(iter) + ".csv";
	myfile.open(str.c_str());


	for (LabelType i = 0; i < mat.size(); i++)
	{
		myfile << mat[i] << "\n";
	}
	myfile.close();
}

void utils::saveToFile(const MatrixXd & mat, const char * filePrefixName)
{
	ofstream myfile;
	static int iter = 0;
	++iter;
	std::string str = filePrefixName + std::to_string(iter) + ".csv";
	myfile.open(str.c_str());


	for (BaseType i = 0; i < mat.rows(); i++)
	{
		for (BaseType j = 0; j < mat.cols(); j++)
		{
			myfile << mat(i, j) << ",";

		}
		myfile << "\n";
	}
	myfile.close();
}

void utils::saveToFile(const VectorXd & mat, const char * filePrefixName)
{
	ofstream myfile;
	static int iter = 0;
	++iter;
	std::string str = filePrefixName + std::to_string(iter) + ".csv";
	myfile.open(str.c_str());


	for (BaseType i = 0; i < mat.rows(); i++)
	{
		for (BaseType j = 0; j < mat.cols(); j++)
		{
			myfile << mat(i, j) << ",";

		}
		myfile << "\n";
	}
	myfile.close();
}

void utils::saveToFile(const MatrixXd & mat1, const MatrixXd & mat2, const char * filePrefixName)
{
	ofstream myfile;
	static int iter = 0;
	++iter;
	std::string str = filePrefixName + std::to_string(iter) + ".csv";
	myfile.open(str.c_str());


	for (BaseType i = 0; i < mat1.rows(); i++)
	{
		for (BaseType j = 0; j < mat1.cols(); j++)
		{
			myfile << mat1(i, j) << ",";

		}
		myfile << "\n";
	}

	myfile << "\n\n";

	for (BaseType i = 0; i < mat2.rows(); i++)
	{
		for (BaseType j = 0; j < mat2.cols(); j++)
		{
			myfile << mat2(i, j) << ",";

		}
		myfile << "\n";
	}
	myfile.close();
}

double utils::log_multivariate_gamma(double x, long D)
{
	double res = D * (D - 1) / 4.0 * log(EIGEN_PI);
	for (long d = 1; d <= D; ++d)
	{
		res += r8_gamma_log((x + (1 - d) / 2.0));
	}
	return res;
}