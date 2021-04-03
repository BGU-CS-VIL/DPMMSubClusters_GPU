#pragma once

#include "Eigen/Dense"
#include "ds.h"
#include "distribution_sample.h"

using namespace Eigen;
using namespace std;

class mv_gaussian : public distribution_sample
{
public:
	mv_gaussian() {	}
	mv_gaussian(const VectorXd &mu, const MatrixXd &sigma, const MatrixXd &invSigma, double logdetSigma, const LLT<MatrixXd, Upper> &invChol) : mu(mu), sigma(sigma), invSigma(invSigma), logdetSigma(logdetSigma), invChol(invChol) {}
//	void log_likelihood(cudaKernel* cuda, VectorXd &r, const MatrixXd &x, const distribution_sample *distribution_sample) override;
	distribution_sample *clone()
	{
		mv_gaussian *pmv_gaussian = new mv_gaussian();
		pmv_gaussian->mu = mu;
		pmv_gaussian->sigma = sigma;
		pmv_gaussian->invSigma = invSigma;
		pmv_gaussian->logdetSigma = logdetSigma;
		pmv_gaussian->invChol = invChol;
		return pmv_gaussian;
	}
	VectorXd mu;
	MatrixXd sigma;
	MatrixXd invSigma;
	double logdetSigma;
	LLT<MatrixXd, Upper> invChol;
};