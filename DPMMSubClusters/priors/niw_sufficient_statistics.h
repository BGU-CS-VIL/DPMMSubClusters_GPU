#pragma once

#include <memory>
#include "sufficient_statistics.h"

class niw_sufficient_statistics : public sufficient_statistics
{
public:
	niw_sufficient_statistics() {}
	niw_sufficient_statistics(int N, const VectorXd& points_sum, const MatrixXd& S) : sufficient_statistics(N, points_sum), S(S)
	{
	}

	std::shared_ptr<sufficient_statistics> clone()
	{
		std::shared_ptr<niw_sufficient_statistics> ss = std::make_shared<niw_sufficient_statistics>();

		ss->N = N;
		ss->points_sum = points_sum;
		ss->S = S;
		return ss;
	}
	MatrixXd S;
};

