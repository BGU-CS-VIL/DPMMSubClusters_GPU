using namespace std;
#include <iostream>
#include <fstream>
#include <ctime>
#include <random>
#include <chrono>
#include "utils.h"
#include "distributions_util/pdflib.hpp"
#include "cnpy.h"
#include "check_time.h"

// We expects the data to be in npy format, return a dict of{ group: items }, each file is a different group
void utils::load_data(std::string fileName, Eigen::MatrixXd& mat_out, bool swapDimension)
{
	std::string fullPath = fileName + ".npy";
	cnpy::NpyArray npy_data = cnpy::npy_load(fullPath);

	const int data_row = (int)npy_data.shape[0];
	const int data_col = (int)npy_data.shape[1];

	mat_out.resize(data_row, data_col);

	if (npy_data.word_size == 4)
	{
		float* loaded_data = npy_data.data<float>();
		data_to_mat(loaded_data, mat_out);
	}
	else if (npy_data.word_size == 8)
	{
		double* loaded_data = npy_data.data<double>();
		data_to_mat(loaded_data, mat_out);
	}
	else
	{
		printf("Error data!!! Actual size=%ld\n", (long)npy_data.word_size);
		mat_out.resize(0, 0);
		return;
	}

	if (swapDimension)
	{
		mat_out.adjointInPlace();
	}
}

void utils::load_data(std::string fileName, std::shared_ptr<LabelsType> &vec_out)
{
	std::string fullPath = fileName + ".labels";
	cnpy::NpyArray npy_data = cnpy::npy_load(fullPath);

	const int data_row = (int)npy_data.shape[1];

	vec_out->resize(data_row);

	if (npy_data.word_size == 4)
	{
		float* loaded_data = npy_data.data<float>();
		data_to_vec(loaded_data, vec_out);
	}
	else if (npy_data.word_size == 8)
	{
		double* loaded_data = npy_data.data<double>();
		data_to_vec(loaded_data, vec_out);
	}
	else
	{
		printf("Error data!!! Actual size=%ld\n", (long)npy_data.word_size);
		vec_out->resize(0);
		return;
	}
}

void utils::save_data(std::string fileName, const Eigen::MatrixXd& mat)
{
	std::string fullPath = fileName + ".npy";
	std::vector<double> data(mat.size());
	mat_to_data(mat, data);
	cnpy::npy_save(fullPath, &data[0], { (size_t)mat.cols(),(size_t)mat.rows() });
}

double utils::log_multivariate_gamma(double x, long D)
{
	CHECK_TIME("utils::log_multivariate_gamma");
	double res = D * (D - 1) / 4.0 * log(EIGEN_PI);
	for (long d = 1; d <= D; ++d)
	{
		res += r8_gamma_log((x + (1 - d) / 2.0));
	}
	return res;
}


template<typename T>
void utils::data_to_mat(const T& data, Eigen::MatrixXd& mat)
{
	int l = 0;
	for (int j = 0; j < mat.cols(); j++)
	{
		for (int i = 0; i < mat.rows(); i++)
		{
			mat(i, j) = data[l];
			l++;
		}
	}
}

void utils::mat_to_data(const Eigen::MatrixXd& mat, std::vector<double>& data)
{
	int l = 0; 
	for (int i = 0; i < mat.rows(); i++)
	{
		for (int j = 0; j < mat.cols(); j++)
		{
			data[l] = mat(i, j);
			l++;
		}
	}
}

template<typename T>
void utils::data_to_vec(const T& data, std::shared_ptr<LabelsType> &vec)
{
	for (int i = 0; i < vec->size(); i++)
	{
		(*vec)[i] = data[i];
	}
}