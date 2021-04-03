#pragma once

#include <random>
#include "Eigen/Dense"

using namespace Eigen;
#include "distribution_sample.h"
#include "sufficient_statistics.h"
#include "cudaKernel.cuh"
#include "hyperparams.h"



class prior 
{
public:
	prior() {}//: hyper_params(NULL) {}
	prior *clone()
	{
		return do_clone();
	}
	virtual ~prior() 
	{
		
	}
	virtual	hyperparams* calc_posterior(const hyperparams* hyperParams, const sufficient_statistics* suff_statistics) = 0;
	virtual distribution_sample* sample_distribution(const hyperparams* pHyperparams, std::mt19937* gen) = 0;
	virtual sufficient_statistics* create_sufficient_statistics(const hyperparams* hyperParams, const hyperparams* posterior, const MatrixXd &points) = 0;
	virtual double log_marginal_likelihood(const hyperparams* hyperParams, const hyperparams* posterior_hyper, const sufficient_statistics* suff_stats) = 0;
	virtual void aggregate_suff_stats(sufficient_statistics* suff_l, sufficient_statistics* suff_r, sufficient_statistics* &suff_out) = 0;
	virtual prior *do_clone() = 0;
	virtual cudaKernel *get_cuda() = 0;

//	hyperparams *hyper_params;
};

