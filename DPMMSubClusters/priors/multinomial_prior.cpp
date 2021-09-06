#include <vector>
#include "multinomial_prior.h"
using namespace std;

#include "distributions_util/dirichlet.h"
#include "distributions_util/pdflib.hpp"
#include "distributions/multinomial_dist.h"
#include "cudaKernel_multinomial.cuh"
#include "multinomial_hyper.h"
#include "multinomial_sufficient_statistics.h"

std::shared_ptr<hyperparams> multinomial_prior::calc_posterior(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<sufficient_statistics>& suff_statistics)
{
	multinomial_hyper* p_hyperparams = dynamic_cast<multinomial_hyper*>(hyperParams.get());
	if (suff_statistics->N == 0)
	{
		return p_hyperparams->clone();
	}

	std::shared_ptr<hyperparams> hyper_params = std::make_shared<multinomial_hyper>(p_hyperparams->alpha + suff_statistics->points_sum);
	return hyper_params;
}

std::shared_ptr<distribution_sample> multinomial_prior::sample_distribution(const std::shared_ptr<hyperparams>& pHyperparams, std::unique_ptr<std::mt19937> &gen, std::unique_ptr<cudaKernel>& cuda)
{
	multinomial_hyper* pHyperParams;

	if (pHyperparams == NULL)
	{
		return std::make_shared<multinomial_dist>();
	}
	else
	{
		pHyperParams = dynamic_cast<multinomial_hyper*>(pHyperparams.get());
	}

	vector<double> alpha_vec(pHyperParams->alpha.data(), pHyperParams->alpha.data() + pHyperParams->alpha.rows() * pHyperParams->alpha.cols());

	// Dirichlet distribution using mt19937 rng
	dirichlet_distribution<std::mt19937> d(alpha_vec);
	std::shared_ptr<multinomial_dist> ms = std::make_shared<multinomial_dist>();
	ms->alpha = d(*gen);
	for (int i = 0; i < alpha_vec.size(); ++i)
	{
		ms->alpha[i] = log(ms->alpha[i]);
	}

	return ms;
}


std::shared_ptr<sufficient_statistics> multinomial_prior::create_sufficient_statistics(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<hyperparams>& posterior, const MatrixXd& points)
{
	return std::make_shared<multinomial_sufficient_statistics>((int)points.cols(), points.rowwise().sum());
}

double multinomial_prior::log_marginal_likelihood(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<hyperparams>& posterior_hyper, const std::shared_ptr<sufficient_statistics>& suff_stats)
{
	multinomial_hyper* prior = dynamic_cast<multinomial_hyper*>(hyperParams.get());
	multinomial_hyper* posterior = dynamic_cast<multinomial_hyper*>(posterior_hyper.get());

	double val = r8_gamma_log(prior->alpha.sum()) - r8_gamma_log(posterior->alpha.sum());
	for (int i = 0; i < prior->alpha.size(); i++)
	{
		val += r8_gamma_log(posterior->alpha(i)) - r8_gamma_log(prior->alpha(i));
	}
	return val;
}

void multinomial_prior::aggregate_suff_stats(std::shared_ptr<sufficient_statistics>& suff_l, std::shared_ptr<sufficient_statistics>& suff_r, std::shared_ptr<sufficient_statistics>& suff_out)
{
	suff_out->N = suff_l->N + suff_r->N;
	suff_out->points_sum = suff_l->points_sum + suff_r->points_sum;
}

std::unique_ptr<cudaKernel> multinomial_prior::get_cuda()
{
	return std::make_unique<cudaKernel_multinomial>();
}

std::shared_ptr<hyperparams> multinomial_prior::create_hyperparams(Json::Value& hyper_params_value)
{
	shared_ptr<multinomial_hyper> result = std::make_shared<multinomial_hyper>();

	Json::Value val = hyper_params_value["alpha"];
	int size = val.size();
	if (size > 0)
	{
		result->alpha.resize(size);
		for (int i = 0; i < size; i++)
		{
			result->alpha(i) = val.get(i, hyper_params_value["alpha"]).asDouble();
		}
	}

	return result;
}

std::shared_ptr<hyperparams> multinomial_prior::create_hyperparams(DimensionsType d)
{
	VectorXd alpha_vec = VectorXd::Zero(d);
	return std::make_shared<multinomial_hyper>(alpha_vec);
}