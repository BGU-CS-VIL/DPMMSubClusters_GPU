#ifndef CudaKernel_gaussian_CU
#define CudaKernel_gaussian_CU

#include "cudaKernel_gaussian.cuh"

void cudaKernel_gaussian::log_likelihood(Eigen::VectorXd& r, const Eigen::MatrixXd& x, const distribution_sample* distribution_sample)
{
	const mv_gaussian* pDistribution_sample = (mv_gaussian*)distribution_sample;
	MatrixXd z = x.colwise() - pDistribution_sample->mu;

	int sizeVec = z.cols();

	double* d_z;
	double* d_b;
	double* d_c;
	double* d_r;

	runCuda(cudaMalloc((void**)&d_z, sizeof(double) * z.size()));
	runCuda(cudaMalloc((void**)&d_b, sizeof(double) * pDistribution_sample->invSigma.size()));
	runCuda(cudaMalloc((void**)&d_c, sizeof(double) * pDistribution_sample->invSigma.rows() * z.cols()));
	runCuda(cudaMemcpy(d_z, z.data(), sizeof(double) * z.size(), cudaMemcpyHostToDevice));
	runCuda(cudaMemcpy(d_b, pDistribution_sample->invSigma.data(), sizeof(double) * pDistribution_sample->invSigma.size(), cudaMemcpyHostToDevice));

	naive_matrix_multiply(d_b, d_z, d_c, pDistribution_sample->invSigma.rows(), z.cols(), pDistribution_sample->invSigma.cols());

	runCuda(cudaFree(d_b));

	runCuda(cudaMalloc((void**)&d_r, sizeof(double) * sizeVec));

	double scalar = -((pDistribution_sample->sigma.size() * log(2 * EIGEN_PI) + pDistribution_sample->logdetSigma) / 2);

	dcolwise_dot_all(sizeVec, z.rows(), d_z, d_c, scalar, d_r, 0);

	r.resize(sizeVec);
	runCuda(cudaMemcpy(r.data(), d_r, sizeof(double) * sizeVec, cudaMemcpyDeviceToHost));

	runCuda(cudaFree(d_z));
	runCuda(cudaFree(d_c));
	runCuda(cudaFree(d_r));
}

//__global__ void get_mat_from_indices(double* d_points, double* d_x, int* d_indices, int dim, int indicesSize)
//{
//	int idx = threadIdx.x + blockIdx.x * blockDim.x;
//	if (idx < indicesSize)
//	{
//		for (int j = 0; j < dim; j++)
//		{
//			d_x[IDX2C(j, idx, dim)] = d_points[IDX2C(j, d_indices[idx], dim)];
//		}
//	}
//}


__global__ void divide_points_by_mu(double* d_points, int* d_indices, int dim, int indicesSize, double* d_mu, double* d_z)
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

__global__ void divide_points_by_mu_all(double* d_points, int dim, int indicesSize, double* d_mu, double* d_z)
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

void cudaKernel_gaussian::log_likelihood_v2(
	double* d_r,
	int r_offset,
	int* d_indices,
	int indicesSize,
	int dim, 
	const distribution_sample* distribution_sample, 
	cudaStream_t& stream,
	int deviceId)
{
	const mv_gaussian* pDistribution_sample = (mv_gaussian*)distribution_sample;
	double* d_b;
	double* d_c;
	double* d_z;
	double* d_mu;

	//TODO - remove for perf.
	//double* d_x;
	//runCuda(cudaMalloc((void**)&d_x, sizeof(double) * dim* indicesSize));
	//get_mat_from_indices << <blocks, threads >> > (d_points, d_x, d_indices, dim, indicesSize);
	//runCuda(cudaPeekAtLastError());
	//runCuda(cudaDeviceSynchronize());

	//MatrixXd x(dim, indicesSize);
	//runCuda(cudaMemcpy(x.data(), d_x, sizeof(double) * dim * indicesSize, cudaMemcpyDeviceToHost));
	//End - remove for perf.

//	MatrixXd z = x.colwise() - pDistribution_sample->mu;
	runCuda(cudaMallocAsync((void**)&d_mu, sizeof(double) * pDistribution_sample->mu.size(), stream));
	runCuda(cudaMemcpyAsync(d_mu, pDistribution_sample->mu.data(), sizeof(double) * pDistribution_sample->mu.size(), cudaMemcpyHostToDevice, stream));
	runCuda(cudaMallocAsync((void**)&d_z, sizeof(double) * dim* indicesSize, stream));
	runCuda(cudaMallocAsync((void**)&d_c, sizeof(double) * pDistribution_sample->invSigma.rows() * indicesSize, stream));

	divide_points_by_mu << <blocks, threads , 0, stream>> > (gpuCapabilities[deviceId].d_points, d_indices, dim, indicesSize, d_mu, d_z);
	runCuda(cudaFreeAsync(d_mu, stream));

	runCuda(cudaMallocAsync((void**)&d_b, sizeof(double) * pDistribution_sample->invSigma.size(), stream));
	runCuda(cudaMemcpyAsync(d_b, pDistribution_sample->invSigma.data(), sizeof(double) * pDistribution_sample->invSigma.size(), cudaMemcpyHostToDevice, stream));

	//naive_matrix_multiply_v2(d_b, d_z, d_c, pDistribution_sample->invSigma.rows(), indicesSize, pDistribution_sample->invSigma.cols(), stream);
	naive_matrix_multiply_v3(d_b, d_z, d_c, pDistribution_sample->invSigma.rows(), pDistribution_sample->invSigma.cols(), indicesSize, stream);
	runCuda(cudaFreeAsync(d_b,stream));

	double scalar = -((pDistribution_sample->sigma.size() * log(2 * EIGEN_PI) + pDistribution_sample->logdetSigma) / 2);
	dcolwise_dot_all_v2(indicesSize, dim, d_z, d_c, scalar, d_r, r_offset, stream);
	runCuda(cudaFreeAsync(d_z, stream));
	runCuda(cudaFreeAsync(d_c, stream));
}

void cudaKernel_gaussian::log_likelihood_v3(
	double* d_r,
	int dim,
	double weight, 
	const distribution_sample* distribution_sample,
	cudaStream_t& stream,
	int deviceId)
{
	const mv_gaussian* pDistribution_sample = (mv_gaussian*)distribution_sample;
	double* d_b;
	double* d_c;
	double* d_z;
	double* d_mu;

	runCuda(cudaMallocAsync((void**)&d_mu, sizeof(double) * pDistribution_sample->mu.size(), stream));
	runCuda(cudaMemcpyAsync(d_mu, pDistribution_sample->mu.data(), sizeof(double) * pDistribution_sample->mu.size(), cudaMemcpyHostToDevice, stream));
	runCuda(cudaMallocAsync((void**)&d_z, sizeof(double) * dim * numLabels, stream));
	runCuda(cudaMallocAsync((void**)&d_c, sizeof(double) * pDistribution_sample->invSigma.rows() * numLabels, stream));
	divide_points_by_mu_all << <blocks, threads, 0, stream >> > (gpuCapabilities[deviceId].d_points, dim, numLabels, d_mu, d_z);
	runCuda(cudaFreeAsync(d_mu, stream));

	runCuda(cudaMallocAsync((void**)&d_b, sizeof(double) * pDistribution_sample->invSigma.size(), stream));
	runCuda(cudaMemcpyAsync(d_b, pDistribution_sample->invSigma.data(), sizeof(double) * pDistribution_sample->invSigma.size(), cudaMemcpyHostToDevice, stream));

	//naive_matrix_multiply_v2(d_b, d_z, d_c, pDistribution_sample->invSigma.rows(), numLabels, pDistribution_sample->invSigma.cols(), stream);
	naive_matrix_multiply_v3(d_b, d_z, d_c, pDistribution_sample->invSigma.rows(), pDistribution_sample->invSigma.cols(), numLabels, stream);
	runCuda(cudaFreeAsync(d_b, stream));

	double scalar = -((pDistribution_sample->sigma.size() * log(2 * EIGEN_PI) + pDistribution_sample->logdetSigma) / 2);
	dcolwise_dot_all_v3(numLabels, dim, d_z, d_c, scalar, d_r, weight, stream);
	runCuda(cudaFreeAsync(d_z, stream));
	runCuda(cudaFreeAsync(d_c, stream));
}


#endif //CudaKernel_gaussian_CU