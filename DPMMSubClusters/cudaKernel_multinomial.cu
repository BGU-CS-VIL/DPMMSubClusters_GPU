#ifndef CudaKernel_multinomial_CU
#define CudaKernel_multinomial_CU

#include "cudaKernel_multinomial.cuh"

void cudaKernel_multinomial::log_likelihood(Eigen::VectorXd& r, const Eigen::MatrixXd& x, const distribution_sample* distribution_sample)
{
	const multinomial_dist* pDistribution_sample = (multinomial_dist*)distribution_sample;
	Eigen::VectorXd alpha_vec = Eigen::VectorXd::Map(pDistribution_sample->alpha.data(), pDistribution_sample->alpha.size());
	r = (alpha_vec.adjoint() * x).row(0);
}

void cudaKernel_multinomial::log_likelihood_v2(
	double* d_r,
	int* d_r_offset,
	double* d_b,
	double* d_c,
	double* d_z,
	double* d_mu,
	int* d_indices,
	int* d_indicesSize,
	int dim,
	const distribution_sample* distribution_sample,
	cudaStream_t& stream)
{
	//TODO
}

void cudaKernel_multinomial::log_likelihood_v3(
	double* d_r,
	int* d_r_offset,
	double* d_b,
	double* d_c,
	double* d_z,
	double* d_mu,
	int* d_indicesSize,
	int dim,
	double weight,
	const distribution_sample* distribution_sample,
	cudaStream_t& stream)
{
	//TODO

}

#endif //CudaKernel_multinomial_CU