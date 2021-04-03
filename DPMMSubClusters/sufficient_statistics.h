#pragma once
#include "Eigen/Dense"

using namespace Eigen;
//#Suff statistics must contain N which is the number of points associated with the cluster

class sufficient_statistics
{
public:
	virtual sufficient_statistics* clone() = 0;
	sufficient_statistics() {}
	sufficient_statistics(int N, const VectorXd &points_sum) :N(N), points_sum(points_sum) {}
	int N;
	VectorXd points_sum;

};