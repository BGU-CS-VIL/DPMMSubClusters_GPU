#pragma once

#include <random>
#include "Eigen/Dense"
#include "priors/prior.h"
#include "ds.h"
#include "global_params.h"

class utils
{
public:
	static void load_data(std::string path, std::string prefix, Eigen::MatrixXd& mat_out, bool swapDimension = true);
	static double log_multivariate_gamma(double x, long D);
	static void saveToFile(const LabelsType & mat, const char * filePrefixName);
	static void saveToFile(const MatrixXd &mat, const char* filePrefixName);
	static void saveToFile(const VectorXd & mat, const char * filePrefixName);
	static void saveToFile(const MatrixXd & mat1, const MatrixXd & mat2, const char * filePrefixName);
};

