#pragma once
#include "prior.h"
#include "Eigen/Dense"
#include "ds.h"
//#include "global_params.h"

using namespace Eigen;

class multinomial_sufficient_statistics : public sufficient_statistics
{
public:
	multinomial_sufficient_statistics() {}
	multinomial_sufficient_statistics(double N, VectorXd points_sum) : sufficient_statistics(N, points_sum) {}
	sufficient_statistics *clone()
	{
		multinomial_sufficient_statistics *ss = new multinomial_sufficient_statistics();
		ss->N = N;
		ss->points_sum = points_sum;
		return ss;
	}
};

class multinomial_hyper : public hyperparams
{
public:
	multinomial_hyper(VectorXd alpha) : hyperparams(), alpha(alpha) {}

	virtual hyperparams* clone()
	{
		return new multinomial_hyper(alpha);
	}

	VectorXd alpha;
};

class multinomial_prior : public prior
{
public:
	multinomial_prior() {}
	virtual ~multinomial_prior()
	{
		//if (ss != NULL)
		//{
		//	delete ss;
		//	ss = NULL;
		//}

		//if (hyper_params != NULL)
		//{
		//	delete hyper_params;
		//	hyper_params = NULL;
		//}
	}

	multinomial_prior(const multinomial_prior& mp2) {}
	prior *do_clone()
	{
		multinomial_prior *pMp = new multinomial_prior();
//		pMp->ss = ss != NULL ? ss->clone() : NULL;
//		pMp->hyper_params = hyper_params != NULL ? hyper_params->clone() : NULL;
		return pMp;
	}
		
	hyperparams* calc_posterior(const hyperparams* hyperParams, const sufficient_statistics* suff_statistics) override;
	distribution_sample* sample_distribution(const hyperparams* pHyperparams, std::mt19937* gen) override;
	sufficient_statistics* create_sufficient_statistics(const hyperparams* hyperParams, const hyperparams* posterior, const MatrixXd &points) override;
	double log_marginal_likelihood(const hyperparams* hyperParams, const hyperparams* posterior_hyper, const sufficient_statistics* suff_stats) override;
	void aggregate_suff_stats(sufficient_statistics* suff_l, sufficient_statistics* suff_r, sufficient_statistics* &suff_out) override;
	cudaKernel* get_cuda() override;

private:
};

