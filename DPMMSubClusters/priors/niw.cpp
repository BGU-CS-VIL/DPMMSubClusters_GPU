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
#include "check_time.h"
#include "niw_hyperparams.h"
#include "niw_sufficient_statistics.h"

#define STATS_ENABLE_EIGEN_WRAPPERS
#define STATS_DONT_USE_OPENMP //due to Visual Studio bug. Should be fixed in VS 2019 16.10

#include "stats.hpp"

using namespace std;

//[Normal Inverse Wishart](https://en.wikipedia.org/wiki/Normal-inverse-Wishart_distribution)
std::shared_ptr<hyperparams> niw::calc_posterior(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<sufficient_statistics>& suff_statistics)
{
	//CHECK_TIME("niw::calc_posterior");
	niw_hyperparams* pNiw_hyperparams = dynamic_cast<niw_hyperparams*>(hyperParams.get());

	if (suff_statistics->N == 0)
	{
		return pNiw_hyperparams->clone();
	}

	niw_sufficient_statistics* ss = dynamic_cast<niw_sufficient_statistics*>(suff_statistics.get());

	double k;
	VectorXd m;
	double v;
	MatrixXd psi;

	k = pNiw_hyperparams->k + suff_statistics->N;
	v = pNiw_hyperparams->v + suff_statistics->N;
	m = (pNiw_hyperparams->m * pNiw_hyperparams->k + suff_statistics->points_sum) / k;

	MatrixXd tempMat = pNiw_hyperparams->v * pNiw_hyperparams->psi + pNiw_hyperparams->k * pNiw_hyperparams->m * pNiw_hyperparams->m.adjoint();
	psi = (tempMat - k * m * m.adjoint() + ss->S) / v;
	psi = psi.selfadjointView<Upper>();
	psi = (psi + psi.adjoint()) / 2;
	std::shared_ptr<niw_hyperparams> hyper_params = std::make_shared<niw_hyperparams>(k, m, v, psi);
	return hyper_params;
}

MatrixXd niw::inverseWishart(const MatrixXd& sigma, double v)
{
	//CHECK_TIME("niw::inverseWishart");
	return stats::rinvwish(sigma, v);
}

MatrixXd niw::wishart(const MatrixXd& sigma, double v)
{
	//CHECK_TIME("niw::wishart");
	return stats::rwish(sigma, v);
}

void niw::inverseWishartNoInverse(const MatrixXd& sigma, double v, std::unique_ptr < cudaKernel >& cuda, MatrixXd& matOut)
{
	//CHECK_TIME("niw::inverseWishartNoInverse");
	if (cuda == NULL)
	{
		matOut = stats::rinvwish(sigma, v, false, false);
	}
	else
	{
		MatrixXd B;// = sigma.inverse();
		cuda->inverse_matrix(sigma, B);

		B = B.llt().matrixL();
		cuda->multiplie_matrix_for_inverseWishart(B, stats::rinvwish2(B, v), matOut);
		/*B = B * stats::rinvwish2(v);

		return B * mat_ops::trans(B);*/


	}
	//else
	//{
	//	MatrixXd B;// = sigma.inverse();
	//	cuda->inverse_matrix(sigma, B);

	//	B = B.llt().matrixL();
	//	return stats::rinvwish(B, v, true, false);

	//}
}

double* niw::multinormal_sample(int n, double mu[], double r[])
{
	//CHECK_TIME("niw::multinormal_sample");
	return r8vec_multinormal_sample(n, mu, r);
}

std::shared_ptr<distribution_sample> niw::sample_distribution(const std::shared_ptr<hyperparams>& pHyperparams, std::unique_ptr<std::mt19937>& gen, std::unique_ptr < cudaKernel >& cuda)
{
	//CHECK_TIME("niw::sample_distribution");
	niw_hyperparams* pNiw_hyperparams;

	if (pHyperparams == NULL)
	{
		return std::make_shared<mv_gaussian>();
	}
	else
	{
		pNiw_hyperparams = dynamic_cast<niw_hyperparams*>(pHyperparams.get());
	}

	MatrixXd niwSigma = pNiw_hyperparams->v * pNiw_hyperparams->psi;
	//MatrixXd sigma = inverseWishart(niwSigma, pNiw_hyperparams->v);
	//MatrixXd invSigma = sigma.inverse();
	MatrixXd invSigma;
	inverseWishartNoInverse(niwSigma, pNiw_hyperparams->v, cuda, invSigma);
	MatrixXd sigma;// = invSigma.inverse();
	cuda->inverse_matrix(invSigma, sigma);
	MatrixXd mat1 = (sigma / pNiw_hyperparams->k);
	LLT<MatrixXd> cholmat1(mat1);
	MatrixXd L = cholmat1.matrixL();
	double* mu = multinormal_sample((int)mat1.rows(), pNiw_hyperparams->m.data(), L.data());
//	LLT<MatrixXd, Upper> chol(invSigma.selfadjointView<Upper>()); // compute the Cholesky
	LLT<MatrixXd, Upper> chol;
	Eigen::VectorXd muV = Eigen::VectorXd::Map(mu, mat1.rows());

	return std::make_shared<mv_gaussian>(muV, sigma, invSigma, logdet(sigma, true), chol);
}

std::shared_ptr<sufficient_statistics> niw::create_sufficient_statistics(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<hyperparams>& posterior, const MatrixXd& points)
{
	//CHECK_TIME("niw::create_sufficient_statistics");
	if (points.cols() == 0)
	{
		niw_hyperparams* pNiw_hyperparams = dynamic_cast<niw_hyperparams*>(hyperParams.get());
		return std::make_shared<niw_sufficient_statistics>((int)points.cols(), VectorXd::Zero(pNiw_hyperparams->m.size()), MatrixXd::Zero(pNiw_hyperparams->m.size(), pNiw_hyperparams->m.size()));
	}

	MatrixXd S;
	{
		//CHECK_TIME("niw::create_sufficient_statistics, calc S");
		S = points * points.adjoint();
		S = 0.5 * (S + S.adjoint());
	}
	return std::make_shared<niw_sufficient_statistics>((int)points.cols(), points.rowwise().sum(), S);
}


double niw::log_marginal_likelihood(const std::shared_ptr<hyperparams>& hyperParams, const std::shared_ptr<hyperparams>& posterior_hyper, const std::shared_ptr<sufficient_statistics>& suff_stats)
{
	//CHECK_TIME("niw::log_marginal_likelihood");
	niw_hyperparams* pNiw_hyperparams = dynamic_cast<niw_hyperparams*>(hyperParams.get());
	niw_hyperparams* pPosterior_hyper = dynamic_cast<niw_hyperparams*>(posterior_hyper.get());
	int D = (int)suff_stats->points_sum.size();
	double logpi = log(EIGEN_PI);

	double logdet_niw;
	{
		//CHECK_TIME("niw::log_marginal_likelihood logdet_niw");

		logdet_niw = logdet(pNiw_hyperparams->psi, true);
	}
	double logdet_posterior;
	{
		//CHECK_TIME("niw::log_marginal_likelihood logdet_posterior");
		logdet_posterior = logdet(pPosterior_hyper->psi, true);
	}

	return -suff_stats->N * D * 0.5 * logpi +
		utils::log_multivariate_gamma(pPosterior_hyper->v / 2.0, D) -
		utils::log_multivariate_gamma(pNiw_hyperparams->v / 2.0, D) +
		(pNiw_hyperparams->v / 2.0) * (D * log(pNiw_hyperparams->v) + logdet_niw) -
		(pPosterior_hyper->v / 2.0) * (D * log(pPosterior_hyper->v) + logdet_posterior) +
		(D / 2.0) * (log(pNiw_hyperparams->k / pPosterior_hyper->k));
}


void niw::aggregate_suff_stats(std::shared_ptr<sufficient_statistics>& suff_l, std::shared_ptr<sufficient_statistics>& suff_r, std::shared_ptr<sufficient_statistics>& suff_out)
{
	niw_sufficient_statistics* niw_suff_l = dynamic_cast<niw_sufficient_statistics*>(suff_l.get());
	niw_sufficient_statistics* niw_suff_r = dynamic_cast<niw_sufficient_statistics*>(suff_r.get());
	niw_sufficient_statistics* niw_suff_out = dynamic_cast<niw_sufficient_statistics*>(suff_out.get());
	niw_suff_out->N = niw_suff_l->N + niw_suff_r->N;
	niw_suff_out->points_sum = niw_suff_l->points_sum + niw_suff_r->points_sum;
	niw_suff_out->S = niw_suff_l->S + niw_suff_r->S;
}

std::unique_ptr<cudaKernel> niw::get_cuda()
{
	return std::make_unique<cudaKernel_gaussian>();
}

std::shared_ptr<hyperparams> niw::create_hyperparams(Json::Value& hyper_params_value)
{
	shared_ptr<niw_hyperparams> result = std::make_shared<niw_hyperparams>();
	result->k = hyper_params_value["k"].asDouble();

	Json::Value val = hyper_params_value["m"];
	int size = val.size();
	if (size > 0)
	{
		result->m.resize(size);
		for (int i = 0; i < size; i++)
		{
			result->m(i) = val.get(i, hyper_params_value["m"]).asDouble();
		}
	}

	result->v = hyper_params_value["v"].asDouble();

	val = hyper_params_value["psi"];
	int rows = val.size();
	if (rows > 0)
	{
		int cols = val.get((Json::ArrayIndex)0, hyper_params_value["psi"]).size();
		if (cols > 0)
		{
			result->psi.resize(rows, cols);
			for (int i = 0; i < rows; i++)
			{
				Json::Value valRow = val.get(i, hyper_params_value["psi"]);
				for (int j = 0; j < cols; j++)
				{
					result->psi(i, j) = valRow.get(j, valRow).asDouble();
				}
			}
		}
	}

	return result;
}

std::shared_ptr<hyperparams> niw::create_hyperparams(DimensionsType d)
{
	return std::make_shared<niw_hyperparams>(1.0, VectorXd::Zero(d), 5, MatrixXd::Identity(d, d));
}
