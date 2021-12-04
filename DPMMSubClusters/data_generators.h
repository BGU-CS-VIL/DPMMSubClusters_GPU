#pragma once
#include <memory>
using namespace std;
#include "Eigen/Dense"
using namespace Eigen;
#include "moduleTypes.h"

class data_generators
{
public:
	//Generate `N` observations, generated from `K` `D` dimensions Gaussians, with the Gaussian means sampled from a `Normal` distribution with mean `0` and `MixtureVar` variance.
	void generate_gaussian_data(PointType N, DimensionsType D, BaseType K, double MixtureVar, MatrixXd& x,
		std::shared_ptr<LabelsType>& tz,
		double**& tmean,
		double**& tcov);

	void generate_mnmm_data(PointType N, DimensionsType D, BaseType K, BaseType trials,
		MatrixXd& x,
		LabelsType& labels,
		MatrixXd& clusters);
};