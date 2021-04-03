//#include <iostream>
#include "mv_gaussian.h"
//#include "utils.h"
//#include "Eigen/Dense"

//using namespace Eigen;


//void mv_gaussian::log_likelihood(cudaKernel* cuda, VectorXd &r, const MatrixXd &x, const distribution_sample *distribution_sample)
//{
//	const mv_gaussian *pDistribution_sample = (mv_gaussian*)distribution_sample;
//	MatrixXd z = x.colwise() - pDistribution_sample->mu;
//
//	cuda->dcolwise_dot(r, z, pDistribution_sample->invSigma);
//
//	double scalar = -((pDistribution_sample->sigma.size() * log(2 * EIGEN_PI) + pDistribution_sample->logdetSigma) / 2);
//	r = scalar * VectorXd::Ones(r.size()) - r / 2;
//}
