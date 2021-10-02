#pragma once

#include <random>
#include "Eigen/Dense"

using namespace Eigen;
#include "distribution_sample.h"
#include "sufficient_statistics.h"
#include "cudaKernel.cuh"
#include "hyperparams.h"
#include <json/json.h>

class prior
{
public:
	prior() {}
	prior* clone()//TODO - remove
	{
		return do_clone();
	}
	virtual ~prior(){}

	virtual	std::shared_ptr<hyperparams> calc_posterior(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<sufficient_statistics>& suff_statistics) = 0;
	virtual std::shared_ptr<distribution_sample> sample_distribution(const std::shared_ptr<hyperparams>& pHyperparams, std::unique_ptr<std::mt19937>& gen, std::unique_ptr<cudaKernel>& cuda) = 0;
	virtual std::shared_ptr<sufficient_statistics> create_sufficient_statistics(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<hyperparams>& posterior, const MatrixXd& points) = 0;
	virtual double log_marginal_likelihood(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<hyperparams>& posterior_hyper, const std::shared_ptr<sufficient_statistics>& suff_stats) = 0;
	virtual void aggregate_suff_stats(std::shared_ptr<sufficient_statistics>& suff_l, std::shared_ptr<sufficient_statistics>& suff_r, std::shared_ptr<sufficient_statistics>& suff_out) = 0;
	virtual prior* do_clone() = 0;
	virtual std::unique_ptr<cudaKernel> get_cuda() = 0;
	virtual std::shared_ptr<hyperparams> create_hyperparams(Json::Value& hyper_params_value) = 0;
	virtual std::shared_ptr<hyperparams> create_hyperparams(DimensionsType d) = 0;
};