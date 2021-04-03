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
	niw_sufficient_statistics(double N, const VectorXd &points_sum, const MatrixXd &S) : sufficient_statistics(N, points_sum), S(S) 
	{
	}

	sufficient_statistics *clone()
	{
		niw_sufficient_statistics *ss = new niw_sufficient_statistics();
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

	virtual hyperparams* clone()
	{
		return new niw_hyperparams(k, m, v, psi);
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
		//if (ss != NULL)
		//{
		//	delete ss;
		//	ss = NULL;
		//}

	/*	if (hyper_params != NULL)
		{
			delete hyper_params;
			hyper_params = NULL;
		}*/
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
		/*if (niwObj.hyper_params != NULL)
		{
			hyper_params = niwObj.hyper_params->clone();
		}
		else
		{
			hyper_params = NULL;
		}*/
	}

	hyperparams* calc_posterior(const hyperparams* hyperParams, const sufficient_statistics* suff_statistics);
	distribution_sample* sample_distribution(const hyperparams* pHyperparams, std::mt19937* gen);
	sufficient_statistics* create_sufficient_statistics(const hyperparams* hyperParams, const hyperparams* posterior, const MatrixXd &points);
	double log_marginal_likelihood(const hyperparams* hyperParams, const hyperparams* posterior_hyper, const sufficient_statistics* suff_stats) override;
	void aggregate_suff_stats(sufficient_statistics* suff_l, sufficient_statistics* suff_r, sufficient_statistics* &suff_out);
	cudaKernel* get_cuda() override;

protected:
	virtual MatrixXd inverseWishart(const MatrixXd& sigma, double v);
	virtual double* multinormal_sample(int n, double mu[], double r[]);

};

