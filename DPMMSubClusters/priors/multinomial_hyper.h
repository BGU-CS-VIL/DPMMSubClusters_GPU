#pragma once

#include "hyperparams.h"
#include "Eigen/Dense"

using namespace Eigen;

class multinomial_hyper : public hyperparams
{
public:
	multinomial_hyper() {}
	multinomial_hyper(VectorXd alpha) : hyperparams(), alpha(alpha) {}
	~multinomial_hyper() {}

	std::shared_ptr<hyperparams> clone() override
	{
		return std::make_shared<multinomial_hyper>(alpha);
	}

	VectorXd alpha;
};

