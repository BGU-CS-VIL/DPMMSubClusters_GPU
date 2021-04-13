#ifndef CudaKernel_multinomial_H
#define CudaKernel_multinomial_H

#include "cudaKernel.cuh"
#include "distributions/multinomial_dist.h"

class cudaKernel_multinomial : public cudaKernel
{
public:
	virtual void log_likelihood(Eigen::VectorXd& r, const Eigen::MatrixXd& x, const distribution_sample* distribution_sample);


protected:
	virtual void log_likelihood_v2(
		double* d_r,
		int r_offset,
		int* d_indices,
		int indicesSize,
		int dim,
		const distribution_sample* distribution_sample,
		cudaStream_t& stream,
		int deviceId);

	virtual void log_likelihood_v3(
		double* d_r,
		int dim,
		double weight,
		const distribution_sample* distribution_sample,
		cudaStream_t& stream,
		int deviceId);
};

#endif //CudaKernel_multinomial_H
