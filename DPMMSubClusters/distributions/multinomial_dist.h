#pragma once
#include <vector>
#include <ds.h>

class multinomial_dist  : public distribution_sample
{
public:
	multinomial_dist() {}
	virtual ~multinomial_dist() {}
	multinomial_dist(const std::vector<double>& alpha) :alpha(alpha) {}
	std::shared_ptr<distribution_sample> clone () override
	{
		std::shared_ptr<multinomial_dist> pmultinomial_dist = std::make_shared<multinomial_dist>();
		pmultinomial_dist->alpha = alpha;
		return pmultinomial_dist;
	}

	virtual void serialize(Json::Value& root)
	{
		size_t size = alpha.size();
		for (size_t i = 0; i < size; i++)
		{
			root["alpha"].append(alpha[i]);
		}
	}

	std::vector<double> alpha;
};