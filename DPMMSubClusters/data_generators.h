#pragma once
#include <ctime> 
using namespace std;
#include "distributions_util/dirichlet.h"
#include "distributions_util/pdflib.hpp"
#include "Eigen/Dense"
using namespace Eigen;
#define STATS_ENABLE_EIGEN_WRAPPERS
#define STATS_DONT_USE_OPENMP //due to Visual Studio bug. Should be fixed in VS 2019 16.10
#include "stats.hpp"
#include "moduleTypes.h"
class data_generators
{
public:
	//Generate `N` observations, generated from `K` `D` dimensions Gaussians, with the Gaussian means sampled from a `Normal` distribution with mean `0` and `MixtureVar` variance.
	void generate_gaussian_data(PointType N, DimensionsType D, BaseType K, double MixtureVar, MatrixXd& x,
		LabelsType& tz,
		double**& tmean,
		double**& tcov)
	{
		printf("\nN=%ld, D=%ld, K=%ld\n", N, D, K);
		x.resize(D, N);
		tmean = new double* [D];
		for (DimensionsType i = 0; i < D; ++i)
		{
			tmean[i] = new double[K]();
		}
		tcov = new double* [D];
		for (DimensionsType i = 0; i < D; ++i)
		{
			tcov[i] = new double[K]();
		}
		srand((unsigned)time(NULL));
		std::mt19937 gen(rand());
		std::vector<double> ones = std::vector<double>(K, 1);

		// Dirichlet distribution using mt19937 rng
		dirichlet_distribution<std::mt19937> d(ones);
		std::vector<double> tpi = d(gen);
		double* p = new double[K];
		for (int i = 0; i < K; ++i)
		{
			p[i] = tpi[i];
		}

		int* tzn = i4vec_multinomial_sample((int)N, p, K);
		delete[]p;
		tz.resize(N);

		int ind = 1;
		for (int i = 1; i <= K; ++i)
		{
			int from = ind - 1;
			int to = ind + tzn[i - 1] - 1;
			for (int j = from; j < to; j++)
			{
				tz[j] = i;
			}
			double* mu = new double[D]();

			LLT<MatrixXd> chol(MatrixXd::Identity(D, D) * MixtureVar); // compute the Cholesky decomposition of A
			MatrixXd L = chol.matrixL();

			double* curTmean = r8vec_multinormal_sample((int)D, mu, L.data());
			for (int l = 0; l < D; l++)
			{
				tmean[l][i - 1] = curTmean[l];
			}

			MatrixXd r = MatrixXd::Identity(D, D);

			MatrixXd niwMat = stats::rinvwish(r, D + 2);
			LLT<MatrixXd> cholniw(niwMat);
			L = cholniw.matrixL();

			for (int j = from; j < to; j++)
			{
				double* d = r8vec_multinormal_sample((int)D, curTmean, L.data());
				for (int l = 0; l < D; l++)
				{
					x(l, j) = d[l];
				}
				delete[]d;
			}

			delete[]mu;
			delete[]curTmean;

			ind += tzn[i - 1];
		}

		printf("\nX1:\n");
		for (int i = 0; i < N && i < 10; ++i)
		{
			printf(" %f ", x(0, i));
		}
		printf("\nX2:\n");
		for (int i = 0; i < N && i < 10; ++i)
		{
			printf(" %f ", x(1, i));
		}
		printf("\n");
		delete[]tzn;
	}

	void generate_mnmm_data(PointType N, DimensionsType D, BaseType K, BaseType trials,
		MatrixXd& x,
		LabelsType& labels,
		MatrixXd& clusters)
	{
		clusters.resize(D, K);
		x.resize(D, N);

		for (int i = 0; i < N; i++)
		{
			labels.push_back(rand() % K + 1);
		}

		srand((unsigned)time(NULL));
		std::mt19937 gen(rand());
		for (int i = 0; i < K; i++)
		{
			std::vector<double> alphas;
			for (int j = 0; j < D; j++)
			{
				alphas.push_back(rand() % 20 + 1);
			}
			alphas[i] = rand() % 100 + 30;

			dirichlet_distribution<std::mt19937> d(alphas);
			clusters.col(i) = Eigen::Map<Eigen::VectorXd>(d(gen).data(), D);
		}

		for (int i = 0; i < N; i++)
		{
			int* multinomial = i4vec_multinomial_sample(trials, clusters.col(labels[i] - 1).data(), D);
			for (int j = 0; j < D; j++)
			{
				x(j, i) = multinomial[j];
			}
			delete[]multinomial;
		}
	}
};