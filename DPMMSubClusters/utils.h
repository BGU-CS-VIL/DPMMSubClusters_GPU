#pragma once

#include <random>
#include "Eigen/Dense"
#include "priors/prior.h"
#include "ds.h"
#include "global_params.h"

class utils
{
public:
	static void load_data(std::string fileName, Eigen::MatrixXd& mat_out, bool swapDimension = true);
	static void load_data(std::string fileName, LabelsType& vec_out);
	static void save_data(std::string fileName, const Eigen::MatrixXd& mat);
	static double log_multivariate_gamma(double x, long D);

private:
	template<typename T>
	static void data_to_mat(const T& data, Eigen::MatrixXd& mat);
	static void mat_to_data(const Eigen::MatrixXd& mat, std::vector<double>& data);
	template<typename T>
	static void data_to_vec(const T& data, LabelsType& vec);

};

