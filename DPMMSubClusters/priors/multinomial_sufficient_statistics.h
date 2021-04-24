#pragma once

#include "sufficient_statistics.h"

class multinomial_sufficient_statistics : public sufficient_statistics
{
public:
	multinomial_sufficient_statistics() {}
	multinomial_sufficient_statistics(int N, VectorXd points_sum) : sufficient_statistics(N, points_sum) {}
	std::shared_ptr<sufficient_statistics> clone()
	{
		std::shared_ptr<multinomial_sufficient_statistics> ss = std::make_shared<multinomial_sufficient_statistics>();

		ss->N = N;
		ss->points_sum = points_sum;
		return ss;
	}
};
