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
	static prior* create_sufficient_statistics(hyperparams* dist, sufficient_statistics **suff_statistics, global_params *globalParams, const MatrixXd &pts = MatrixXd(0, 0));
	static void dcolwise_dot(VectorXd &r, const MatrixXd &a, const MatrixXd &b);
	static void saveToFile(const LabelsType & mat, const char * filePrefixName);
	static void saveToFile(const MatrixXd &mat, const char* filePrefixName);
	static void saveToFile(const VectorXd & mat, const char * filePrefixName);
	static void saveToFile(const MatrixXd & mat1, const MatrixXd & mat2, const char * filePrefixName);
};

