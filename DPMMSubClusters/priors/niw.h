#pragma once
#include "prior.h"
#include "ds.h"
#include "distributions/mv_gaussian.h"
#include "Eigen/Dense"

using namespace Eigen;
using namespace std;

class niw_sufficient_statistics : public sufficient_statistics
{
public:
	niw_sufficient_statistics() {}
	niw_sufficient_statistics(int N, const VectorXd &points_sum, const MatrixXd &S) : sufficient_statistics(N, points_sum), S(S) 
	{
	}

	std::shared_ptr<sufficient_statistics> clone()
	{
		std::shared_ptr<niw_sufficient_statistics> ss = std::make_shared<niw_sufficient_statistics>();

		ss->N = N;
		ss->points_sum = points_sum;
		ss->S = S;
		return ss;
	}
	MatrixXd S;
};

class niw_hyperparams : public hyperparams
{
public:
	niw_hyperparams(double k, const VectorXd &m, double v, const MatrixXd &psi) : k(k), m(m), v(v), psi(psi) {}
	~niw_hyperparams() {}

	std::shared_ptr<hyperparams> clone() override
	{
		return std::make_shared<niw_hyperparams>(k, m, v, psi);
	}
	
	double k;
	VectorXd m;
	double v;
	MatrixXd psi;
};

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
	std::shared_ptr<distribution_sample> sample_distribution(const std::shared_ptr<hyperparams>& pHyperparams, std::unique_ptr<std::mt19937>& gen) override;
	std::shared_ptr<sufficient_statistics> create_sufficient_statistics(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<hyperparams>& posterior, const MatrixXd& points) override;
	double log_marginal_likelihood(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<hyperparams>& posterior_hyper, const std::shared_ptr<sufficient_statistics>& suff_stats) override;
	void aggregate_suff_stats(std::shared_ptr<sufficient_statistics>& suff_l, std::shared_ptr<sufficient_statistics>& suff_r, std::shared_ptr<sufficient_statistics>& suff_out) override;
	std::unique_ptr<cudaKernel> get_cuda() override;

protected:
	virtual MatrixXd inverseWishart(const MatrixXd& sigma, double v);
	virtual double* multinormal_sample(int n, double mu[], double r[]);

};

