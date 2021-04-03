#include <vector>
#include "multinomial_prior.h"
using namespace std;

#include "distributions_util/dirichlet.h"
#include "distributions_util/pdflib.hpp"
#include "distributions/multinomial_dist.h"
#include "cudaKernel_multinomial.cuh"

hyperparams* multinomial_prior::calc_posterior(const hyperparams* hyperParams, const sufficient_statistics* suff_statistics)
{
	multinomial_hyper* p_hyperparams = (multinomial_hyper*)hyperParams;
	if (suff_statistics->N == 0)
	{
		return p_hyperparams->clone();
	}

	multinomial_hyper* hyper_params = new multinomial_hyper(((multinomial_hyper*)hyperParams)->alpha + suff_statistics->points_sum);
	return hyper_params;
}

distribution_sample* multinomial_prior::sample_distribution(const hyperparams* pHyperparams, std::mt19937* gen)
{
	multinomial_hyper* pHyperParams;

	if (pHyperparams == NULL)
	{
		return new multinomial_dist();
	}
	else
	{
		pHyperParams = (multinomial_hyper*)pHyperparams;
	}

	vector<double> alpha_vec(pHyperParams->alpha.data(), pHyperParams->alpha.data() + pHyperParams->alpha.rows() * pHyperParams->alpha.cols());

	// Dirichlet distribution using mt19937 rng
	dirichlet_distribution<std::mt19937> d(alpha_vec);
	multinomial_dist* ms = new multinomial_dist();
	ms->alpha = d(*gen);
	for (int i = 0; i < alpha_vec.size(); ++i)
	{
		ms->alpha[i] = log(ms->alpha[i]);
	}

	return ms;
}


sufficient_statistics* multinomial_prior::create_sufficient_statistics(const hyperparams* hyperParams, const hyperparams* posterior, const MatrixXd &points)
{
	return new multinomial_sufficient_statistics(points.cols(), points.rowwise().sum());
}

double multinomial_prior::log_marginal_likelihood(const hyperparams* hyperParams, const hyperparams* posterior_hyper, const sufficient_statistics* suff_stats)
{
	multinomial_hyper* prior = (multinomial_hyper*)hyperParams;
	multinomial_hyper* posterior = (multinomial_hyper*)posterior_hyper;

	double val = r8_gamma_log(prior->alpha.sum()) - r8_gamma_log(posterior->alpha.sum());
	for (int i = 0; i < prior->alpha.size(); i++)
	{
		val += r8_gamma_log(posterior->alpha(i)) - r8_gamma_log(prior->alpha(i));
	}
	return val;
}

void multinomial_prior::aggregate_suff_stats(sufficient_statistics* suff_l, sufficient_statistics* suff_r, sufficient_statistics* &suff_out)
{
	suff_out->N = suff_l->N + suff_r->N;
	suff_out->points_sum = suff_l->points_sum + suff_r->points_sum;
}

cudaKernel* multinomial_prior::get_cuda()
{
	return new cudaKernel_multinomial();
}