#ifndef CudaKernel_multinomial_CU
#define CudaKernel_multinomial_CU

#include "cudaKernel_multinomial.cuh"


__global__ void get_first_row_multiple_alpha_with_x(double* d_points, int dim, int* d_indices, int indicesSize, double* d_alpha, double* d_r)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < indicesSize)
	{
		double sum = 0;
		for (int i = 0; i < dim; i++)
		{
			sum += d_alpha[i] * d_points[IDX2C(i, d_indices[idx], dim)];
		}
		d_r[idx] = sum;
	}
}


__global__ void get_first_row_multiple_alpha_with_x_all(double* d_points, int dim, int indicesSize, double* d_alpha, double* d_r)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;

	if (idx < indicesSize)
	{
		double sum = 0;
		for (int i = 0; i < dim; i++)
		{
			sum += d_alpha[i] * d_points[IDX2C(i, idx, dim)];
		}
		d_r[idx] = sum;
	}
}

void cudaKernel_multinomial::log_likelihood_sub_labels(
	double* d_r,
	int r_offset,
	int* d_indices,
	int indicesSize,
	int dim,
	const std::shared_ptr<distribution_sample>& distribution_sample,
	cudaStream_t& stream,
	int deviceId)
{
	double* d_alpha;
	multinomial_dist* pDistribution_sample = dynamic_cast<multinomial_dist*>(distribution_sample.get());

	runCuda(cudaMallocAsync((void**)&d_alpha, sizeof(double) * pDistribution_sample->alpha.size(), stream));
	runCuda(cudaMemcpyAsync(d_alpha, pDistribution_sample->alpha.data(), sizeof(double) * pDistribution_sample->alpha.size(), cudaMemcpyHostToDevice, stream));
	
	dim3 blocks_size = dim3(indicesSize / threads.x + 1);

	get_first_row_multiple_alpha_with_x << <blocks_size, threads, 0, stream >> > (gpuCapabilities[deviceId].d_points, dim, d_indices, indicesSize, d_alpha, d_r+r_offset);

	runCuda(cudaFreeAsync(d_alpha, stream));
}

void cudaKernel_multinomial::log_likelihood_labels(
	double* d_r,
	int dim,
	double weight,
	const std::shared_ptr<distribution_sample>& distribution_sample,
	cudaStream_t& stream,
	int deviceId)
{
	double* d_alpha;
	multinomial_dist* pDistribution_sample = dynamic_cast<multinomial_dist*>(distribution_sample.get());

	runCuda(cudaMallocAsync((void**)&d_alpha, sizeof(double) * pDistribution_sample->alpha.size(), stream));
	runCuda(cudaMemcpyAsync(d_alpha, pDistribution_sample->alpha.data(), sizeof(double) * pDistribution_sample->alpha.size(), cudaMemcpyHostToDevice, stream));

	get_first_row_multiple_alpha_with_x_all << <blocks, threads, 0, stream >> > (gpuCapabilities[deviceId].d_points, dim, numLabels, d_alpha, d_r);

	runCuda(cudaFreeAsync(d_alpha, stream));
}

#endif //CudaKernel_multinomial_CU