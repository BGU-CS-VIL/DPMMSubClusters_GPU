#pragma once

#include "hyperparams.h"
#include "Eigen/Dense"
#include "json/json.h"

using namespace Eigen;

class niw_hyperparams : public hyperparams
{
public:
	niw_hyperparams() {}
	niw_hyperparams(double k, const VectorXd& m, double v, const MatrixXd& psi) : k(k), m(m), v(v), psi(psi) {}
	~niw_hyperparams() {}
	virtual void serialize(Json::Value& root)
	{
		root["k"] = k;

		size_t size = m.size();
		for (size_t i = 0; i < size; i++)
		{
			root["m"].append(m[i]);
		}

		root["v"] = v;

		size = psi.size();
		double* data = psi.data();
		for (size_t i = 0; i < size; i++)
		{
			root["psi"].append(data[i]);
		}
	}

	std::shared_ptr<hyperparams> clone() override
	{
		return std::make_shared<niw_hyperparams>(k, m, v, psi);
	}

	double k;
	VectorXd m;
	double v;
	MatrixXd psi;
};