#include "utils.h"
#include "niw.h"
#include "ds.h"
#include "vector"
#include <iostream>
#include "distributions/mv_gaussian.h"
#include "distributions_util/pdflib.hpp"
#include "distributions_util/logdet.h"
#include <iostream>
#include <fstream>
#include "cudaKernel_gaussian.cuh"

#define STATS_ENABLE_EIGEN_WRAPPERS
#define STATS_DONT_USE_OPENMP //due to Visual Studio bug. Should be fixed in VS 2019 16.10

#include "stats.hpp"

using namespace std;

/*
niw_hyperparams(k::Float32, m::AbstractArray{ Float32 }, v::Float32, psi::AbstractArray{ Float32 })

[Normal Inverse Wishart](https://en.wikipedia.org/wiki/Normal-inverse-Wishart_distribution)
*/

hyperparams* niw::calc_posterior(const hyperparams* hyperParams, const sufficient_statistics* suff_statistics)
{
	niw_hyperparams* pNiw_hyperparams = (niw_hyperparams*)hyperParams;
	if (suff_statistics->N == 0)
	{
		return pNiw_hyperparams->clone();
	}

	double k;
	VectorXd m;
	double v;
	MatrixXd psi;

	k = pNiw_hyperparams->k + suff_statistics->N;
	v = pNiw_hyperparams->v + suff_statistics->N;
	m = (pNiw_hyperparams->m*pNiw_hyperparams->k + suff_statistics->points_sum) / k;
	
	/*for (size_t i = 0; i < ((niw_sufficient_statistics*)suff_statistics)->S.rows(); i++)
	{
		for (size_t j = 0; j < ((niw_sufficient_statistics*)suff_statistics)->S.cols(); j++)
		{
			double raz = ((niw_sufficient_statistics*)suff_statistics)->S(i, j);
			std::cout << raz;

		}
	}*/

	MatrixXd tempMat = pNiw_hyperparams->v * pNiw_hyperparams->psi + pNiw_hyperparams->k*pNiw_hyperparams->m*pNiw_hyperparams->m.adjoint();
	psi = (tempMat - k * m*m.adjoint() + ((niw_sufficient_statistics*)suff_statistics)->S) / v;
	psi = psi.selfadjointView<Upper>();

	niw_hyperparams* hyper_params = new niw_hyperparams(k, m, v, psi);
	return hyper_params;
}

MatrixXd niw::inverseWishart(const MatrixXd& sigma, double v)
{
	return stats::rinvwish(sigma, v);
}

double* niw::multinormal_sample(int n, double mu[], double r[])
{
	return r8vec_multinormal_sample(n, mu, r);
}

distribution_sample* niw::sample_distribution(const hyperparams* pHyperparams, std::mt19937* gen)
{
	niw_hyperparams* pNiw_hyperparams;

	if (pHyperparams == NULL)
	{
		return new mv_gaussian();
	}
	else
	{
		pNiw_hyperparams = (niw_hyperparams*)pHyperparams;
	}

	MatrixXd niwSigma = pNiw_hyperparams->v* pNiw_hyperparams->psi;
	MatrixXd sigma = inverseWishart(niwSigma, pNiw_hyperparams->v);

	MatrixXd mat1 = (sigma / pNiw_hyperparams->k);
	LLT<MatrixXd> cholmat1(mat1);
	MatrixXd L = cholmat1.matrixL();
	double *mu = multinormal_sample(mat1.rows(), pNiw_hyperparams->m.data(), L.data());
    MatrixXd invSigma = sigma.inverse();
	LLT<MatrixXd, Upper> chol(invSigma.selfadjointView<Upper>()); // compute the Cholesky
	Eigen::VectorXd muV = Eigen::VectorXd::Map(mu, mat1.rows());
	return new mv_gaussian(muV, sigma, invSigma, logdet(sigma), chol);
}


sufficient_statistics* niw::create_sufficient_statistics(const hyperparams* hyperParams, const hyperparams* posterior, const MatrixXd &points)
{
	if (points.cols() == 0)
	{
		niw_hyperparams* pNiw_hyperparams = (niw_hyperparams*)hyperParams;
		return new niw_sufficient_statistics(points.cols(), VectorXd::Zero(pNiw_hyperparams->m.size()), MatrixXd::Zero(pNiw_hyperparams->m.size(), pNiw_hyperparams->m.size()));
	}

	return new niw_sufficient_statistics(points.cols(), points.rowwise().sum(), points * points.adjoint());
}


double niw::log_marginal_likelihood(const hyperparams* hyperParams, const hyperparams* posterior_hyper, const sufficient_statistics* suff_stats)
{
	niw_hyperparams* pNiw_hyperparams = (niw_hyperparams*)hyperParams;
	niw_hyperparams* pPosterior_hyper = (niw_hyperparams*)posterior_hyper;
	long D = suff_stats->points_sum.size();
	double logpi = log(EIGEN_PI);
	return -suff_stats->N*D*0.5*logpi +
		utils::log_multivariate_gamma(pPosterior_hyper->v / 2.0, D) -
		utils::log_multivariate_gamma(pNiw_hyperparams->v / 2.0, D) +
		(pNiw_hyperparams->v / 2.0)*(D*log(pNiw_hyperparams->v) + logdet(pNiw_hyperparams->psi)) -
		(pPosterior_hyper->v / 2.0)*(D*log(pPosterior_hyper->v) + logdet(pPosterior_hyper->psi)) +
		(D / 2.0)*(log(pNiw_hyperparams->k / pPosterior_hyper->k));
}


void niw::aggregate_suff_stats(sufficient_statistics* suff_l, sufficient_statistics* suff_r, sufficient_statistics* &suff_out)
{
	niw_sufficient_statistics* niw_suff_l = (niw_sufficient_statistics*)suff_l;
	niw_sufficient_statistics* niw_suff_r = (niw_sufficient_statistics*)suff_r;
	niw_sufficient_statistics* niw_suff_out = (niw_sufficient_statistics*)suff_out;
	niw_suff_out->N = niw_suff_l->N + niw_suff_r->N;
	niw_suff_out->points_sum = niw_suff_l->points_sum + niw_suff_r->points_sum;
	niw_suff_out->S = niw_suff_l->S + niw_suff_r->S;
}

cudaKernel* niw::get_cuda()
{
	return new cudaKernel_gaussian();
}
