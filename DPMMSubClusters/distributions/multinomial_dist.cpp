#include "multinomial_dist.h"

/*

	[Multivariate Normal Distribution](https://en.wikipedia.org/wiki/Multivariate_normal_distribution)
*/


//void multinomial_dist::log_likelihood(cudaKernel* cuda, VectorXd& r, const MatrixXd& x, const distribution_sample* distribution_sample)
//{
//	const multinomial_dist* pDistribution_sample = (multinomial_dist*)distribution_sample;
//	Eigen::VectorXd alpha_vec = Eigen::VectorXd::Map(pDistribution_sample->alpha.data(), pDistribution_sample->alpha.size());
//	r = (alpha_vec.adjoint() * x).row(0);
//}
