#ifndef CudaKernel_gaussian_H
#define CudaKernel_gaussian_H

#include "cudaKernel.cuh"
#include "distributions/mv_gaussian.h"

class cudaKernel_gaussian : public cudaKernel
{
public:
	virtual void log_likelihood(Eigen::VectorXd& r, const Eigen::MatrixXd& x, const distribution_sample* distribution_sample);

protected:
	virtual void log_likelihood_v2(
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
		cudaStream_t& stream);

	virtual void log_likelihood_v3(
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
		cudaStream_t& stream);


};

#endif //CudaKernel_gaussian_H
