#ifndef CudaKernel_gaussian_H
#define CudaKernel_gaussian_H

#include "cudaKernel.cuh"
#include "distributions/mv_gaussian.h"

class cudaKernel_gaussian : public cudaKernel
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
		double weight,
		const std::shared_ptr<distribution_sample>& distribution_sample,
		cudaStream_t& stream,
		int deviceId);

	void divide_points_by_mu_all(int dim, const mv_gaussian* dist, double* d_z, cudaStream_t& stream, int deviceId);
	void do_create_sufficient_statistics(
		double* d_pts,
		int rows,
		int cols,
		const std::shared_ptr<hyperparams>& hyperParams,
		const std::shared_ptr<hyperparams>& posterior,
		cudaStream_t& stream,
		std::shared_ptr<sufficient_statistics>& ss) override;
	void mul_scalar_sum_A_AT(double* d_A, double* d_B, int n, double scalar, cudaStream_t& stream);
	void mul_scalar_sum_A_AT(double* d_A, double* d_B, int n, double scalar);

};

#endif //CudaKernel_gaussian_H
