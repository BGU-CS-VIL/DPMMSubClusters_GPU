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
	std::shared_ptr<distribution_sample> clone()
	{
		std::shared_ptr<mv_gaussian> pmv_gaussian = std::make_shared<mv_gaussian>();

		pmv_gaussian->mu = mu;
		pmv_gaussian->sigma = sigma;
		pmv_gaussian->invSigma = invSigma;
		pmv_gaussian->logdetSigma = logdetSigma;
		pmv_gaussian->invChol = invChol;
		return pmv_gaussian;
	}
	virtual void serialize(Json::Value& root)
	{
		int size = mu.size();
		double* data = mu.data();
		for (int i = 0; i < size; i++)
		{
			root["mu"].append(data[i]);
		}

		size = sigma.size();
		data = sigma.data();
		for (int i = 0; i < size; i++)
		{
			root["sigma"].append(data[i]);
		}

		size = invSigma.size();
		data = invSigma.data();
		for (int i = 0; i < size; i++)
		{
			root["invSigma"].append(data[i]);
		}

		root["logdetSigma"] = logdetSigma;
	}
	virtual void deserialize(Json::Value& root)
	{
		//TODO
	}

	VectorXd mu;
	MatrixXd sigma;
	MatrixXd invSigma;
	double logdetSigma;
	LLT<MatrixXd, Upper> invChol;
};