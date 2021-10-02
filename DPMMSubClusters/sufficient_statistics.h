#pragma once
#include <memory>
#include "Eigen/Dense"
#include "IJsonSerializable.h"

using namespace Eigen;
//#Suff statistics must contain N which is the number of points associated with the cluster

class sufficient_statistics : public IJsonSerializable
{
public:
	virtual std::shared_ptr<sufficient_statistics> clone() = 0;
	sufficient_statistics() {}
	sufficient_statistics(int N, const VectorXd &points_sum) :N(N), points_sum(points_sum) {}
	virtual void serialize(Json::Value& root)
	{
		root["N"] = N;

		size_t size = points_sum.size();
		double* data = points_sum.data();
		for (size_t i = 0; i < size; i++)
		{
			root["points_sum"].append(data[i]);
		}
	}

	int N;
	VectorXd points_sum;
};