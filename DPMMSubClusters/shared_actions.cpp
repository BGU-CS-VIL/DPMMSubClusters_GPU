#include <string>
using namespace std;

#include "shared_actions.h"
#include "distributions_util/dirichlet.h"
#include "distributions_util/pdflib.hpp"
#include "prior.h"

void shared_actions::sample_cluster_params(std::shared_ptr<splittable_cluster_params>& params, double alpha, bool first)
{
	std::vector<double> points_count;

	params->cluster_params->distribution = first ? globalParams->pPrior->sample_distribution(params->cluster_params->prior_hyperparams, globalParams->gen, globalParams->cuda) : globalParams->pPrior->sample_distribution(params->cluster_params->posterior_hyperparams, globalParams->gen, globalParams->cuda);
	params->cluster_params_l->distribution = first ? globalParams->pPrior->sample_distribution(params->cluster_params_l->prior_hyperparams, globalParams->gen, globalParams->cuda) : globalParams->pPrior->sample_distribution(params->cluster_params_l->posterior_hyperparams, globalParams->gen, globalParams->cuda);
	params->cluster_params_r->distribution = first ? globalParams->pPrior->sample_distribution(params->cluster_params_r->prior_hyperparams, globalParams->gen, globalParams->cuda) : globalParams->pPrior->sample_distribution(params->cluster_params_r->posterior_hyperparams, globalParams->gen, globalParams->cuda);

	points_count.push_back(params->cluster_params_l->suff_statistics->N);
	points_count.push_back(params->cluster_params_r->suff_statistics->N);

	for (size_t i = 0; i < points_count.size(); i++)
	{
		points_count[i] += alpha / 2;
	}

	params->lr_weights = get_dirichlet_distribution(points_count);

	double log_likihood_l = globalParams->pPrior->log_marginal_likelihood(params->cluster_params_l->prior_hyperparams, params->cluster_params_l->posterior_hyperparams, params->cluster_params_l->suff_statistics);
	double log_likihood_r = globalParams->pPrior->log_marginal_likelihood(params->cluster_params_r->prior_hyperparams, params->cluster_params_r->posterior_hyperparams, params->cluster_params_r->suff_statistics);

	for (int i = 0; i < globalParams->burnout_period - 1; i++)
	{
		params->logsublikelihood_hist[i] = params->logsublikelihood_hist[i + 1];
	}

	params->logsublikelihood_hist[globalParams->burnout_period - 1].first = true;
	params->logsublikelihood_hist[globalParams->burnout_period - 1].second = log_likihood_l + log_likihood_r;
	double	logsublikelihood_now = 0.0;
	bool allReady = true;
	for (int i = 0; i < globalParams->burnout_period; i++)
	{
		if (params->logsublikelihood_hist[i].first)
		{
			logsublikelihood_now += params->logsublikelihood_hist[i].second * (1 / (globalParams->burnout_period - 0.1));
		}
		else
		{
			allReady = false;
		}
	}

	if (allReady && logsublikelihood_now - params->logsublikelihood_hist[globalParams->burnout_period - 1].second < 1e-2) // propogate abs change to other versions ?
	{
		params->splittable = true;
	}
}

void shared_actions::create_splittable_from_params(std::shared_ptr<cluster_parameters>& params, double alpha, std::shared_ptr<splittable_cluster_params>& scpOut)
{
	scpOut->cluster_params_l = params->clone();
	scpOut->cluster_params_l->distribution = globalParams->pPrior->sample_distribution(params->posterior_hyperparams, globalParams->gen, globalParams->cuda);
	scpOut->cluster_params_r = params->clone();
	scpOut->cluster_params_r->distribution = globalParams->pPrior->sample_distribution(params->posterior_hyperparams, globalParams->gen, globalParams->cuda);

	std::vector<double> alphas = std::vector<double>(2, alpha / 2);
	dirichlet_distribution<std::mt19937> d(alphas);
	scpOut->lr_weights = d(*globalParams->gen);
	scpOut->cluster_params = params->clone();
	scpOut->logsublikelihood_hist = Logsublikelihood_hist(globalParams->burnout_period + 5, std::make_pair(false, 0));
	scpOut->splittable = false;
}

void shared_actions::merge_clusters_to_splittable(std::shared_ptr<splittable_cluster_params>& scpl, std::shared_ptr<cluster_parameters>& cpr, double alpha)
{
	globalParams->pPrior->aggregate_suff_stats(scpl->cluster_params->suff_statistics, cpr->suff_statistics, scpl->cluster_params->suff_statistics);
	scpl->cluster_params->posterior_hyperparams = globalParams->pPrior->calc_posterior(scpl->cluster_params->prior_hyperparams, scpl->cluster_params->suff_statistics);
	std::vector<double> alphas;
	alphas.push_back(scpl->cluster_params->suff_statistics->N + (alpha / 2));
	alphas.push_back(cpr->suff_statistics->N + (alpha / 2));
	dirichlet_distribution<std::mt19937> d(alphas);
	scpl->lr_weights = d(*globalParams->gen);
	scpl->cluster_params_l = scpl->cluster_params->clone();
	scpl->cluster_params_r = cpr->clone();
	scpl->splittable = false;
	scpl->splittable = false;
	scpl->logsublikelihood_hist = Logsublikelihood_hist(globalParams->burnout_period + 5, std::make_pair(false, 0));
}

void shared_actions::should_merge(bool& should_merge, std::shared_ptr<cluster_parameters>& cpl, std::shared_ptr<cluster_parameters>& cpr, double alpha, bool bFinal)
{
	std::shared_ptr<cluster_parameters> cp = cpl->clone();
	globalParams->pPrior->aggregate_suff_stats(cpl->suff_statistics, cpr->suff_statistics, cp->suff_statistics);
	cp->posterior_hyperparams = globalParams->pPrior->calc_posterior(cp->prior_hyperparams, cp->suff_statistics);
	double log_likihood_l = globalParams->pPrior->log_marginal_likelihood(cpl->prior_hyperparams, cpl->posterior_hyperparams, cpl->suff_statistics);
	double log_likihood_r = globalParams->pPrior->log_marginal_likelihood(cpr->prior_hyperparams, cpr->posterior_hyperparams, cpr->suff_statistics);
	double log_likihood = globalParams->pPrior->log_marginal_likelihood(cp->prior_hyperparams, cp->posterior_hyperparams, cp->suff_statistics);

	double log_HR = (-log(alpha) + r8_gamma_log(alpha) - 2 * r8_gamma_log(0.5 * alpha) + r8_gamma_log(cp->suff_statistics->N) - r8_gamma_log(cp->suff_statistics->N + alpha) +
		r8_gamma_log(cpl->suff_statistics->N + 0.5 * alpha) - r8_gamma_log(cpl->suff_statistics->N) - r8_gamma_log(cpr->suff_statistics->N) +
		r8_gamma_log(cpr->suff_statistics->N + 0.5 * alpha) + log_likihood - log_likihood_l - log_likihood_r);

	double a_random_double = get_uniform_real_distribution();

	if ((log_HR > log(a_random_double)) || (bFinal && log_HR > log(0.1)))
	{
		should_merge = true;
	}
	else
	{
		should_merge = false;
	}
}

double shared_actions::get_uniform_real_distribution()
{
	std::uniform_real_distribution<double> dist(0, 1);
	return dist(*globalParams->gen);
}

std::vector<double> shared_actions::get_dirichlet_distribution(std::vector<double> &points_count)
{
	dirichlet_distribution<std::mt19937> d(points_count);
	return d(*globalParams->gen);
}