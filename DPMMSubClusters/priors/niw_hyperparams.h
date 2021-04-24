#pragma once

#include "hyperparams.h"
#include "Eigen/Dense"

using namespace Eigen;

class niw_hyperparams : public hyperparams
{
public:
	niw_hyperparams() {}
	niw_hyperparams(double k, const VectorXd& m, double v, const MatrixXd& psi) : k(k), m(m), v(v), psi(psi) {}
	~niw_hyperparams() {}

	std::shared_ptr<hyperparams> clone() override
	{
		return std::make_shared<niw_hyperparams>(k, m, v, psi);
	}

	double k;
	VectorXd m;
	double v;
	MatrixXd psi;
};