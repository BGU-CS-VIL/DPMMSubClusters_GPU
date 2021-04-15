#ifndef CudaKernel_multinomial_H
#define CudaKernel_multinomial_H

#include "cudaKernel.cuh"
#include "distributions/multinomial_dist.h"

class cudaKernel_multinomial : public cudaKernel
{
protected:
	virtual void log_likelihood_sub_labels(
		double* d_r,
		int r_offset,
		int* d_indices,
		int indicesSize,
		int dim,
		const std::shared_ptr<distribution_sample>& distribution_sample,
		cudaStream_t& stream,
		int deviceId);

	virtual void log_likelihood_labels(
		double* d_r,
		int dim,
		double weight,
		const std::shared_ptr<distribution_sample>& distribution_sample,
		cudaStream_t& stream,
		int deviceId);
};

#endif //CudaKernel_multinomial_H
