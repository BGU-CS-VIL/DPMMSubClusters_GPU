#include <string>
using namespace std;

#include "shared_actions.h"
#include "distributions_util/dirichlet.h"
#include "distributions_util/pdflib.hpp"
#include "prior.h"


void shared_actions::sample_cluster_params(splittable_cluster_params* &params, float alpha, bool first, prior *pPrior)
{	  
	std::vector<double> points_count;
	params->cluster_params->distribution = first ? pPrior->sample_distribution(params->cluster_params->prior_hyperparams, globalParams->gen) : pPrior->sample_distribution(params->cluster_params->posterior_hyperparams, globalParams->gen);
	params->cluster_params_l->distribution = first ? pPrior->sample_distribution(params->cluster_params_l->prior_hyperparams, globalParams->gen) : pPrior->sample_distribution(params->cluster_params_l->posterior_hyperparams, globalParams->gen);
	params->cluster_params_r->distribution = first ? pPrior->sample_distribution(params->cluster_params_r->prior_hyperparams, globalParams->gen) : pPrior->sample_distribution(params->cluster_params_r->posterior_hyperparams, globalParams->gen);
	points_count.push_back(params->cluster_params_l->suff_statistics->N);
	points_count.push_back(params->cluster_params_r->suff_statistics->N);

	for (size_t i = 0; i < points_count.size(); i++)
	{
		points_count[i] += alpha / 2;
	}

	params->lr_weights = get_dirichlet_distribution(points_count);

	double log_likihood_l = pPrior->log_marginal_likelihood(params->cluster_params_l->prior_hyperparams, params->cluster_params_l->posterior_hyperparams, params->cluster_params_l->suff_statistics);
	double log_likihood_r = pPrior->log_marginal_likelihood(params->cluster_params_r->prior_hyperparams, params->cluster_params_r->posterior_hyperparams, params->cluster_params_r->suff_statistics);

	//niw_sufficient_statistics* ss = (niw_sufficient_statistics*)params->cluster_params_l->suff_statistics;
	//std::cout << "N of l:" << ss->N << std::endl;
	//std::cout << "points_sum of l:" << ss->points_sum << std::endl;
	//std::cout << "S of l:" << ss->S << std::endl;
	//niw_hyperparams * nh = (niw_hyperparams*)(params->cluster_params_l->posterior_hyperparams);
		//std::cout << "k of r:" << nh->k << std::endl;
		//std::cout << "m of r:" << nh->m << std::endl;
		//std::cout << "v of r:" << nh->v << std::endl;
		//std::cout << "psi of r:" << nh->psi << std::endl;

		
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
			logsublikelihood_now += params->logsublikelihood_hist[i].second* (1 / (globalParams->burnout_period - 0.1));
		}
		else 
		{
			allReady = false;
		}
	}

//		std::cout << "allReady:" << logsublikelihood_now - params->logsublikelihood_hist[globalParams->burnout_period - 1].second << std::endl;

//	std::cout << "logArray:" << std::endl;
//	std::vector<float> logArray;
//	for (size_t i = 0; i < params->logsublikelihood_hist.size()	; i++)
//	{
//		logArray.push_back(params->logsublikelihood_hist[i].second);
//		printf("%f ", params->logsublikelihood_hist[i].second);
//	}
	if (allReady)
	{
	}

	if (allReady && logsublikelihood_now - params->logsublikelihood_hist[globalParams->burnout_period - 1].second < 1e-2) // propogate abs change to other versions ?
	{
		params->splittable = true;
	}

}

void shared_actions::create_splittable_from_params(prior *pPrior, cluster_parameters* &params, double alpha, splittable_cluster_params* &scpOut)
{
	scpOut->cluster_params_l = params->clone();
	scpOut->cluster_params_l->distribution = pPrior->sample_distribution(params->posterior_hyperparams, globalParams->gen);
	scpOut->cluster_params_r = params->clone();
	scpOut->cluster_params_r->distribution = pPrior->sample_distribution(params->posterior_hyperparams, globalParams->gen);

	std::vector<double> alphas = std::vector<double>(2, alpha / 2);
	dirichlet_distribution<std::mt19937> d(alphas);
	scpOut->lr_weights = d(*globalParams->gen);
	scpOut->cluster_params = params->clone();
	scpOut->logsublikelihood_hist = Logsublikelihood_hist(globalParams->burnout_period + 5, std::make_pair(false, 0));
	scpOut->splittable = false;
}



void shared_actions::merge_clusters_to_splittable(prior *pPrior, splittable_cluster_params* &scpl, cluster_parameters* &cpr, double alpha)
{
	pPrior->aggregate_suff_stats(scpl->cluster_params->suff_statistics, cpr->suff_statistics, scpl->cluster_params->suff_statistics);
	scpl->cluster_params->posterior_hyperparams = pPrior->calc_posterior(scpl->cluster_params->prior_hyperparams, scpl->cluster_params->suff_statistics);
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


void shared_actions::should_merge(prior *pPrior, bool &should_merge, cluster_parameters* &cpl, cluster_parameters* &cpr, double alpha, bool bFinal)
{
	cluster_parameters *cp = cpl->clone();
	pPrior->aggregate_suff_stats(cpl->suff_statistics, cpr->suff_statistics, cp->suff_statistics);
	cp->posterior_hyperparams = pPrior->calc_posterior(cp->prior_hyperparams, cp->suff_statistics);
	double log_likihood_l = pPrior->log_marginal_likelihood(cpl->prior_hyperparams, cpl->posterior_hyperparams, cpl->suff_statistics);
	double log_likihood_r = pPrior->log_marginal_likelihood(cpr->prior_hyperparams, cpr->posterior_hyperparams, cpr->suff_statistics);
	double log_likihood = pPrior->log_marginal_likelihood(cp->prior_hyperparams, cp->posterior_hyperparams, cp->suff_statistics);
	//TODO - should be logabsgamma <= abs
	double log_HR = (-log(alpha) + r8_gamma_log(alpha) - 2 * r8_gamma_log(0.5*alpha) + r8_gamma_log(cp->suff_statistics->N) - r8_gamma_log(cp->suff_statistics->N + alpha) +
		r8_gamma_log(cpl->suff_statistics->N + 0.5*alpha) - r8_gamma_log(cpl->suff_statistics->N) - r8_gamma_log(cpr->suff_statistics->N) +
		r8_gamma_log(cpr->suff_statistics->N + 0.5*alpha) + log_likihood - log_likihood_l - log_likihood_r);

	std::uniform_real_distribution<double> dist(0, 1);
	double a_random_double = dist(*globalParams->gen);

	if ((log_HR > log(a_random_double)) || (bFinal && log_HR > log(0.1)))
	{
		should_merge = true;
	}
	else 
	{
		should_merge = false;
	}
}

std::vector<double> shared_actions::get_dirichlet_distribution(std::vector<double> &points_count)
{
	dirichlet_distribution<std::mt19937> d(points_count);
	return d(*globalParams->gen);
}