#pragma once

#include "hyperparams.h"
#include "Eigen/Dense"
#include "json/json.h"

using namespace Eigen;

class multinomial_hyper : public hyperparams
{
public:
	multinomial_hyper() {}
	multinomial_hyper(VectorXd alpha) : hyperparams(), alpha(alpha) {}
	~multinomial_hyper() {}
	virtual void serialize(Json::Value& root)
	{
		size_t size = alpha.size();
		for (int i = 0; i < size; i++)
		{
			root["alpha"].append(alpha[i]);
		}
	}

	std::shared_ptr<hyperparams> clone() override
	{
		return std::make_shared<multinomial_hyper>(alpha);
	}

	VectorXd alpha;
};