#pragma once
#include <vector>
#include <ds.h>

class multinomial_dist  : public distribution_sample
{
public:
	multinomial_dist() {}
	multinomial_dist(const std::vector<double>& alpha) :alpha(alpha) {}
	distribution_sample *clone()
	{
		multinomial_dist *pmultinomial_dist = new multinomial_dist();
		pmultinomial_dist->alpha = alpha;
		return pmultinomial_dist;
	}

//	void log_likelihood(cudaKernel* cuda, VectorXd& r, const MatrixXd& x, const distribution_sample* distribution_sample) override;

	std::vector<double> alpha;
};

