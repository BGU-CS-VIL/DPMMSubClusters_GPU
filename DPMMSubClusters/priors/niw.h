#pragma once
#include "prior.h"
#include "ds.h"
#include "distributions/mv_gaussian.h"
#include "Eigen/Dense"

using namespace Eigen;
using namespace std;

class niw :	public prior
{
public:
	niw() {}
	niw(const niw& niwObj) 
	{
		copy(niwObj);
	}
	virtual ~niw()
	{
	}

	niw *do_clone()
	{
		niw *pNiw = new niw();
		pNiw->copy(*this);
		return pNiw;
	}

	niw& operator=(niw other)
	{
		copy(other);
		return *this;
	}
	
	void copy(const niw &niwObj)
	{
	}

	std::shared_ptr<hyperparams> calc_posterior(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<sufficient_statistics>& suff_statistics) override;
	std::shared_ptr<distribution_sample> sample_distribution(const std::shared_ptr<hyperparams>& pHyperparams, std::unique_ptr<std::mt19937>& gen, std::unique_ptr<cudaKernel> &cuda) override;
	std::shared_ptr<sufficient_statistics> create_sufficient_statistics(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<hyperparams>& posterior, const MatrixXd& points) override;
	double log_marginal_likelihood(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<hyperparams>& posterior_hyper, const std::shared_ptr<sufficient_statistics>& suff_stats) override;
	void aggregate_suff_stats(std::shared_ptr<sufficient_statistics>& suff_l, std::shared_ptr<sufficient_statistics>& suff_r, std::shared_ptr<sufficient_statistics>& suff_out) override;
	std::unique_ptr<cudaKernel> get_cuda() override;
	std::shared_ptr<hyperparams> create_hyperparams(Json::Value& hyper_params_value) override;
	std::shared_ptr<hyperparams> create_hyperparams() override;

protected:
	virtual MatrixXd inverseWishart(const MatrixXd& sigma, double v);
	virtual MatrixXd wishart(const MatrixXd& sigma, double v);
	virtual void inverseWishartNoInverse(const MatrixXd& sigma, double v, std::unique_ptr < cudaKernel >& cuda, MatrixXd& matOut);

	virtual double* multinormal_sample(int n, double mu[], double r[]);

};

