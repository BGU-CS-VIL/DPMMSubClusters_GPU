#ifndef CudaKernel_gaussian_CU
#define CudaKernel_gaussian_CU

#include "cudaKernel_gaussian.cuh"
#include "niw_hyperparams.h"
#include "niw_sufficient_statistics.h"
#include "check_time.h"

__global__ void divide_points_by_mu_kernel(double* d_points, int* d_indices, int dim, int indicesSize, double* d_mu, double* d_z)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < indicesSize)
	{
		for (int j = 0; j < dim; j++)
		{
			d_z[IDX2C(j, idx, dim)] = d_points[IDX2C(j, d_indices[idx], dim)] - d_mu[j];
		}
	}
}

__global__ void divide_points_by_mu_all_kernel(double* d_points, int dim, int indicesSize, double* d_mu, double* d_z)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < indicesSize)
	{
		for (int j = 0; j < dim; j++)
		{
			d_z[IDX2C(j, idx, dim)] = d_points[IDX2C(j, idx, dim)] - d_mu[j];
		}
	}
}
__global__ void mul_scalar_sum_A_AT_kernel(double* d_A, double* d_B, int n, double scalar)
{
	int row = blockIdx.y * blockDim.y + threadIdx.y;
	int col = blockIdx.x * blockDim.x + threadIdx.x;
	double sum = 0;

	if (col < n && row < n)
	{
		d_B[IDX2C(row, col, n)] = scalar * (d_A[IDX2C(row, col, n)] + d_A[IDX2C(col, row, n)]);
	}
}

void cudaKernel_gaussian::log_likelihood_sub_labels(
	double* d_r,
	int r_offset,
	int* d_indices,
	int indicesSize,
	int dim, 
	const std::shared_ptr<distribution_sample>& distribution_sample,
	cudaStream_t& stream,
	int deviceId)
{
	mv_gaussian* pDistribution_sample = dynamic_cast<mv_gaussian*>(distribution_sample.get());

	double* d_b;
	double* d_c;
	double* d_z;
	double* d_mu;

	runCuda(cudaMallocAsync((void**)&d_mu, sizeof(double) * pDistribution_sample->mu.size(), stream));
	runCuda(cudaMemcpyAsync(d_mu, pDistribution_sample->mu.data(), sizeof(double) * pDistribution_sample->mu.size(), cudaMemcpyHostToDevice, stream));
	runCuda(cudaMallocAsync((void**)&d_z, sizeof(double) * dim* indicesSize, stream));
	runCuda(cudaMallocAsync((void**)&d_c, sizeof(double) * pDistribution_sample->invSigma.rows() * indicesSize, stream));

	divide_points_by_mu_kernel << <blocks, threads , 0, stream>> > (gpuCapabilities[deviceId].d_points, d_indices, dim, indicesSize, d_mu, d_z);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaFreeAsync(d_mu, stream));

	runCuda(cudaMallocAsync((void**)&d_b, sizeof(double) * pDistribution_sample->invSigma.size(), stream));
	runCuda(cudaMemcpyAsync(d_b, pDistribution_sample->invSigma.data(), sizeof(double) * pDistribution_sample->invSigma.size(), cudaMemcpyHostToDevice, stream));

	naive_matrix_multiply(d_b, d_z, d_c, (int)pDistribution_sample->invSigma.rows(), (int)pDistribution_sample->invSigma.cols(), indicesSize, stream);
	runCuda(cudaFreeAsync(d_b,stream));

	double scalar = -((pDistribution_sample->sigma.size() * log(2 * EIGEN_PI) + pDistribution_sample->logdetSigma) / 2);
	dcolwise_dot_all_sub_labels(indicesSize, dim, d_z, d_c, scalar, d_r, r_offset, stream);
	runCuda(cudaFreeAsync(d_z, stream));
	runCuda(cudaFreeAsync(d_c, stream));
}

//OUTPUT: d_r - log_likelihhod allocated in device memroy
void cudaKernel_gaussian::log_likelihood_labels(
	double* d_r,
	double weight, 
	const std::shared_ptr<distribution_sample>& distribution_sample,
	cudaStream_t& stream,
	int deviceId)
{
	mv_gaussian* pDistribution_sample = dynamic_cast<mv_gaussian*>(distribution_sample.get());

	double* d_b;
	double* d_c;
	double* d_z;
	int dim = gpuCapabilities[deviceId].pointsRows;

	runCuda(cudaMallocAsync((void**)&d_z, sizeof(double) * dim * numLabels, stream));
	divide_points_by_mu_all(dim, pDistribution_sample, d_z, stream, deviceId);
	cudaStreamSynchronize(stream);
	MatrixXd data;
	data.resize(dim, numLabels);
	cudaMemcpy(data.data(), d_z, sizeof(double) * data.size(), cudaMemcpyDeviceToHost);

	runCuda(cudaMallocAsync((void**)&d_c, sizeof(double) * pDistribution_sample->invSigma.rows() * numLabels, stream));
	runCuda(cudaMallocAsync((void**)&d_b, sizeof(double) * pDistribution_sample->invSigma.size(), stream));
	runCuda(cudaMemcpyAsync(d_b, pDistribution_sample->invSigma.data(), sizeof(double) * pDistribution_sample->invSigma.size(), cudaMemcpyHostToDevice, stream));

	naive_matrix_multiply(d_b, d_z, d_c, (int)pDistribution_sample->invSigma.rows(), (int)pDistribution_sample->invSigma.cols(), numLabels, stream);
	runCuda(cudaFreeAsync(d_b, stream));

	cudaStreamSynchronize(stream);
	data.resize(dim , numLabels);
	cudaMemcpy(data.data(), d_z, sizeof(double) * data.size(), cudaMemcpyDeviceToHost);

	data.resize(pDistribution_sample->invSigma.rows(), numLabels);
	cudaMemcpy(data.data(), d_c, sizeof(double) * data.size(), cudaMemcpyDeviceToHost);


	double scalar = -((pDistribution_sample->sigma.size() * log(2 * EIGEN_PI) + pDistribution_sample->logdetSigma) / 2);
	dcolwise_dot_all_labels(numLabels, dim, d_z, d_c, scalar, d_r, weight, stream);
	runCuda(cudaFreeAsync(d_z, stream));
	runCuda(cudaFreeAsync(d_c, stream));

	data.resize(numLabels,1);
	cudaMemcpy(data.data(), d_r, sizeof(double) * data.size(), cudaMemcpyDeviceToHost);

}

void cudaKernel_gaussian::divide_points_by_mu_all(int dim, const mv_gaussian* dist, double* d_z, cudaStream_t& stream, int deviceId)
{
	double* d_mu;

	runCuda(cudaMallocAsync((void**)&d_mu, sizeof(double) * dist->mu.size(), stream));
	runCuda(cudaMemcpyAsync(d_mu, dist->mu.data(), sizeof(double) * dist->mu.size(), cudaMemcpyHostToDevice, stream));
	divide_points_by_mu_all_kernel << <blocks, threads, 0, stream >> > (gpuCapabilities[deviceId].d_points, dim, numLabels, d_mu, d_z);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaFreeAsync(d_mu, stream));
}

void cudaKernel_gaussian::do_create_sufficient_statistics(
	double* d_pts,
	int rows,
	int cols,
	const std::shared_ptr<hyperparams>& hyperParams,
	const std::shared_ptr<hyperparams>& posterior,
	cudaStream_t& stream,
	std::shared_ptr<sufficient_statistics>& ss)
{
	if (cols == 0)
	{
		niw_hyperparams* pNiw_hyperparams = dynamic_cast<niw_hyperparams*>(hyperParams.get());
		ss = std::make_shared<niw_sufficient_statistics>(cols, VectorXd::Zero(pNiw_hyperparams->m.size()), MatrixXd::Zero(pNiw_hyperparams->m.size(), pNiw_hyperparams->m.size()));
	}
	else
	{
		CHECK_TIME("cudaKernel_gaussian::do_create_sufficient_statistics");
		const int mem_size = rows * cols * sizeof(double);

		double* d_c;
		runCuda(cudaMallocAsync(&d_c, rows * rows * sizeof(double), stream));
		runCuda(cudaStreamSynchronize(stream));

		multiplie_matrix_by_transpose(d_pts, d_c, rows, cols);

		runCuda(cudaStreamSynchronize(stream));

		ss = std::make_shared<niw_sufficient_statistics>();
		niw_sufficient_statistics* niw_ss = dynamic_cast<niw_sufficient_statistics*>(ss.get());
		niw_ss->S.resize(rows, rows);
		niw_ss->N = cols;
		runCuda(cudaMemcpyAsync(niw_ss->S.data(), d_c, sizeof(double) * rows * rows, cudaMemcpyDeviceToHost, stream));
		runCuda(cudaFreeAsync(d_c, stream));

		double* d_sum_rowwise;
		runCuda(cudaMallocAsync(&d_sum_rowwise, rows * sizeof(double), stream));

		sum_rowwise(d_pts, d_sum_rowwise, rows, cols, stream);
		niw_ss->points_sum.resize(rows);
		runCuda(cudaMemcpyAsync(niw_ss->points_sum.data(), d_sum_rowwise, sizeof(double) * rows, cudaMemcpyDeviceToHost, stream));

		runCuda(cudaFreeAsync(d_sum_rowwise, stream));
	}
}

//Working for Square Matrix only
void cudaKernel_gaussian::mul_scalar_sum_A_AT(double* d_A, double* d_B, int n, double scalar, cudaStream_t& stream)
{
	const int BlockSize = 16;

	unsigned int grid_rows = (n + BlockSize - 1) / BlockSize;
	unsigned int grid_cols = (n + BlockSize - 1) / BlockSize;
	dim3 dimGrid(grid_cols, grid_rows);
	dim3 dimBlock(BlockSize, BlockSize);

	mul_scalar_sum_A_AT_kernel << <dimGrid, dimBlock, 0, stream >> > (d_A, d_B, n, scalar);
	runCuda(cudaPeekAtLastError());
}

void cudaKernel_gaussian::mul_scalar_sum_A_AT(double* d_A, double* d_B, int n, double scalar)
{
	const int BlockSize = 16;

	unsigned int grid_rows = (n + BlockSize - 1) / BlockSize;
	unsigned int grid_cols = (n + BlockSize - 1) / BlockSize;
	dim3 dimGrid(grid_cols, grid_rows);
	dim3 dimBlock(BlockSize, BlockSize);

	mul_scalar_sum_A_AT_kernel << <dimGrid, dimBlock >> > (d_A, d_B, n, scalar);
	runCuda(cudaPeekAtLastError());
}

#endif //CudaKernel_gaussian_CU