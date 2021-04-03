#pragma once
//#include "Eigen/Dense"
//#include "cudaKernel.cuh"

//using namespace Eigen;

class distribution_sample
{
public:
	virtual distribution_sample *clone() = 0;
//	virtual void log_likelihood(cudaKernel* cuda, VectorXd& r, const MatrixXd& x, const distribution_sample* distribution_sample) = 0;
};

