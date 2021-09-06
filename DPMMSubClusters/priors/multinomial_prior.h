#pragma once
#include "prior.h"
#include "Eigen/Dense"
#include "ds.h"

using namespace Eigen;

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
	std::shared_ptr<distribution_sample> sample_distribution(const std::shared_ptr<hyperparams>& pHyperparams, std::unique_ptr<std::mt19937> &gen, std::unique_ptr<cudaKernel>& cuda) override;
	std::shared_ptr<sufficient_statistics> create_sufficient_statistics(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<hyperparams>& posterior, const MatrixXd &points) override;
	double log_marginal_likelihood(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<hyperparams>& posterior_hyper, const std::shared_ptr<sufficient_statistics>& suff_stats) override;
	void aggregate_suff_stats(std::shared_ptr<sufficient_statistics>& suff_l, std::shared_ptr<sufficient_statistics>& suff_r, std::shared_ptr<sufficient_statistics>& suff_out) override;
	std::unique_ptr<cudaKernel> get_cuda() override;
	std::shared_ptr<hyperparams> create_hyperparams(Json::Value& hyper_params_value) override;
	std::shared_ptr<hyperparams> create_hyperparams(DimensionsType d) override;

private:
};

