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
	multinomial_sufficient_statistics(int N, VectorXd points_sum) : sufficient_statistics(N, points_sum) {}
	std::shared_ptr<sufficient_statistics> clone()
	{
		std::shared_ptr<multinomial_sufficient_statistics> ss = std::make_shared<multinomial_sufficient_statistics>();

		ss->N = N;
		ss->points_sum = points_sum;
		return ss;
	}
};

class multinomial_hyper : public hyperparams
{
public:
	multinomial_hyper(VectorXd alpha) : hyperparams(), alpha(alpha) {}

	std::shared_ptr<hyperparams> clone() override
	{
		return std::make_shared<multinomial_hyper>(alpha);
	}

	VectorXd alpha;
};

class multinomial_prior : public prior
{
public:
	multinomial_prior() {}
	virtual ~multinomial_prior()
	{
	}

	multinomial_prior(const multinomial_prior& mp2) {}
	prior *do_clone()
	{
		multinomial_prior *pMp = new multinomial_prior();
		return pMp;
	}
		
	std::shared_ptr<hyperparams> calc_posterior(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<sufficient_statistics>& suff_statistics) override;
	std::shared_ptr<distribution_sample> sample_distribution(const std::shared_ptr<hyperparams>& pHyperparams, std::unique_ptr<std::mt19937> &gen) override;
	std::shared_ptr<sufficient_statistics> create_sufficient_statistics(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<hyperparams>& posterior, const MatrixXd &points) override;
	double log_marginal_likelihood(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<hyperparams>& posterior_hyper, const std::shared_ptr<sufficient_statistics>& suff_stats) override;
	void aggregate_suff_stats(std::shared_ptr<sufficient_statistics>& suff_l, std::shared_ptr<sufficient_statistics>& suff_r, std::shared_ptr<sufficient_statistics>& suff_out) override;
	std::unique_ptr<cudaKernel> get_cuda() override;

private:
};

