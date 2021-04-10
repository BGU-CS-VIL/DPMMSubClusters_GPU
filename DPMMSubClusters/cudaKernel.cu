#ifndef CudaKernel_CU
#define CudaKernel_CU

//#pragma warning( disable : 2886 )
//#pragma warning( disable : 2929)
#include <omp.h>

#include <cuda_runtime.h>
#include <cublas_v2.h>
#include<curand.h>
#include<curand_kernel.h>

#include<time.h>
#include "cudaKernel.cuh"
#include "distributions/mv_gaussian.h"

// function to define seed
__global__ void initCurand(curandState *state, unsigned long long seed, int maxIdx) {
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < maxIdx)
	{
		curand_init(seed, idx, 0, &state[idx]);
	}
}

__device__ void sample_by_probability(curandState *state, double *weight, int numClusters, int rows, int idx, int *index,
									  double *y, int *a, int *b)
{
	int i;
	int j;
	int k;
	int n = numClusters;

	
//	if (n > 1)
	{
		a[idx] = 0;
		for (i = 1; i <= n; i++)
		{
			a[i*rows + idx] = i;
		}
		a[(n+1)*rows + idx] = n + 1;

		b[idx] = 0;
		for (i = 1; i <= n; i++)
		{
			b[i*rows + idx] = i;
		}
		b[(n+1)*rows + idx] = n + 1;
		/*
		  Copy Y from X.
		  Scale the probability vector and set sentinel values at the ends.
		*/
		y[idx] = 0.0;
		for (i = 1; i <= n; i++)
		{
			y[i*rows + idx] = weight[(i-1)*rows + idx] * (double)(n);
		}
		y[(n+1)*rows + idx] = 2.0;

		i = 0;
		j = n + 1;
		for (; ; )
		{
			/*
			  Find i so Y[B[i]] needs more.
			*/
			do
			{
				i++;
			} while (y[b[i*rows + idx]*rows + idx] < 1.0);
			/*
				  Find j so Y[B[j]] wants less.
				*/
			do
			{
				j--;
			} while (1.0 <= y[b[j*rows + idx]*rows + idx]);

			if (j <= i)
			{
				break;
			}
			/*
			  Swap B[i] and B[j].
			*/
			k = b[i*rows + idx];
			b[i*rows + idx] = b[j*rows + idx];
			b[j*rows + idx] = k;
		}

		i = j;
		j++;

		while (0 < i)
		{
			/*
			  Find J such that Y[B[j]] needs more.
			*/
			while (y[b[j*rows + idx]*rows + idx] <= 1.0)
			{
				j++;
			}
			/*
			  Meanwhile, Y[B[i]] wants less.
			*/
			if (n < j)
			{
				break;
			}
			/*
			  B[i] will donate to B[j] to fix up.
			*/
			y[b[j*rows + idx]*rows + idx] = y[b[j*rows + idx]*rows + idx] - (1.0 - y[b[i*rows + idx]*rows + idx]);
			a[b[i*rows + idx]*rows + idx] = b[j*rows + idx];
			/*
			  Y[B[j]] now wants less so readjust ordering.
			*/
			if (y[b[j*rows + idx]*rows + idx] < 1.0)
			{
				k = b[i*rows + idx];
				b[i*rows + idx] = b[j*rows + idx];
				b[j*rows + idx] = k;
				j++;
			}
			else
			{
				i--;
			}
		}

		double r;
		/*
		  Let i = random uniform integer from {1,2,...N};
		*/
		i = 1 + (int)(n * curand_uniform(state));
		//for (int j = 0; j < 2; j++)
		{
			r = curand_uniform(state);

			if (y[i*rows + idx] < r)
			{
				i = a[i*rows + idx];
//				break;
			}
		}
		*index = i;
//		*index = 1;
	}
//	else
	{
//		*index = 1;
	}
}

//__device__ void sample_by_probability(curandState *state, double *weight, int numClusters, int rows, int idx, int *index)
//{
//	//do
//	{
//		int cluster1 = 0;
//		int cluster2 = 0;
//		if (numClusters > 1)
//		{
//			cluster2 = 1;
//		}
//
//		if (weight[cluster1*rows + idx] > weight[cluster2*rows + idx])
//		{
//			*index = cluster1 + 1;
//		}
//		else
//		{
//			*index = cluster2 + 1;
//
//		}
//	}
//}

__global__ void sample_log_cat_array_all(curandState *state, int *dev_sample, int maxIdx, int numClusters, double *d_log_likelihood_array, double *y, int *a, int *b)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < maxIdx)
	{
		sample_by_probability(&state[idx], d_log_likelihood_array, numClusters, maxIdx, idx, &dev_sample[idx], y, a, b);
	}
}

__global__ void sample_log_cat_array_sub_cluster_all(curandState *state, int *dev_sample, int maxIdx, int num, double *d_log_likelihood_array, int *indices, double *y, int *a, int *b)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < maxIdx)
	{
		sample_by_probability(&state[indices[idx]], d_log_likelihood_array, num, maxIdx, idx, &dev_sample[indices[idx]], y, a, b);
	}
}

__device__ void sample_sub_label(curandState *state, int *d_label)
{
	*d_label = ((int)(curand_uniform(state) * 2)) % 2 + 1;
}

__global__ void sample_sub_labels_all(curandState *state, int *d_labels, int maxIdx)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < maxIdx)
	{
		sample_sub_label(&state[idx], &d_labels[idx]);
	}
}

__device__ void sample_label(curandState *state, int *d_label, int initial_clusters, double outlier_mod)
{
	*d_label = ((int)(curand_uniform(state)*initial_clusters)) % initial_clusters + 1 + ((outlier_mod > 0) ? 1 : 0);
}

__global__ void sample_labels_all(curandState *state, int *d_labels, int maxIdx, int initial_clusters, double outlier_mod)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < maxIdx)
	{
		sample_label(&state[idx], &d_labels[idx], initial_clusters, outlier_mod);
	}
}

__device__ void remove_empty_clusters_worker(int *d_label, int limit)
{
	if (*d_label > limit)
	{
		*d_label -= 1;
	}
}

__global__ void remove_empty_clusters_worker_all(int *d_labels, int maxIdx, int limit)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < maxIdx)
	{
		remove_empty_clusters_worker(&d_labels[idx], limit);
	}
}

__global__ void find_indices(int *d_labels, int maxIdx, int label, int *d_indices, int *d_indicesSize)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < maxIdx)
	{
		if (d_labels[idx] == label)
		{
			int curIndex = atomicAdd(d_indicesSize, 1);
			d_indices[curIndex] = idx;
		}
	}
}

__device__ void split_cluster_local_worker(curandState *state, int *d_labels, int *d_sub_label, int index, int newIndex)
{
	if (*d_labels == index + 1)
	{
		if (*d_sub_label == 2)
		{
			*d_labels = newIndex + 1;
		}
		*d_sub_label = curand(state) % 2 + 1;
	}
}

__global__ void split_cluster_local_worker_all(curandState *state, int *d_labels, int maxIdx, int *d_sub_labels, int index, int newIndex)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < maxIdx)
	{
		split_cluster_local_worker(&state[idx], &d_labels[idx], &d_sub_labels[idx], index, newIndex);
	}
}

__device__ void merge_clusters_worker(curandState *state, int *d_labels, int *d_sub_labels, int index, int newIndex)
{
	if (*d_labels == index + 1)
	{
		*d_sub_labels = 1;
	}
	if (*d_labels == newIndex + 1)
	{
		*d_sub_labels = 2;
	}
	if (*d_labels == newIndex + 1)
	{
		*d_labels = index + 1;
	}
}

__global__ void merge_clusters_worker_all(curandState *state, int *d_labels, int maxIdx, int *d_sub_labels, int index, int newIndex)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < maxIdx)
	{
		merge_clusters_worker(&state[idx], &d_labels[idx], &d_sub_labels[idx], index, newIndex);
	}
}

__device__ void reset_bad_clusters_worker(curandState *state, int *d_labels, int *d_sub_labels, int index)
{
	if (*d_labels == index + 1)
	{
		*d_sub_labels = curand(state) % 2 + 1;
	}
}

__global__ void reset_bad_clusters_worker_all(curandState *state, int *d_labels, int maxIdx, int *d_sub_labels, int index)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < maxIdx)
	{
		reset_bad_clusters_worker(&state[idx], &d_labels[idx], &d_sub_labels[idx], index);
	}
}

__global__ void get_sub_labels_count_all(int *d_sub_labels, int maxIdx, int *l, int *r)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < maxIdx)
	{
		if (d_sub_labels[idx] == 1)
		{
			atomicAdd(l, 1);
		}
		else if (d_sub_labels[idx] == 2)
		{
			atomicAdd(r, 1);
		}
	}
}

__global__ void create_suff_stats_dict_worker_all(
	int* d_sub_labels,
	int maxIdx,
	int *d_indices,
	int *d_indicesSize,
	double *group_pts,
	int group_pts_rows,
	double *pts,
	double *pts1,
	double *pts2,
	int *d_j1,
	int *d_j2)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < *d_indicesSize)
	{
		if (d_sub_labels[d_indices[idx]] == 1)
		{
			int curj1 = atomicAdd(d_j1, 1);
			for (int i = 0; i < group_pts_rows; i++)
			{
				double pt = group_pts[IDX2C(i, d_indices[idx], group_pts_rows)];
				pts[IDX2C(i, idx, group_pts_rows)] = pt;
				pts1[IDX2C(i, curj1, group_pts_rows)] = pt;
			}
		}
		else if (d_sub_labels[d_indices[idx]] == 2)
		{
			int curj2 = atomicAdd(d_j2, 1);
			for (int i = 0; i < group_pts_rows; i++)
			{
				double pt = group_pts[IDX2C(i, d_indices[idx], group_pts_rows)];
				pts[IDX2C(i, idx, group_pts_rows)] = pt;
				pts2[IDX2C(i, curj2, group_pts_rows)] = pt;
			}
		}
	}
}

__global__ void dcolwise_dot_all_kernel(int maxIdx, int rows, double* d_a, double* d_b, double scalar, double* d_r, int r_offset)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;

	if (idx < maxIdx)
	{
		double sum = 0;
		for (int i = 0; i < rows; i++)
		{
			sum += d_a[idx * rows + i] * d_b[idx * rows + i];
		}

		d_r[idx + r_offset] = scalar - sum / 2;
	}
}

__global__ void dcolwise_dot_with_log_kernel(int maxIdx, int rows, double* d_a, double* d_b, double scalar, double* d_r, double weight)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;

	if (idx < maxIdx)
	{
		double sum = 0;
		for (int i = 0; i < rows; i++)
		{
			sum += d_a[idx * rows + i] * d_b[idx * rows + i];
		}

		d_r[idx] = scalar - sum / 2 + __logf(weight);
	}
}

__global__ void build_log_likelihood_array_sub_cluster_kernel(int maxIdx, double* d_log_likelihood_array, double* d_lr_weights, int rows, int cols)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;

	if (idx < maxIdx)
	{
		double maxRow = 0;
		bool first = true;
		for (int j = 0; j < cols; j++)
		{
			d_log_likelihood_array[IDX2C(idx, j, rows)] += log(d_lr_weights[j]);
			if (first || d_log_likelihood_array[IDX2C(idx, j, rows)] > maxRow)
			{
				first = false;
				maxRow = d_log_likelihood_array[IDX2C(idx, j, rows)];
			}
		}

		double sum = 0;
		for (int j = 0; j < cols; j++)
		{
			d_log_likelihood_array[IDX2C(idx, j, rows)] = exp(d_log_likelihood_array[IDX2C(idx, j, rows)] - maxRow);
			sum += d_log_likelihood_array[IDX2C(idx, j, rows)];
		}

		for (int j = 0; j < cols; j++)
		{
			d_log_likelihood_array[IDX2C(idx, j, rows)] = d_log_likelihood_array[IDX2C(idx, j, rows)] / sum;
		}
	}
}

__global__ void build_log_likelihood_array_sub_cluster_kernel_v2(int maxIdx, double* d_r, int r_offset, double* d_lr_weights)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;

	if (idx < maxIdx)
	{
		double maxRow;
		double sum;

		d_r[idx] += __logf(d_lr_weights[0]);
		d_r[idx + r_offset] += __logf(d_lr_weights[1]);
		maxRow = fmax(d_r[idx], d_r[idx + r_offset]);

		d_r[idx] = __expf(d_r[idx] - maxRow);
		d_r[idx + r_offset] = __expf(d_r[idx + r_offset] - maxRow);
		sum = d_r[idx] + d_r[idx + r_offset];

		d_r[idx] = d_r[idx] / sum;
		d_r[idx + r_offset] = d_r[idx + r_offset] / sum;
	}
}

__global__ void build_log_likelihood_array_kernel(int maxIdx, double* d_log_likelihood_array, int rows, int cols)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;

	if (idx < maxIdx)
	{
		double maxRow = 0;
		bool first = true;
		for (int j = 0; j < cols; j++)
		{
			if (first || d_log_likelihood_array[IDX2C(idx, j, rows)] > maxRow)
			{
				first = false;
				maxRow = d_log_likelihood_array[IDX2C(idx, j, rows)];
			}
		}

		double sum = 0;
		for (int j = 0; j < cols; j++)
		{
			d_log_likelihood_array[IDX2C(idx, j, rows)] = exp(d_log_likelihood_array[IDX2C(idx, j, rows)] - maxRow);
			sum += d_log_likelihood_array[IDX2C(idx, j, rows)];
		}

		for (int j = 0; j < cols; j++)
		{
			d_log_likelihood_array[IDX2C(idx, j, rows)] = d_log_likelihood_array[IDX2C(idx, j, rows)] / sum;
		}
	}
}

__global__ void update_labels_by_max_index_kernel(double* parr, int* d_labels, int maxIdx, int dim)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < maxIdx)
	{
		double max = 0;
		bool first = true;
		int max_index = 0;
		for (int i = 0; i < dim; ++i)
		{
			if (first || max < parr[IDX2C(idx, i, maxIdx)])
			{
				first = false;
				max = parr[IDX2C(idx, i, maxIdx)];
				max_index = i;
			}
		}
		d_labels[idx] = max_index + 1;
	}
}

__global__ void gpu_matrix_mult(double* a, double* b, double* c, int m, int n, int k)
{
	int row = blockIdx.y * blockDim.y + threadIdx.y;
	int col = blockIdx.x * blockDim.x + threadIdx.x;
	double sum = 0;
//	printf("row=%d, col=%d\n", row, col);
	if (col < k && row < m)
	{
		for (int i = 0; i < n; i++)
		{
			//sum += a[row * n + i] * b[i * k + col];
			sum += a[IDX2C(row, i,m)] * b[IDX2C(i, col,n)];
			//printf("cuSum=%f, sum=%f\n", a[IDX2C(row, i, m)] * b[IDX2C(i, col, n)], sum);
//			printf("cuSum=%f, sum=%f, a=%f, b=%f\n", a[IDX2C(row, i, m)] * b[IDX2C(i, col, n)], sum, a[IDX2C(row, i, n)], b[IDX2C(i, col, k)]);

//			printf("row=%d, col=%d, i=%d, a=%f, b=%f\n", row, col,i, a[IDX2C(row, i, n)], b[IDX2C(i, col, k)]);

			
		}
		//c[row * k + col] = sum;
		c[IDX2C(row, col,m)] = sum;

	}
}

void cudaKernel::init(int numLabelsIn, MatrixXd &points, unsigned long long seed)
{
	printf("Init cuda\n");

	int numGPU;
	int driverVersion = 0, runtimeVersion = 0;

	lastDevice = 0;

	runCuda(cudaGetDeviceCount(&numGPU));
	numGPU = 1;

	printf("Number of GPUs: %i\n", numGPU);
	printf("number of host CPUs:\t%d\n", omp_get_num_procs());

	for (int i = 0; i < numGPU; i++)
	{
		cudaSetDevice(i);
		cudaDeviceProp deviceProp;
		cudaGetDeviceProperties(&deviceProp, i);

		cudaDriverGetVersion(&driverVersion);
		cudaRuntimeGetVersion(&runtimeVersion);

		printf("\nDevice %d: \"%s\"\n", i, deviceProp.name);
		printf("CUDA Driver Version / Runtime Version          %d.%d / %d.%d\n", driverVersion / 1000, (driverVersion % 100) / 10, runtimeVersion / 1000, (runtimeVersion % 100) / 10);
		printf("CUDA Capability Major/Minor version number:    %d.%d\n", deviceProp.major, deviceProp.minor);

		int* dummy;
		cudaStream_t stream;

		cudaStreamCreate(&stream);

		cudaError_t err = cudaMallocAsync((void**)&dummy, sizeof(int), stream);
		if (cudaSuccess != err)
		{
			cudaGetLastError();
			printf("Not capable device. Can't perform asynchronous memory allocation.\n");
		}
		else
		{
			gpuCapabilities[i] = gpuCapability();
		}
		cudaStreamSynchronize(stream);
		cudaStreamDestroy(stream);
	}
		
	printf("\nNumber of GPUs that will be used: %i\n\n", gpuCapabilities.size());

	numLabels = numLabelsIn;
	threads = dim3(512);
	blocks = dim3(numLabels / threads.x + 1);

	for (std::map<int, gpuCapability>::iterator iter = gpuCapabilities.begin(); iter != gpuCapabilities.end(); iter++)
	{
		cudaSetDevice(iter->first);
		runCuda(cudaDeviceReset());
		runCuda(cudaMalloc((void**)&(iter->second.devState), numLabels * sizeof(curandState)));
		initCurand << <blocks, threads >> > (iter->second.devState, seed, numLabels);
		runCuda(cudaPeekAtLastError());

		runCuda(cudaMalloc((void**)&(iter->second.d_labels), numLabels * sizeof(int)));
		runCuda(cudaMalloc((void**)&(iter->second.d_sub_labels), numLabels * sizeof(int)));
		runCuda(cudaMalloc((void**)&(iter->second.d_points), points.size() * sizeof(double)));
		runCuda(cudaMemcpy(iter->second.d_points, points.data(), points.size() * sizeof(double), cudaMemcpyHostToDevice));
	}

	if (gpuCapabilities.size() > 0)
	{
		cudaSetDevice(gpuCapabilities.begin()->first);
	}
}

void cudaKernel::release()
{
	printf("Release cuda\n");

	for (std::map<int, gpuCapability>::iterator iter = gpuCapabilities.begin(); iter != gpuCapabilities.end(); iter++)
	{
		if (iter->second.devState != NULL)
		{
			runCuda(cudaFree(iter->second.devState));
		}

		if (iter->second.d_labels != NULL)
		{
			runCuda(cudaFree(iter->second.d_labels));
		}

		if (iter->second.d_sub_labels != NULL)
		{
			runCuda(cudaFree(iter->second.d_sub_labels));
		}

		if (iter->second.d_points != NULL)
		{
			runCuda(cudaFree(iter->second.d_points));
		}
	}
}

int cudaKernel::peak_device()
{
	int result;
	//++lastDevice;
	//if (lastDevice >= gpuCapabilities.size())
	//	lastDevice = 0;

	//int i = 0;
	//for (std::map<int, gpuCapability>::iterator iter = gpuCapabilities.begin(); i <= lastDevice && iter != gpuCapabilities.end(); iter++, ++i)
	//{
	//	result = iter->first;
	//}
	//cudaSetDevice(result);
	//return result;
	result = gpuCapabilities.begin()->first;
	cudaSetDevice(result);
	return result;
}

int cudaKernel::peak_device_v2()
{
	int result;
	++lastDevice;
	if (lastDevice >= gpuCapabilities.size())
		lastDevice = 0;

	int i = 0;
	for (std::map<int, gpuCapability>::iterator iter = gpuCapabilities.begin(); i <= lastDevice && iter != gpuCapabilities.end(); iter++, ++i)
	{
		result = iter->first;
	}
	cudaSetDevice(result);
	return result;
	//return 0;
}

int cudaKernel::sample_log_cat_array_sub_cluster(LabelType *indices, int labelsSize, Eigen::MatrixXd &log_likelihood_array, std::vector<double>& lr_weights, int deviceId)
{
	int *d_indices;
	double *d_y;
	int *d_a;
	int *d_b;
	double* d_lr_weights;
	
	runCuda(cudaMalloc((void**)&d_indices, sizeof(int) * labelsSize));
	runCuda(cudaMemcpy(d_indices, indices, sizeof(int) * labelsSize, cudaMemcpyHostToDevice));
	runCuda(cudaMalloc((void**)&d_lr_weights, sizeof(double) * lr_weights.size()));
	runCuda(cudaMemcpy(d_lr_weights, lr_weights.data(), sizeof(double) * lr_weights.size(), cudaMemcpyHostToDevice));

	dim3 blocks_size = dim3(labelsSize / threads.x + 1);
	
	int n = log_likelihood_array.size();
	double *d_log_likelihood_array;
	runCuda(cudaMalloc((void **)&d_log_likelihood_array, sizeof(double)*n));
	runCuda(cudaMemcpy(d_log_likelihood_array, log_likelihood_array.data(), sizeof(double)*n, cudaMemcpyHostToDevice));
	runCuda(cudaMalloc((void **)&d_y, sizeof(double)*labelsSize*(log_likelihood_array.cols() + 2)));
	runCuda(cudaMalloc((void **)&d_a, sizeof(int)*labelsSize*(log_likelihood_array.cols() + 2)));
	runCuda(cudaMalloc((void **)&d_b, sizeof(int)*labelsSize*(log_likelihood_array.cols() + 2)));

	//	int dev = 0;//GPU index. need to loop for all GPUs

	build_log_likelihood_array_sub_cluster_kernel << <blocks_size, threads >> > (log_likelihood_array.rows(), d_log_likelihood_array, d_lr_weights, log_likelihood_array.rows(), log_likelihood_array.cols());
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());
	
	sample_log_cat_array_sub_cluster_all << <blocks_size, threads >> > (gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_sub_labels, labelsSize, log_likelihood_array.cols(), d_log_likelihood_array, d_indices, d_y, d_a, d_b);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());

	//free memory
	runCuda(cudaFree(d_indices));
	runCuda(cudaFree(d_lr_weights));
	runCuda(cudaFree(d_log_likelihood_array));
	runCuda(cudaFree(d_y));
	runCuda(cudaFree(d_a));
	runCuda(cudaFree(d_b));

	return 0;
}

void cudaKernel::sample_log_cat_array_sub_cluster_v2(
	double* d_r,
	int r_offset,
	int* d_indices,
	int indicesSize,
	double* d_lr_weights,
	cudaStream_t& stream,
	int deviceId)
{
	double* d_y;
	int* d_a;
	int* d_b;

	runCuda(cudaMallocAsync((void**)&d_y, sizeof(double) * indicesSize * (2 + 2), stream));
	runCuda(cudaMallocAsync((void**)&d_a, sizeof(int) * indicesSize * (2 + 2), stream));
	runCuda(cudaMallocAsync((void**)&d_b, sizeof(int) * indicesSize * (2 + 2), stream));

	build_log_likelihood_array_sub_cluster_kernel_v2 << <blocks, threads, 0, stream >> > (indicesSize, d_r, r_offset, d_lr_weights);
	runCuda(cudaPeekAtLastError());

	//TODO - Can we remove d_y, d_a, d_b?
	sample_log_cat_array_sub_cluster_all << <blocks, threads, 0, stream >> > (gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_sub_labels, indicesSize, 2, d_r, d_indices, d_y, d_a, d_b);
	runCuda(cudaPeekAtLastError());

	runCuda(cudaFreeAsync(d_y, stream));
	runCuda(cudaFreeAsync(d_a, stream));
	runCuda(cudaFreeAsync(d_b, stream));

	update_sub_labels_to_all_other_devices(deviceId, stream);
}

void cudaKernel::sample_log_cat_array_v2(
	double* d_r,
	int dim,
	cudaStream_t& stream,
	int deviceId)
{
	double* d_y;
	int* d_a;
	int* d_b;

	runCuda(cudaMallocAsync((void**)&d_y, sizeof(double) * numLabels * (dim + 2), stream));
	runCuda(cudaMallocAsync((void**)&d_a, sizeof(int) * numLabels * (dim + 2), stream));
	runCuda(cudaMallocAsync((void**)&d_b, sizeof(int) * numLabels * (dim + 2), stream));
	build_log_likelihood_array_kernel << <blocks, threads, 0, stream >> > (numLabels, d_r, numLabels, dim);
	runCuda(cudaPeekAtLastError());

	sample_log_cat_array_all << <blocks, threads, 0, stream >> > (gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_labels, numLabels, dim, d_r, d_y, d_a, d_b);
	runCuda(cudaPeekAtLastError());

	runCuda(cudaFreeAsync(d_y, stream));
	runCuda(cudaFreeAsync(d_a, stream));
	runCuda(cudaFreeAsync(d_b, stream));

	update_labels_to_all_other_devices(deviceId, stream);
}

int cudaKernel::sample_log_cat_array(Eigen::MatrixXd &log_likelihood_array,	int deviceId)
{
	int n = log_likelihood_array.size();
	double *d_log_likelihood_array;
	double *d_y;
	int *d_a;
	int *d_b;

	runCuda(cudaMalloc((void **)&d_log_likelihood_array, sizeof(double)*n));
	runCuda(cudaMemcpy(d_log_likelihood_array, log_likelihood_array.data(), sizeof(double)*n, cudaMemcpyHostToDevice));

	runCuda(cudaMalloc((void **)&d_y, sizeof(double)*numLabels*(log_likelihood_array.cols() + 2)));
	runCuda(cudaMalloc((void **)&d_a, sizeof(int)*numLabels*(log_likelihood_array.cols() + 2)));
	runCuda(cudaMalloc((void **)&d_b, sizeof(int)*numLabels*(log_likelihood_array.cols() + 2)));

//	int dev = 0;//GPU index. need to loop for all GPUs
	build_log_likelihood_array_kernel << <blocks, threads >> > (log_likelihood_array.rows(), d_log_likelihood_array, log_likelihood_array.rows(), log_likelihood_array.cols());
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());

	sample_log_cat_array_all <<<blocks, threads>>>(gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_labels, numLabels, log_likelihood_array.cols(), d_log_likelihood_array, d_y, d_a, d_b);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());

	//free memory
	runCuda(cudaFree(d_log_likelihood_array));
	runCuda(cudaFree(d_y));
	runCuda(cudaFree(d_a));
	runCuda(cudaFree(d_b));
	return 0;

}

//void cudaKernel::sample_sub_clusters_worker(LabelType label, LabelType* &indices, LabelType &indicesSize, int deviceId)
//{
//	int *d_indices;
//	runCuda(cudaMalloc((void **)&d_indices, sizeof(int)*numLabels));
//
//	int *d_indicesSize;
//	runCuda(cudaMalloc(&d_indicesSize, sizeof(int)));
//	runCuda(cudaMemset(d_indicesSize, 0, sizeof(int)));
//
//	find_indices << <blocks, threads >> > (gpuCapabilities[deviceId].d_labels, numLabels, label, d_indices, d_indicesSize);
//
//	runCuda(cudaPeekAtLastError());
//	runCuda(cudaDeviceSynchronize());
//
//	runCuda(cudaMemcpy(&indicesSize, d_indicesSize, sizeof(int), cudaMemcpyDeviceToHost));
//	runCuda(cudaMemcpy(indices, d_indices, indicesSize * sizeof(int), cudaMemcpyDeviceToHost));
//
//	runCuda(cudaFree(d_indicesSize));
//	runCuda(cudaFree(d_indices));
//}

void cudaKernel::sample_sub_clusters_worker_v2(LabelType label, int* d_indices, int &indicesSize, cudaStream_t& stream, int deviceId)
{
	int* d_indicesSize;
	runCuda(cudaMallocAsync(&d_indicesSize, sizeof(int), stream));
	runCuda(cudaMemsetAsync(d_indicesSize, 0, sizeof(int), stream));

	find_indices << <blocks, threads, 0, stream >> > (gpuCapabilities[deviceId].d_labels, numLabels, label, d_indices, d_indicesSize);
	runCuda(cudaMemcpyAsync(&indicesSize, d_indicesSize, sizeof(int), cudaMemcpyDeviceToHost, stream));

	runCuda(cudaPeekAtLastError());
}

void cudaKernel::create_suff_stats_dict_worker(
	LabelType label,
	LabelType &indicesSize,
	Eigen::MatrixXd &group_pts,
	Eigen::MatrixXd* &pts,
	Eigen::MatrixXd* &pts1,
	Eigen::MatrixXd* &pts2)
{
	int deviceId = peak_device();
	int *d_indices;
	runCuda(cudaMalloc((void **)&d_indices, sizeof(int)*numLabels));

	int *d_indicesSize;
	runCuda(cudaMalloc(&d_indicesSize, sizeof(int)));
	runCuda(cudaMemset(d_indicesSize, 0, sizeof(int)));

	find_indices << <blocks, threads >> > (gpuCapabilities[deviceId].d_labels, numLabels, label, d_indices, d_indicesSize);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());
	runCuda(cudaMemcpy(&indicesSize, d_indicesSize, sizeof(int), cudaMemcpyDeviceToHost));

	double *d_group_pts;
	runCuda(cudaMalloc((void **)&d_group_pts, sizeof(double)*group_pts.size()));
	runCuda(cudaMemcpy(d_group_pts, group_pts.data(), sizeof(double)*group_pts.size(), cudaMemcpyHostToDevice));

	double *d_pts;
	runCuda(cudaMalloc((void **)&d_pts, sizeof(double)*group_pts.rows()*indicesSize));

	double *d_pts1;
	runCuda(cudaMalloc((void **)&d_pts1, sizeof(double)*group_pts.rows()*indicesSize));

	double *d_pts2;
	runCuda(cudaMalloc((void **)&d_pts2, sizeof(double)*group_pts.rows()*indicesSize));

	int *d_j1;
	int *d_j2;
	runCuda(cudaMalloc(&d_j1, sizeof(int)));
	runCuda(cudaMemset(d_j1, 0, sizeof(int)));
	runCuda(cudaMalloc(&d_j2, sizeof(int)));
	runCuda(cudaMemset(d_j2, 0, sizeof(int)));

	dim3 blocks_size = dim3(indicesSize / threads.x + 1);
	create_suff_stats_dict_worker_all << <blocks_size, threads >> > (
		gpuCapabilities[deviceId].d_sub_labels,
		numLabels,
		d_indices,
		d_indicesSize,
		d_group_pts,
		group_pts.rows(),
		d_pts,
		d_pts1,
		d_pts2,
		d_j1,
		d_j2);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());

	int j1;
	int j2;
	runCuda(cudaMemcpy(&j1, d_j1, sizeof(int), cudaMemcpyDeviceToHost));
	runCuda(cudaMemcpy(&j2, d_j2, sizeof(int), cudaMemcpyDeviceToHost));

	pts = new Eigen::MatrixXd(group_pts.rows(), indicesSize);
	pts1 = new Eigen::MatrixXd(group_pts.rows(), j1);
	pts2 = new Eigen::MatrixXd(group_pts.rows(), j2);

	runCuda(cudaMemcpy(pts->data(), d_pts, sizeof(double)*group_pts.rows()*indicesSize, cudaMemcpyDeviceToHost));
	runCuda(cudaMemcpy(pts1->data(), d_pts1, sizeof(double)*group_pts.rows()*j1, cudaMemcpyDeviceToHost));
	runCuda(cudaMemcpy(pts2->data(), d_pts2, sizeof(double)*group_pts.rows()*j2, cudaMemcpyDeviceToHost));

	runCuda(cudaFree(d_j1));
	runCuda(cudaFree(d_j2));
	runCuda(cudaFree(d_indicesSize));
	runCuda(cudaFree(d_group_pts));
	runCuda(cudaFree(d_pts));
	runCuda(cudaFree(d_pts1));
	runCuda(cudaFree(d_pts2));
}
//
//void cudaKernel::create_suff_stats_dict_worker_v2(MatrixXd& group_pts, hyperparams* hyper_params, LabelsType& indices)
//{
//	std::map<LabelType, thin_suff_stats*> suff_stats_dict;
//
//	for (LabelType index = 0; index < indices.size(); index++)
//	{
//		LabelType indicesLabelsSize = 0;
//
//		MatrixXd* pts;
//		MatrixXd* pts1;
//		MatrixXd* pts2;
//
//		create_suff_stats_dict_worker(indices[index] + 1,
//			indicesLabelsSize,
//			group_pts,
//			pts,
//			pts1,
//			pts2);
//
//		thin_suff_stats* tss = new thin_suff_stats();
//		prior* cpl_suff = utils::create_sufficient_statistics(hyper_params, &(tss->l_suff), globalParams, *pts1);
//		delete cpl_suff;
//		prior* cpr_suff = utils::create_sufficient_statistics(hyper_params, &(tss->r_suff), globalParams, *pts2);
//		delete cpr_suff;
//		prior* cp_suff = utils::create_sufficient_statistics(hyper_params, &(tss->cluster_suff), globalParams, *pts);
//		delete cp_suff;
//
//		suff_stats_dict[index] = tss;
//
//		delete pts;
//		delete pts1;
//		delete pts2;
//	}
//
//	return suff_stats_dict;
//}

void cudaKernel::sample_sub_labels()
{
	int deviceId = peak_device();
	sample_sub_labels_all << <blocks, threads >> > (gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_sub_labels, numLabels);
	runCuda(cudaPeekAtLastError());

	update_sub_labels_to_all_other_devices(deviceId);
}

void cudaKernel::sample_labels(int initial_clusters, double outlier_mod)
{
	int deviceId = peak_device();
	sample_labels_all << <blocks, threads >> > (gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_labels, numLabels, initial_clusters, outlier_mod);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());

	update_labels_to_all_other_devices(deviceId);
}

void cudaKernel::get_sub_labels(LabelsType &subLabels)
{
	int deviceId = peak_device();
	int *h_subLabels;
	h_subLabels = (int*)malloc(numLabels * sizeof(int));
	runCuda(cudaMemcpy(h_subLabels, gpuCapabilities[deviceId].d_sub_labels, numLabels * sizeof(int), cudaMemcpyDeviceToHost));

	subLabels.resize(numLabels);
	for (size_t i = 0; i < numLabels; i++)
	{
		subLabels[i] = h_subLabels[i];
	}

	free(h_subLabels);
}

void cudaKernel::get_labels(LabelsType &labels)
{
	int deviceId = peak_device();
	int *h_labels;
	h_labels = (int*)malloc(numLabels * sizeof(int));
	runCuda(cudaMemcpy(h_labels, gpuCapabilities[deviceId].d_labels, numLabels * sizeof(int), cudaMemcpyDeviceToHost));

	labels.resize(numLabels);
	for (size_t i = 0; i < numLabels; i++)
	{
		labels[i] = h_labels[i];
	}

	free(h_labels);
}

void cudaKernel::update_labels(int *updateLabels, int numLabels, int deviceId)
{
	runCuda(cudaMemcpy(gpuCapabilities[deviceId].d_labels, updateLabels, sizeof(int)*numLabels, cudaMemcpyHostToDevice));
}

void cudaKernel::update_labels_to_all_other_devices(int srcDeviceId)
{
	for (std::map<int, gpuCapability>::iterator iter = gpuCapabilities.begin(); iter != gpuCapabilities.end(); iter++)
	{
		//printf("20\n");
		bool needToFree;
		device_to_device_copy(srcDeviceId, iter->first, numLabels, gpuCapabilities[srcDeviceId].d_labels, iter->second.d_labels, true, needToFree);
	}
}

void cudaKernel::update_labels_to_all_other_devices(int srcDeviceId, cudaStream_t& stream)
{
	for (std::map<int, gpuCapability>::iterator iter = gpuCapabilities.begin(); iter != gpuCapabilities.end(); iter++)
	{
//		printf("21\n");
		bool needToFree;
		device_to_device_copy(srcDeviceId, iter->first, numLabels, gpuCapabilities[srcDeviceId].d_labels, iter->second.d_labels, true, needToFree, stream);
	}
}

void cudaKernel::update_sub_labels_to_all_other_devices(int srcDeviceId)
{
	for (std::map<int, gpuCapability>::iterator iter = gpuCapabilities.begin(); iter != gpuCapabilities.end(); iter++)
	{
		//printf("22\n");

		bool needToFree;
		device_to_device_copy(srcDeviceId, iter->first, numLabels, gpuCapabilities[srcDeviceId].d_sub_labels, iter->second.d_sub_labels, true, needToFree);
	}
}

void cudaKernel::update_sub_labels_to_all_other_devices(int srcDeviceId, cudaStream_t& stream)
{
	for (std::map<int, gpuCapability>::iterator iter = gpuCapabilities.begin(); iter != gpuCapabilities.end(); iter++)
	{
		//printf("23\n");

		bool needToFree;
		device_to_device_copy(srcDeviceId, iter->first, numLabels, gpuCapabilities[srcDeviceId].d_sub_labels, iter->second.d_sub_labels, true, needToFree, stream);
	}
}

void cudaKernel::update_labels_by_max_index(double* parr, int dim, cudaStream_t& stream, int deviceId)
{
	update_labels_by_max_index_kernel << <blocks, threads, 0, stream >> > (parr, gpuCapabilities[deviceId].d_labels, numLabels, dim);

	update_labels_to_all_other_devices(deviceId, stream);
}


void cudaKernel::remove_empty_clusters_worker(int limit)
{
	int deviceId = peak_device();
	remove_empty_clusters_worker_all << <blocks, threads >> > (gpuCapabilities[deviceId].d_labels, numLabels, limit);
	runCuda(cudaPeekAtLastError());

	update_labels_to_all_other_devices(deviceId);
}

void cudaKernel::split_cluster_local_worker(LabelType index, LabelType newIndex)
{
	int deviceId = peak_device();
	split_cluster_local_worker_all << <blocks, threads >> > (gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_labels, numLabels, gpuCapabilities[deviceId].d_sub_labels, index, newIndex);
	runCuda(cudaPeekAtLastError());

	update_labels_to_all_other_devices(deviceId);
	update_sub_labels_to_all_other_devices(deviceId);
}

void cudaKernel::merge_clusters_worker(LabelType index, LabelType newIndex)
{
	int deviceId = peak_device();
	merge_clusters_worker_all << <blocks, threads >> > (gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_labels, numLabels, gpuCapabilities[deviceId].d_sub_labels, index, newIndex);
	runCuda(cudaPeekAtLastError());

	update_labels_to_all_other_devices(deviceId);
	update_sub_labels_to_all_other_devices(deviceId);
}

void cudaKernel::reset_bad_clusters_worker(LabelType index)
{
	int deviceId = peak_device();

	reset_bad_clusters_worker_all << <blocks, threads >> > (gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_labels, numLabels, gpuCapabilities[deviceId].d_sub_labels, index);
	runCuda(cudaPeekAtLastError());

	update_sub_labels_to_all_other_devices(deviceId);
}

void cudaKernel::get_sub_labels_count(int &l, int &r)
{
	int deviceId = peak_device();
	int *d_l;
	runCuda(cudaMalloc((void **)&d_l, sizeof(int)));
	runCuda(cudaMemset(d_l, 0, sizeof(int)));
	
	int *d_r;
	runCuda(cudaMalloc((void **)&d_r, sizeof(int)));
	runCuda(cudaMemset(d_r, 0, sizeof(int)));

	get_sub_labels_count_all << <blocks, threads >> > (gpuCapabilities[deviceId].d_sub_labels, numLabels, d_l, d_r);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());

	runCuda(cudaMemcpy(&l, d_l, sizeof(int), cudaMemcpyDeviceToHost));
	runCuda(cudaMemcpy(&r, d_r, sizeof(int), cudaMemcpyDeviceToHost));

	runCuda(cudaFree(d_l));
	runCuda(cudaFree(d_r));
}

// C(m,k) = A(m,n) * B(n,k)
void cudaKernel::naive_matrix_multiply(const double* A, const double* B, double* C, int m, int n, int k)
{
	int lda = m, ldb = k, ldc = m;
	const double alf = 1;
	const double bet = 0;
	const double* alpha = &alf;
	const double* beta = &bet;

	// Create a handle for CUBLAS
	cublasHandle_t handle;
	cublasCreate(&handle);

	// Do the actual multiplication
	cublasDgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc);

	// Destroy the handle
	cublasDestroy(handle);
}

// C(m,k) = A(m,n) * B(n,k)
void cudaKernel::naive_matrix_multiply_v2(const double* A, const double* B, double* C, int m, int n, int k, cudaStream_t& stream)
{
	int lda = m, ldb = k, ldc = m;
	const double alf = 1;
	const double bet = 0;
	const double* alpha = &alf;
	const double* beta = &bet;

	// Create a handle for CUBLAS
	cublasHandle_t handle;
	cublasCreate(&handle);
	cublasSetStream(handle, stream);
	// Do the actual multiplication
	cublasDgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc);

	// Destroy the handle
	cublasDestroy(handle);
}

void cudaKernel::naive_matrix_multiply_v3(double* A, double* B, double* C, int m, int n, int k, cudaStream_t& stream)
{
	const int BlockSize = 16;

	unsigned int grid_rows = (m + BlockSize - 1) / BlockSize;
	unsigned int grid_cols = (k + BlockSize - 1) / BlockSize;
	dim3 dimGrid(grid_cols, grid_rows);
	dim3 dimBlock(BlockSize, BlockSize);


//	printf("m=%d, n=%d, k=%d\n", m, n, k);
	if (k > 0)
	{
		gpu_matrix_mult << <dimGrid, dimBlock, 0, stream >> > (A, B, C, m, n, k);
		runCuda(cudaPeekAtLastError());
	}


//	runCuda(cudaMemcpy(A2, A, sizeof(double)* sizeVec, cudaMemcpyDeviceToHost));




}

void cudaKernel::dcolwise_dot_all(int maxIdx, int rows, double* d_a, double* d_b, double scalar, double* d_r, int r_offset)
{
	dcolwise_dot_all_kernel << <blocks, threads>> > (maxIdx, rows, d_a, d_b, scalar, d_r, r_offset);
}

void cudaKernel::dcolwise_dot_all_v2(int maxIdx, int rows, double* d_a, double* d_b, double scalar, double* d_r, int r_offset, cudaStream_t& stream)
{
	dcolwise_dot_all_kernel << <blocks, threads, 0, stream >> > (maxIdx, rows, d_a, d_b, scalar, d_r, r_offset);
}

void cudaKernel::dcolwise_dot_all_v3(int maxIdx, int rows, double* d_a, double* d_b, double scalar, double* d_r, double weight, cudaStream_t& stream)
{
	dcolwise_dot_with_log_kernel << <blocks, threads, 0, stream >> > (maxIdx, rows, d_a, d_b, scalar, d_r, weight);
}

//void cudaKernel::dcolwise_dot(Eigen::VectorXd& r, const Eigen::MatrixXd& a, const Eigen::MatrixXd& b)
//{
//	int sizeVec = a.cols();
//
//	double* d_a;
//	double* d_b;
//	double* d_c;
//	double* d_r;
//
//	runCuda(cudaMalloc((void**)&d_a, sizeof(double) * a.size()));
//	runCuda(cudaMalloc((void**)&d_b, sizeof(double) * b.size()));
//	runCuda(cudaMalloc((void**)&d_c, sizeof(double) * b.rows() * a.cols()));
//	runCuda(cudaMemcpy(d_a, a.data(), sizeof(double) * a.size(), cudaMemcpyHostToDevice));
//	runCuda(cudaMemcpy(d_b, b.data(), sizeof(double) * b.size(), cudaMemcpyHostToDevice));
//
//	naive_matrix_multiply(d_b, d_a, d_c, b.rows(), a.cols(), b.cols());
//
//	runCuda(cudaFree(d_b));
//
//	runCuda(cudaMalloc((void**)&d_r, sizeof(double)* sizeVec));
//
//	dcolwise_dot_all(sizeVec, a.rows(), d_a, d_c, d_r);
//
//	r.resize(sizeVec);
//	runCuda(cudaMemcpy(r.data(), d_r, sizeof(double)* sizeVec, cudaMemcpyDeviceToHost));
//	
//	runCuda(cudaFree(d_a));
//	runCuda(cudaFree(d_c));
//	runCuda(cudaFree(d_r));
//}

typedef struct
{
	//Stream for asynchronous command execution
	cudaStream_t stream;

	int* d_indices;
	int indicesSize;
	double* d_r;
	double* d_lr_weights;
	int deviceId;
} subclusters_labels_plan;

void cudaKernel::create_subclusters_labels(int numClusters, std::vector<thin_cluster_params*>& cluster_params, int dim)
//LabelType* indices, LabelType indicesSize, distribution_sample* l_dist, distribution_sample* r_dist, std::vector<double> &lr_weights)
{
//	omp_set_num_threads(20);
//	#pragma omp parallel
	{
//		unsigned int i = omp_get_thread_num();

//		printf("**** i=%d\n", i);
	}

	subclusters_labels_plan* plan = new subclusters_labels_plan[numClusters];

	//Allocate memory for all streams
	//omp_set_num_threads(numClusters);
	//#pragma omp parallel
	for (int i = 0; i < numClusters; i++)
	{
//		unsigned int i = omp_get_thread_num();

		plan[i].deviceId = peak_device_v2();
		runCuda(cudaStreamCreate(&(plan[i].stream)));
		runCuda(cudaMallocAsync((void**)&(plan[i].d_indices), sizeof(int) * numLabels, plan[i].stream));
		//printf("10\n");

		//Both
		runCuda(cudaMallocAsync((void**)&(plan[i].d_lr_weights), sizeof(double) * cluster_params[i]->lr_weights.size(), plan[i].stream));
		runCuda(cudaMemcpyAsync(plan[i].d_lr_weights, cluster_params[i]->lr_weights.data(), sizeof(double) * cluster_params[i]->lr_weights.size(), cudaMemcpyHostToDevice, plan[i].stream));
	}

	//printf("11\n");
	//omp_set_num_threads(numClusters);
	//#pragma omp parallel
	for (int i = 0; i < numClusters; i++)
	{
//		unsigned int i = omp_get_thread_num();

//		printf("i=%d, start log_likelihood_v2\n", i);
		cudaSetDevice(plan[i].deviceId);
		//Find indices
		//Can be used on any GPU
		//printf("12\n");
		sample_sub_clusters_worker_v2(i + 1, plan[i].d_indices, plan[i].indicesSize, plan[i].stream, plan[i].deviceId);
		//printf("13\n");

		//Return the likelihood in r vector.
		//Can be used on any GPU
		runCuda(cudaMallocAsync((void**)&(plan[i].d_r), sizeof(double) * plan[i].indicesSize * 2, plan[i].stream));

		//printf("14\n");
		log_likelihood_v2(plan[i].d_r, 0, plan[i].d_indices, plan[i].indicesSize, dim, cluster_params[i]->l_dist, plan[i].stream, plan[i].deviceId);
		log_likelihood_v2(plan[i].d_r, plan[i].indicesSize, plan[i].d_indices, plan[i].indicesSize, dim, cluster_params[i]->r_dist, plan[i].stream, plan[i].deviceId);
//		printf("i=%d, end log_likelihood_v2\n", i);
		//printf("15\n");
	}

	//omp_set_num_threads(numClusters);
	//#pragma omp parallel for
	for (int i = 0; i < numClusters; i++)
	{
		cudaSetDevice(plan[i].deviceId);
		runCuda(cudaStreamSynchronize(plan[i].stream));
	}
	//printf("16\n");

	//omp_set_num_threads(numClusters);
	//#pragma omp parallel for
	for (int i = 0; i < numClusters; i++)
	{
		//run on one GPU (plan[0].deviceId) - maybe could be optimized
		cudaSetDevice(plan[0].deviceId);
		
		//printf("1\n");
		int* d_indices;
		bool needToFree_d_indices;
		device_to_device_copy(plan[i].deviceId, plan[0].deviceId, plan[i].indicesSize, plan[i].d_indices, d_indices, false, needToFree_d_indices);

		//printf("2\n");
		double* d_lr_weights;
		bool needToFree_d_lr_weights;
		device_to_device_copy(plan[i].deviceId, plan[0].deviceId, cluster_params[i]->lr_weights.size(), plan[i].d_lr_weights, d_lr_weights, false, needToFree_d_lr_weights);

		//printf("3\n");
		double* d_r;
		bool needToFree_d_r;
		device_to_device_copy(plan[i].deviceId, plan[0].deviceId, plan[i].indicesSize * 2, plan[i].d_r, d_r, false, needToFree_d_r);

		sample_log_cat_array_sub_cluster_v2(d_r, plan[i].indicesSize, d_indices, plan[i].indicesSize, d_lr_weights, plan[0].stream, plan[0].deviceId);

		if (needToFree_d_indices)
		{
			runCuda(cudaFreeAsync(d_indices, plan[0].stream));
		}
		if (needToFree_d_lr_weights)
		{
			runCuda(cudaFreeAsync(d_lr_weights, plan[0].stream));
		}
		if (needToFree_d_r)
		{
			runCuda(cudaFreeAsync(d_r, plan[0].stream));
		}
	}

	//Wait for all operations to finish
	//omp_set_num_threads(numClusters);
	//#pragma omp parallel for
	for (int i = 0; i < numClusters; i++)
	{
		cudaSetDevice(plan[i].deviceId);
		runCuda(cudaStreamSynchronize(plan[i].stream));
	}

	//omp_set_num_threads(numClusters);
	//#pragma omp parallel for
	for (int i = 0; i < numClusters; i++)
	{
		cudaSetDevice(plan[i].deviceId);
		runCuda(cudaFreeAsync(plan[i].d_indices, plan[i].stream));
		runCuda(cudaFreeAsync(plan[i].d_r, plan[i].stream));

		runCuda(cudaFreeAsync(plan[i].d_lr_weights, plan[i].stream));

		runCuda(cudaStreamDestroy(plan[i].stream));
	}

	delete[]plan;
}

typedef struct
{
	//Stream for asynchronous command execution
	cudaStream_t stream;
	int deviceId;
	double* d_r;
} clusters_labels_plan;

void cudaKernel::create_clusters_labels(int numClusters, std::vector<thin_cluster_params*>& cluster_params, std::vector<double>& weights, bool bFinal)
{
	int masterDevice = -1;
	clusters_labels_plan* plan = new clusters_labels_plan[numClusters];
	const mv_gaussian* ds = (mv_gaussian*)(cluster_params[0]->cluster_dist);
	int dim = ds->invSigma.rows();

	//printf("Need %ld,  sizeof(double):%ld,  numLabels:%ld,  numClusters:%ld\n", sizeof(double) * numLabels * numClusters, sizeof(double) , numLabels , numClusters);

	//Allocate memory for all streams
	for (int i = 0; i < numClusters; i++)
	{
		plan[i].deviceId = peak_device();
		runCuda(cudaStreamCreate(&(plan[i].stream)));

		if (i == 0)
		{
			masterDevice = plan[i].deviceId;
			runCuda(cudaMalloc((void**)&(plan[i].d_r), sizeof(double) * numLabels * numClusters));
		}
		else if (masterDevice != plan[i].deviceId)
		{
			runCuda(cudaMalloc((void**)&(plan[i].d_r), sizeof(double) * numLabels));
		}

		//		printf("ds->invSigma.rows():%ld, ds->invSigma.cols():%ld, numLabels:%ld\n", ds->invSigma.rows(), ds->invSigma.cols(), numLabels);
	}

	//omp_set_num_threads(numClusters);
	//#pragma omp parallel for
	for (int i = 0; i < numClusters; i++)
	{
//		printf("i=%d, start log_likelihood_v3\n",i);
		cudaSetDevice(plan[i].deviceId);
		if (masterDevice == plan[i].deviceId)
		{
			log_likelihood_v3(plan[0].d_r + i * numLabels, dim, weights[i], cluster_params[i]->cluster_dist, plan[i].stream, plan[i].deviceId);
		}
		else
		{
			log_likelihood_v3(plan[i].d_r, dim, weights[i], cluster_params[i]->cluster_dist, plan[i].stream, plan[i].deviceId);
		}
//		printf("i=%d, end log_likelihood_v3\n", i);
	}

	//Wait for all operations to finish
	//omp_set_num_threads(numClusters);
	//#pragma omp parallel for
	for (int i = 0; i < numClusters; i++)
	{
		cudaSetDevice(plan[i].deviceId);
		runCuda(cudaStreamSynchronize(plan[i].stream));
	}

	//Copy d_r from all streams and GPU to one
	//omp_set_num_threads(numClusters);
	//#pragma omp parallel for
	for (int i = 0; i < numClusters; i++)
	{
		if (masterDevice != plan[i].deviceId)
		{
			//For improvement maybe worth to check if cudaDeviceCanAccessPeer
			double* r = new double[numLabels];

			cudaSetDevice(plan[i].deviceId);
			runCuda(cudaMemcpy(r, plan[i].d_r, numLabels * sizeof(double), cudaMemcpyDeviceToHost));

			cudaSetDevice(plan[0].deviceId);
			runCuda(cudaMemcpy(plan[0].d_r + i * numLabels, r, numLabels * sizeof(double), cudaMemcpyHostToDevice));

			delete []r;
		}
	}

	cudaSetDevice(plan[0].deviceId);
	if (bFinal)
	{
		update_labels_by_max_index(plan[0].d_r, numClusters, plan[0].stream, plan[0].deviceId);
	}
	else
	{
		sample_log_cat_array_v2(plan[0].d_r, numClusters, plan[0].stream, plan[0].deviceId);
	}

	//omp_set_num_threads(numClusters);
	//#pragma omp parallel for
	for (int i = 0; i < numClusters; i++)
	{
		cudaSetDevice(plan[i].deviceId);
		runCuda(cudaStreamSynchronize(plan[i].stream));
		runCuda(cudaStreamDestroy(plan[i].stream));
		if (i == 0 || masterDevice != plan[i].deviceId)
		{
			runCuda(cudaFree(plan[i].d_r));
		}
	}

	delete[]plan;
}

void cudaKernel::checkCUDAError(cudaError_t err, const char* file, int line)
{
	if (cudaSuccess != err)
	{
		printf("Cuda error: %s(%d):%s.\n", file, line, cudaGetErrorString(err));
		exit(EXIT_FAILURE);
	}
}

template<typename T>
void cudaKernel::device_to_device_copy(int srcDeviceId, int trgDeviceId, int dataSize, T* srcData, T*& trgData, bool alreadyAllocated, bool& needToFree, cudaStream_t& stream)
{
	if (srcDeviceId != trgDeviceId)
	{
		runCuda(cudaStreamSynchronize(stream));
	}
	device_to_device_copy(srcDeviceId, trgDeviceId, dataSize, srcData, trgData, alreadyAllocated, needToFree);
}

template<typename T>
void cudaKernel::device_to_device_copy(int srcDeviceId, int trgDeviceId, int dataSize, T* srcData, T* &trgData, bool alreadyAllocated, bool &needToFree)
{
	needToFree = false;
//	printf("alreadyAllocated=%d\n", alreadyAllocated);

	if (srcDeviceId == trgDeviceId)
	{
		trgData = srcData;
	}
	else
	{
		T* data = new T[dataSize];

		cudaSetDevice(srcDeviceId);
		runCuda(cudaMemcpy(data, srcData, dataSize * sizeof(T), cudaMemcpyDeviceToHost));

		cudaSetDevice(trgDeviceId);

		if (!alreadyAllocated)
		{
			runCuda(cudaMalloc((void**)&trgData, dataSize * sizeof(T)));
			needToFree = true;
		}

		runCuda(cudaMemcpy(trgData, data, dataSize * sizeof(T), cudaMemcpyHostToDevice));

		delete[]data;
	}
}

#endif