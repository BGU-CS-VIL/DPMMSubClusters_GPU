#ifndef CudaKernel_CU
#define CudaKernel_CU

#include <omp.h>

#include <cuda_runtime.h>
#include<curand.h>
#include<curand_kernel.h>
#include<time.h>
#include "cudaKernel.cuh"
#include "cudaKernel_plan.cuh"

#define TILE_DIM  32
#define BLOCK_ROWS  8

//Begin of kernels

// function to define seed
__global__ void initCurand(curandState *state, unsigned long long seed, int maxIdx) {
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < maxIdx)
	{
		curand_init(seed, idx, 0, &state[idx]);
	}
}

__device__ void sample_by_probability(curandState* state, double* weight, int numClusters, int rows, int idx, int* index,
	double* y, int* a, int* b)
{
	int i;
	int j;
	int k;
	int n = numClusters;

	a[idx] = 0;
	for (i = 1; i <= n; i++)
	{
		a[i * rows + idx] = i;
	}
	a[(n + 1) * rows + idx] = n + 1;

	b[idx] = 0;
	for (i = 1; i <= n; i++)
	{
		b[i * rows + idx] = i;
	}
	b[(n + 1) * rows + idx] = n + 1;

	y[idx] = 0.0;
	for (i = 1; i <= n; i++)
	{
		y[i * rows + idx] = weight[(i - 1) * rows + idx] * n;
	}
	y[(n + 1) * rows + idx] = 2.0;

	i = 0;
	j = n + 1;
	
	while (true)
	{
		do
		{
			i++;
		} while (y[b[i * rows + idx] * rows + idx] < 1.0);

		do
		{
			j--;
		} while (1.0 <= y[b[j * rows + idx] * rows + idx]);

		if (j <= i)
		{
			break;
		}

		k = b[i * rows + idx];
		b[i * rows + idx] = b[j * rows + idx];
		b[j * rows + idx] = k;
	}

	i = j;
	j++;

	while (0 < i)
	{

		while (y[b[j * rows + idx] * rows + idx] <= 1.0)
		{
			j++;
		}

		if (n < j)
		{
			break;
		}

		y[b[j * rows + idx] * rows + idx] = y[b[j * rows + idx] * rows + idx] - (1.0 - y[b[i * rows + idx] * rows + idx]);
		a[b[i * rows + idx] * rows + idx] = b[j * rows + idx];

		if (y[b[j * rows + idx] * rows + idx] < 1.0)
		{
			k = b[i * rows + idx];
			b[i * rows + idx] = b[j * rows + idx];
			b[j * rows + idx] = k;
			j++;
		}
		else
		{
			i--;
		}
	}

	double r;

	i = (int)(curand_uniform(state) * (n - 1 + 0.999999) + 1);
	r = curand_uniform(state);

	if (y[i * rows + idx] < r)
	{
		i = a[i * rows + idx];

	}
	*index = i;
}

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
	*d_label = (int)(curand_uniform(state) * (2 - 1 + 0.999999) + 1);
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
	*d_label = (int)(curand_uniform(state) * (initial_clusters - 1 + 0.999999) + 1 + ((outlier_mod > 0) ? 1 : 0));
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

		d_r[idx] = scalar - sum / 2 + log(weight);
	}
}

__global__ void build_log_likelihood_array_sub_cluster_kernel(int maxIdx, double* d_r, int r_offset, double* d_lr_weights)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;

	if (idx < maxIdx)
	{
		double maxRow;
		double sum;

		d_r[idx] += log(d_lr_weights[0]);
		d_r[idx + r_offset] += log(d_lr_weights[1]);
		maxRow = fmax(d_r[idx], d_r[idx + r_offset]);

		d_r[idx] = exp(d_r[idx] - maxRow);
		d_r[idx + r_offset] = exp(d_r[idx + r_offset] - maxRow);
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

	if (col < k && row < m)
	{
		for (int i = 0; i < n; i++)
		{
			sum += a[IDX2C(row, i, m)] * b[IDX2C(i, col, n)];
		}
		c[IDX2C(row, col, m)] = sum;
	}
}

__global__ void sum_rowwise_kernel(double* d_A, double* d_B, int rows, int cols)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < rows)
	{
		double sum = 0;
		for (int j = 0; j < cols; j++)
		{
			sum += d_A[IDX2C(idx, j, rows)];
		}
		d_B[idx] = sum;
	}
}

__global__ void nodiag_normalize(double* A, double* I, int n, int i) {
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x < n && y < n)
		if (x == i && x != y) {
			I[x * n + y] /= A[i * n + i];
			A[x * n + y] /= A[i * n + i];
		}

}

__global__ void diag_normalize(double* A, double* I, int n, int i) {
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x < n && y < n)
		if (x == y && x == i) {
			I[x * n + y] /= A[i * n + i];
			A[x * n + y] /= A[i * n + i];
		}
}

__global__ void gaussjordan(double* A, double* I, int n, int i)
{
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x < n && y < n) {
		if (x != i) {
			I[x * n + y] -= I[i * n + y] * A[x * n + i];
			if (y != i) {
				A[x * n + y] -= A[i * n + y] * A[x * n + i];
			}
		}
	}

}

__global__ void set_zero(double* A, double* I, int n, int i) {
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x < n && y < n) {
		if (x != i) {
			if (y == i) {
				A[x * n + y] = 0;
			}
		}
	}
}

__global__ void transposeGPUcoalescing(double* matIn, int n, int m, double* matTran) {
	__shared__ double tile[TILE_DIM][TILE_DIM];
	int i_n = blockIdx.x * TILE_DIM + threadIdx.x;
	int i_m = blockIdx.y * TILE_DIM + threadIdx.y; // <- threadIdx.y only between 0 and 7

	// Load matrix into tile
	// Every Thread loads in this case 4 elements into tile.
	int i;
	for (i = 0; i < TILE_DIM; i += BLOCK_ROWS) {
		if (i_n < n && (i_m + i) < m) {
			tile[threadIdx.y + i][threadIdx.x] = matIn[(i_m + i) * n + i_n];
		}
	}
	__syncthreads();

	i_n = blockIdx.y * TILE_DIM + threadIdx.x;
	i_m = blockIdx.x * TILE_DIM + threadIdx.y;

	for (i = 0; i < TILE_DIM; i += BLOCK_ROWS) {
		if (i_n < m && (i_m + i) < n) {
			matTran[(i_m + i) * m + i_n] = tile[threadIdx.x][threadIdx.y + i]; // <- multiply by m, non-squared!

		}
	}
}

//End of kernels

//cudaKernel class is the class that wraps all the calls to cuda APIs
void cudaKernel::init(int numLabelsIn, MatrixXd &points, unsigned long long seed, bool verbose, int forceKernel)
{
	use_verbose = verbose;

	if (use_verbose)
	{
		printf("Init cuda\n");
	}

	int numGPU;
	int driverVersion = 0, runtimeVersion = 0;

	lastDevice = 0;

	runCuda(cudaGetDeviceCount(&numGPU));

	//Remove the following line in order to run with more than 1 GPU
	numGPU = 1;

	if (use_verbose)
	{
		printf("Number of GPUs: %i\n", numGPU);
		printf("number of host CPUs:\t%d\n", omp_get_num_procs());
	}

	for (int i = 0; i < numGPU; i++)
	{
		cudaSetDevice(i);
		cudaDeviceProp deviceProp;
		cudaGetDeviceProperties(&deviceProp, i);

		cudaDriverGetVersion(&driverVersion);
		cudaRuntimeGetVersion(&runtimeVersion);

		if (use_verbose)
		{
			printf("\nDevice %d: \"%s\"\n", i, deviceProp.name);
			printf("CUDA Driver Version / Runtime Version          %d.%d / %d.%d\n", driverVersion / 1000, (driverVersion % 100) / 10, runtimeVersion / 1000, (runtimeVersion % 100) / 10);
			printf("CUDA Capability Major/Minor version number:    %d.%d\n", deviceProp.major, deviceProp.minor);
		}

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
			cudaFreeAsync(dummy, stream);
			gpuCapabilities[i] = gpuCapability();
		}
		cudaStreamSynchronize(stream);
		cudaStreamDestroy(stream);
	}
		
	if (use_verbose)
	{
		printf("\nNumber of GPUs that will be used: %i\n\n", (int)gpuCapabilities.size());
	}

	numLabels = numLabelsIn;
	if (use_verbose)
	{
		printf("Number of Labels: %i\n", numLabels);
	}

	threads = dim3(512);
	blocks = dim3(numLabels / threads.x + 1);

	for (std::map<int, gpuCapability>::iterator iter = gpuCapabilities.begin(); iter != gpuCapabilities.end(); iter++)
	{
		cudaSetDevice(iter->first);
		runCuda(cudaDeviceReset());
		runCuda(cudaMalloc((void**)&(iter->second.devState), numLabels * sizeof(curandState)));
		initCurand << <blocks, threads >> > (iter->second.devState, seed, numLabels);
		runCuda(cudaPeekAtLastError());
		runCuda(cudaDeviceSynchronize());

		runCuda(cudaMalloc((void**)&(iter->second.d_labels), numLabels * sizeof(int)));
		runCuda(cudaMalloc((void**)&(iter->second.d_sub_labels), numLabels * sizeof(int)));
		runCuda(cudaMalloc((void**)&(iter->second.d_points), points.size() * sizeof(double)));
		runCuda(cudaMemcpy(iter->second.d_points, points.data(), points.size() * sizeof(double), cudaMemcpyHostToDevice));
		iter->second.pointsRows = (int)points.rows();
		iter->second.pointsCols = (int)points.cols();

		optimize_kernels(iter->second, forceKernel);
	}

	if (gpuCapabilities.size() > 0)
	{
		cudaSetDevice(gpuCapabilities.begin()->first);
	}
}

void cudaKernel::optimize_kernels(gpuCapability& gpu, int forceKernel)
{
	if (forceKernel == 1)
	{
		gpu.do_multiplie_matrix_by_transpose = &do_multiplie_matrix_by_transpose1;
		gpu.matrixMultiply = &do_matrixMultiply1;
	}
	else if (forceKernel == 2)
	{
		gpu.do_multiplie_matrix_by_transpose = &do_multiplie_matrix_by_transpose2;
		gpu.matrixMultiply = &do_matrixMultiply2;
	}
	else
	{
		int numIter = 5;
		MatrixXd A = MatrixXd::Random(gpu.pointsRows, numLabels / 2);

		double* d_A;
		double* d_B;
		runCuda(cudaMalloc((void**)&(d_A), A.size() * sizeof(double)));
		runCuda(cudaMemcpy(d_A, A.data(), sizeof(double) * A.size(), cudaMemcpyHostToDevice));
		runCuda(cudaMalloc((void**)&(d_B), A.rows() * A.rows() * sizeof(double)));
		cudaStream_t stream;
		runCuda(cudaStreamCreate(&stream));

		gpu.matrixMultiply = &do_matrixMultiply1;
		do_multiplie_matrix_by_transpose1(d_A, d_B, (int)(A.rows()), (int)(A.cols()), gpu, stream, use_verbose);
		runCuda(cudaStreamSynchronize(stream));

		clock_t begin = clock();
		for (int i = 0; i < numIter; i++)
		{
			do_multiplie_matrix_by_transpose1(d_A, d_B, (int)(A.rows()), (int)(A.cols()), gpu, stream, use_verbose);
		}
		runCuda(cudaStreamSynchronize(stream));
		clock_t end = clock();
		double took1 = double(end - begin);

		gpu.matrixMultiply = &do_matrixMultiply2;
		do_multiplie_matrix_by_transpose2(d_A, d_B, (int)(A.rows()), (int)(A.cols()), gpu, stream, use_verbose);
		runCuda(cudaStreamSynchronize(stream));

		begin = clock();
		for (int i = 0; i < numIter; i++)
		{
			do_multiplie_matrix_by_transpose2(d_A, d_B, (int)(A.rows()), (int)(A.cols()), gpu, stream, use_verbose);
		}
		runCuda(cudaStreamSynchronize(stream));
		end = clock();
		double took2 = double(end - begin);

		runCuda(cudaFree(d_A));
		runCuda(cudaFree(d_B));
		runCuda(cudaStreamDestroy(stream));

		if (use_verbose)
		{
			printf("multiplie_matrix_by_transpose kernel:\n");
			printf("  Kernel 1 took: %f\n", took1);
			printf("  Kernel 2 took: %f\n", took2);
			printf("  Using kernel: #");
		}
		if (took1 < took2)
		{
			gpu.do_multiplie_matrix_by_transpose = &do_multiplie_matrix_by_transpose1;
			gpu.matrixMultiply = &do_matrixMultiply1;
			if (use_verbose)
			{
				printf("1\n");
			}
		}
		else
		{
			gpu.do_multiplie_matrix_by_transpose = &do_multiplie_matrix_by_transpose2;
			gpu.matrixMultiply = &do_matrixMultiply2;
			if (use_verbose)
			{
				printf("2\n");
			}
		}
	}
}

void cudaKernel::release()
{
	if (use_verbose)
	{
		printf("Release cuda\n");
	}

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

int cudaKernel::pick_first_device()
{
	int result;
	result = gpuCapabilities.begin()->first;
	cudaSetDevice(result);
	return result;
}

int cudaKernel::pick_any_device()
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
}

void cudaKernel::sample_log_cat_array_sub_cluster(
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

	build_log_likelihood_array_sub_cluster_kernel << <blocks, threads, 0, stream >> > (indicesSize, d_r, r_offset, d_lr_weights);
	runCuda(cudaPeekAtLastError());

	sample_log_cat_array_sub_cluster_all << <blocks, threads, 0, stream >> > (gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_sub_labels, indicesSize, 2, d_r, d_indices, d_y, d_a, d_b);
	runCuda(cudaPeekAtLastError());

	runCuda(cudaFreeAsync(d_y, stream));
	runCuda(cudaFreeAsync(d_a, stream));
	runCuda(cudaFreeAsync(d_b, stream));

	update_sub_labels_to_all_other_devices(deviceId, stream);
}

void cudaKernel::sample_log_cat_array(
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

void cudaKernel::sample_sub_clusters_worker(LabelType label, int* d_indices, int &indicesSize, cudaStream_t& stream, int deviceId)
{
	int* d_indicesSize;
	runCuda(cudaMallocAsync(&d_indicesSize, sizeof(int), stream));
	runCuda(cudaMemsetAsync(d_indicesSize, 0, sizeof(int), stream));

	find_indices << <blocks, threads, 0, stream >> > (gpuCapabilities[deviceId].d_labels, numLabels, label, d_indices, d_indicesSize);
	runCuda(cudaMemcpyAsync(&indicesSize, d_indicesSize, sizeof(int), cudaMemcpyDeviceToHost, stream));
	runCuda(cudaFreeAsync(d_indicesSize, stream));

	runCuda(cudaPeekAtLastError());
}

std::map<LabelType, std::shared_ptr<thin_suff_stats>> cudaKernel::create_sufficient_statistics(
	LabelsType& indices,
	const std::shared_ptr<hyperparams>& hyperParams,
	const std::shared_ptr<hyperparams>& posterior)
{
	std::map<LabelType, std::shared_ptr<thin_suff_stats>> suff_stats_dict;
	sufficient_statistics_plan* plan = new sufficient_statistics_plan[indices.size()];

	for (LabelType index = 0; index < indices.size(); index++)
	{
		LabelType label = indices[index] + 1;

		plan[index].deviceId = pick_any_device();
		runCuda(cudaStreamCreate(&(plan[index].stream)));

		int pointsRows = gpuCapabilities[plan[index].deviceId].pointsRows;
		int* d_indices;
		runCuda(cudaMallocAsync((void**)&d_indices, sizeof(int) * numLabels, plan[index].stream));

		runCuda(cudaMallocAsync(&(plan[index].d_indicesSize), sizeof(int), plan[index].stream));
		runCuda(cudaMemsetAsync(plan[index].d_indicesSize, 0, sizeof(int), plan[index].stream));

		find_indices << <blocks, threads, 0, plan[index].stream >> > (gpuCapabilities[plan[index].deviceId].d_labels, numLabels, label, d_indices, plan[index].d_indicesSize);
		runCuda(cudaPeekAtLastError());
		runCuda(cudaMemcpyAsync(&(plan[index].indicesSize), plan[index].d_indicesSize, sizeof(int), cudaMemcpyDeviceToHost));

		runCuda(cudaMallocAsync((void**)&(plan[index].d_pts), sizeof(double) * pointsRows * plan[index].indicesSize, plan[index].stream));

		runCuda(cudaMallocAsync((void**)&(plan[index].d_pts1), sizeof(double) * pointsRows * plan[index].indicesSize, plan[index].stream));

		runCuda(cudaMallocAsync((void**)&(plan[index].d_pts2), sizeof(double) * pointsRows * plan[index].indicesSize, plan[index].stream));

		runCuda(cudaMallocAsync(&(plan[index].d_j1), sizeof(int), plan[index].stream));
		runCuda(cudaMemsetAsync(plan[index].d_j1, 0, sizeof(int), plan[index].stream));
		runCuda(cudaMallocAsync(&(plan[index].d_j2), sizeof(int), plan[index].stream));
		runCuda(cudaMemsetAsync(plan[index].d_j2, 0, sizeof(int), plan[index].stream));

		dim3 blocks_size = dim3(numLabels / threads.x + 1);
		create_suff_stats_dict_worker_all << <blocks_size, threads, 0, plan[index].stream >> > (
			gpuCapabilities[plan[index].deviceId].d_sub_labels,
			numLabels,
			d_indices,
			plan[index].d_indicesSize,
			gpuCapabilities[plan[index].deviceId].d_points,
			pointsRows,
			plan[index].d_pts,
			plan[index].d_pts1,
			plan[index].d_pts2,
			plan[index].d_j1,
			plan[index].d_j2);
		runCuda(cudaPeekAtLastError());

		runCuda(cudaFreeAsync(d_indices, plan[index].stream));
	}
	
	for (LabelType index = 0; index < indices.size(); index++)
	{
		int pointsRows = gpuCapabilities[plan[index].deviceId].pointsRows;
		plan[index].tss = std::make_shared<thin_suff_stats>();

		runCuda(cudaStreamSynchronize(plan[index].stream));

		runCuda(cudaStreamCreate(&(plan[index].stream1)));
		do_create_sufficient_statistics(plan[index].d_pts1, pointsRows, plan[index].d_j1, hyperParams, posterior, plan[index].stream1, plan[index].tss->l_suff, plan[index].deviceId);

		runCuda(cudaStreamCreate(&(plan[index].stream2)));
		do_create_sufficient_statistics(plan[index].d_pts2, pointsRows, plan[index].d_j2, hyperParams, posterior, plan[index].stream2, plan[index].tss->r_suff, plan[index].deviceId);

		runCuda(cudaStreamCreate(&(plan[index].stream3)));
		do_create_sufficient_statistics(plan[index].d_pts, pointsRows, plan[index].d_indicesSize, hyperParams, posterior, plan[index].stream3, plan[index].tss->cluster_suff, plan[index].deviceId);

	}

	for (LabelType index = 0; index < indices.size(); index++)
	{
		runCuda(cudaStreamSynchronize(plan[index].stream1));
		runCuda(cudaStreamSynchronize(plan[index].stream2));
		runCuda(cudaStreamSynchronize(plan[index].stream3));

		runCuda(cudaStreamDestroy(plan[index].stream1));
		runCuda(cudaStreamDestroy(plan[index].stream2));
		runCuda(cudaStreamDestroy(plan[index].stream3));

		runCuda(cudaFreeAsync(plan[index].d_indicesSize, plan[index].stream));
		runCuda(cudaFreeAsync(plan[index].d_pts, plan[index].stream));
		runCuda(cudaFreeAsync(plan[index].d_pts1, plan[index].stream));
		runCuda(cudaFreeAsync(plan[index].d_pts2, plan[index].stream));
		runCuda(cudaFreeAsync(plan[index].d_j1, plan[index].stream));
		runCuda(cudaFreeAsync(plan[index].d_j2, plan[index].stream));
		runCuda(cudaStreamDestroy(plan[index].stream));

		suff_stats_dict[indices[index]] = plan[index].tss;
	}

	delete[]plan;
	return suff_stats_dict;
}

// A -> (N x M) 
//Call to the best kernel best of the dimensions that are needed in this run
void cudaKernel::multiplie_matrix_by_transpose(double* d_A, double* d_B, int N, int M, int deviceId, cudaStream_t& stream)
{
	gpuCapabilities[deviceId].do_multiplie_matrix_by_transpose(d_A, d_B, N, M, gpuCapabilities[deviceId], stream, use_verbose);
}

// A -> (N x M) 
//This method is good from low dimensions
void cudaKernel::do_multiplie_matrix_by_transpose1(double* d_A, double* d_B, int N, int M, gpuCapability &gpu, cudaStream_t& stream, bool use_verbose)
{
	dim3 blocks_size = dim3(N / TILE_DIM + 1, M / TILE_DIM + 1);
	dim3 threads = dim3(TILE_DIM, BLOCK_ROWS);

	double* d_A_T;
	runCuda(cudaMallocAsync(&d_A_T, sizeof(double) * N * M, stream));
	transposeGPUcoalescing << <blocks_size, threads, 0, stream >> > (d_A, N, M, d_A_T);
	runCuda(cudaPeekAtLastError());
	gpu.matrixMultiply(d_A, d_A_T, d_B, N, M, N, stream, use_verbose);
	runCuda(cudaFreeAsync(d_A_T, stream));
}

// A -> (N x M) 
//This method is good from high dimensions
void cudaKernel::do_multiplie_matrix_by_transpose2(double* d_A, double* d_B, int N, int M, gpuCapability& gpu, cudaStream_t &stream, bool use_verbose)
{
	cublasHandle_t handle;
	runCuda(cublasCreate(&handle));
	runCuda(cublasSetStream(handle, stream));
	double alpha = 1.0;
	double beta = 0.0;
	runCuda(cublasDgemm(handle, CUBLAS_OP_N, CUBLAS_OP_T, N, N, M, &alpha, d_A, N, d_A, N, &beta, d_B, N));

	runCuda(cublasDestroy(handle));
}

//C = A*B * (A* B)T
void cudaKernel::multiplie_matrix_for_inverseWishart(const MatrixXd& A, const MatrixXd& B, MatrixXd& C)
{
	cublasHandle_t handle;
	runCuda(cublasCreate(&handle));
	double alpha = 1.0;
	double beta = 0.0;
	double* d_A;
	double* d_B;
	double* d_C;
	double* d_temp;

	runCuda(cudaMalloc(&d_A, sizeof(double) * A.size()));
	runCuda(cudaMemcpy(d_A, A.data(), sizeof(double) * A.size(), cudaMemcpyHostToDevice));
	runCuda(cudaMalloc(&d_B, sizeof(double) * B.size()));
	runCuda(cudaMemcpy(d_B, B.data(), sizeof(double) * B.size(), cudaMemcpyHostToDevice));
	runCuda(cudaMalloc(&d_temp, sizeof(double) * A.rows() * B.cols()));
	runCuda(cudaMalloc(&d_C, sizeof(double) * A.rows() * A.rows()));

	// temp(m,k) = A(m,n) * B(n,k)
	int m = (int)(A.rows());
	int n = (int)(A.cols());
	int k = (int)(B.cols());
	runCuda(cublasDgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, m, k, n, &alpha, d_A, m, d_B, n, &beta, d_temp, m));

	// C(m,m) = temp(m,k) * temp(m,k)'
	runCuda(cublasDgemm(handle, CUBLAS_OP_N, CUBLAS_OP_T, m, m, k, &alpha, d_temp, m, d_temp, m, &beta, d_C, m));

	runCuda(cublasDestroy(handle));

	C.resize(A.rows(), A.rows());
	runCuda(cudaMemcpy(C.data(), d_C, sizeof(double) * A.rows() * A.rows(), cudaMemcpyDeviceToHost));

	runCuda(cudaFree(d_A));
	runCuda(cudaFree(d_B));
	runCuda(cudaFree(d_temp));
	runCuda(cudaFree(d_C));
}

void cudaKernel::create_suff_stats_dict_worker(
	LabelType label,
	LabelType& indicesSize,
	Eigen::MatrixXd& pts,
	Eigen::MatrixXd& pts1,
	Eigen::MatrixXd& pts2)
{
	int deviceId = pick_first_device();
	int pointsRows = gpuCapabilities[deviceId].pointsRows;
	int* d_indices;
	runCuda(cudaMalloc((void**)&d_indices, sizeof(int) * numLabels));

	int* d_indicesSize;
	runCuda(cudaMalloc(&d_indicesSize, sizeof(int)));
	runCuda(cudaMemset(d_indicesSize, 0, sizeof(int)));

	find_indices << <blocks, threads >> > (gpuCapabilities[deviceId].d_labels, numLabels, label, d_indices, d_indicesSize);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());
	runCuda(cudaMemcpy(&indicesSize, d_indicesSize, sizeof(int), cudaMemcpyDeviceToHost));

	double* d_pts;
	runCuda(cudaMalloc((void**)&d_pts, sizeof(double) * pointsRows * indicesSize));

	double* d_pts1;
	runCuda(cudaMalloc((void**)&d_pts1, sizeof(double) * pointsRows * indicesSize));

	double* d_pts2;
	runCuda(cudaMalloc((void**)&d_pts2, sizeof(double) * pointsRows * indicesSize));

	int* d_j1;
	int* d_j2;
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
		gpuCapabilities[deviceId].d_points,
		pointsRows,
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

	pts.resize(pointsRows, indicesSize);
	pts1.resize(pointsRows, j1);
	pts2.resize(pointsRows, j2);

	runCuda(cudaMemcpy(pts.data(), d_pts, sizeof(double) * pointsRows * indicesSize, cudaMemcpyDeviceToHost));
	runCuda(cudaMemcpy(pts1.data(), d_pts1, sizeof(double) * pointsRows * j1, cudaMemcpyDeviceToHost));
	runCuda(cudaMemcpy(pts2.data(), d_pts2, sizeof(double) * pointsRows * j2, cudaMemcpyDeviceToHost));

	runCuda(cudaFree(d_j1));
	runCuda(cudaFree(d_j2));
	runCuda(cudaFree(d_indices));
	runCuda(cudaFree(d_indicesSize));
	runCuda(cudaFree(d_pts));
	runCuda(cudaFree(d_pts1));
	runCuda(cudaFree(d_pts2));
}

void cudaKernel::sample_sub_labels()
{
	int deviceId = pick_first_device();
	sample_sub_labels_all << <blocks, threads >> > (gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_sub_labels, numLabels);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());

	update_sub_labels_to_all_other_devices(deviceId);
}

void cudaKernel::sample_labels(int initial_clusters, double outlier_mod)
{
	int deviceId = pick_first_device();
	sample_labels_all << <blocks, threads >> > (gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_labels, numLabels, initial_clusters, outlier_mod);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());

	update_labels_to_all_other_devices(deviceId);
}

void cudaKernel::get_sub_labels(std::shared_ptr<LabelsType> &subLabels)
{
	int deviceId = pick_first_device();
	int *h_subLabels;
	h_subLabels = (int*)malloc(numLabels * sizeof(int));
	runCuda(cudaMemcpy(h_subLabels, gpuCapabilities[deviceId].d_sub_labels, numLabels * sizeof(int), cudaMemcpyDeviceToHost));

	subLabels->resize(numLabels);
	for (size_t i = 0; i < numLabels; i++)
	{
		(*subLabels)[i] = h_subLabels[i];
	}

	free(h_subLabels);
}

void cudaKernel::get_labels(std::shared_ptr<LabelsType> &labels)
{
	int deviceId = pick_first_device();
	int *h_labels;
	h_labels = (int*)malloc(numLabels * sizeof(int));
	runCuda(cudaMemcpy(h_labels, gpuCapabilities[deviceId].d_labels, numLabels * sizeof(int), cudaMemcpyDeviceToHost));

	labels->resize(numLabels);
	for (size_t i = 0; i < numLabels; i++)
	{
		(*labels)[i] = h_labels[i];
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
		bool needToFree;
		device_to_device_copy(srcDeviceId, iter->first, numLabels, gpuCapabilities[srcDeviceId].d_labels, iter->second.d_labels, true, needToFree);
	}
}

void cudaKernel::update_labels_to_all_other_devices(int srcDeviceId, cudaStream_t& stream)
{
	for (std::map<int, gpuCapability>::iterator iter = gpuCapabilities.begin(); iter != gpuCapabilities.end(); iter++)
	{
		bool needToFree;
		device_to_device_copy(srcDeviceId, iter->first, numLabels, gpuCapabilities[srcDeviceId].d_labels, iter->second.d_labels, true, needToFree, stream);
	}
}

void cudaKernel::update_sub_labels_to_all_other_devices(int srcDeviceId)
{
	for (std::map<int, gpuCapability>::iterator iter = gpuCapabilities.begin(); iter != gpuCapabilities.end(); iter++)
	{
		bool needToFree;
		device_to_device_copy(srcDeviceId, iter->first, numLabels, gpuCapabilities[srcDeviceId].d_sub_labels, iter->second.d_sub_labels, true, needToFree);
	}
}

void cudaKernel::update_sub_labels_to_all_other_devices(int srcDeviceId, cudaStream_t& stream)
{
	for (std::map<int, gpuCapability>::iterator iter = gpuCapabilities.begin(); iter != gpuCapabilities.end(); iter++)
	{
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
	int deviceId = pick_first_device();
	remove_empty_clusters_worker_all << <blocks, threads >> > (gpuCapabilities[deviceId].d_labels, numLabels, limit);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());

	update_labels_to_all_other_devices(deviceId);
}

void cudaKernel::split_cluster_local_worker(LabelType index, LabelType newIndex)
{
	int deviceId = pick_first_device();
	split_cluster_local_worker_all << <blocks, threads >> > (gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_labels, numLabels, gpuCapabilities[deviceId].d_sub_labels, index, newIndex);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());

	update_labels_to_all_other_devices(deviceId);
	update_sub_labels_to_all_other_devices(deviceId);
}

void cudaKernel::merge_clusters_worker(LabelType index, LabelType newIndex)
{
	int deviceId = pick_first_device();
	merge_clusters_worker_all << <blocks, threads >> > (gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_labels, numLabels, gpuCapabilities[deviceId].d_sub_labels, index, newIndex);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());

	update_labels_to_all_other_devices(deviceId);
	update_sub_labels_to_all_other_devices(deviceId);
}

void cudaKernel::reset_bad_clusters_worker(LabelType index)
{
	int deviceId = pick_first_device();

	reset_bad_clusters_worker_all << <blocks, threads >> > (gpuCapabilities[deviceId].devState, gpuCapabilities[deviceId].d_labels, numLabels, gpuCapabilities[deviceId].d_sub_labels, index);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());

	update_sub_labels_to_all_other_devices(deviceId);
}

void cudaKernel::get_sub_labels_count(int &l, int &r)
{
	int deviceId = pick_first_device();
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
void cudaKernel::do_matrixMultiply1(double* d_A, double* d_B, double* d_C, int m, int n, int k, cudaStream_t& stream, bool use_verbose)
{
	const int BlockSize = 16;

	unsigned int grid_rows = (m + BlockSize - 1) / BlockSize;
	unsigned int grid_cols = (k + BlockSize - 1) / BlockSize;
	dim3 dimGrid(grid_cols, grid_rows);
	dim3 dimBlock(BlockSize, BlockSize);

	if (k > 0)
	{
		gpu_matrix_mult << <dimGrid, dimBlock, 0, stream >> > (d_A, d_B, d_C, m, n, k);
		runCuda(cudaPeekAtLastError());
	}
}

// C(m,k) = A(m,n) * B(n,k)
void cudaKernel::do_matrixMultiply2(double* d_A, double* d_B, double* d_C, int m, int n, int k, cudaStream_t& stream, bool use_verbose)
{
	cublasHandle_t handle;
	runCuda(cublasCreate(&handle));
	runCuda(cublasSetStream(handle, stream));
	double alpha = 1.0;
	double beta = 0.0;
	runCuda(cublasDgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, m, k, n, &alpha, d_A, m, d_B, n, &beta, d_C, m));

	runCuda(cublasDestroy(handle));
}

void cudaKernel::dcolwise_dot_all_sub_labels(int maxIdx, int rows, double* d_a, double* d_b, double scalar, double* d_r, int r_offset, cudaStream_t& stream)
{
	dcolwise_dot_all_kernel << <blocks, threads, 0, stream >> > (maxIdx, rows, d_a, d_b, scalar, d_r, r_offset);
}

void cudaKernel::dcolwise_dot_all_labels(int maxIdx, int rows, double* d_a, double* d_b, double scalar, double* d_r, double weight, cudaStream_t& stream)
{
	dcolwise_dot_with_log_kernel << <blocks, threads, 0, stream >> > (maxIdx, rows, d_a, d_b, scalar, d_r, weight);
}

void cudaKernel::create_subclusters_labels(int numClusters, std::vector<std::shared_ptr<thin_cluster_params>>& cluster_params, int dim)
{
	subclusters_labels_plan* plan = new subclusters_labels_plan[numClusters];

	//Allocate memory for all streams
	//omp_set_num_threads(numClusters);
	//#pragma omp parallel
	for (int i = 0; i < numClusters; i++)
	{
		plan[i].deviceId = pick_any_device();
		runCuda(cudaStreamCreate(&(plan[i].stream)));
		runCuda(cudaMallocAsync((void**)&(plan[i].d_indices), sizeof(int) * numLabels, plan[i].stream));

		//Both
		runCuda(cudaMallocAsync((void**)&(plan[i].d_lr_weights), sizeof(double) * cluster_params[i]->lr_weights.size(), plan[i].stream));
		runCuda(cudaMemcpyAsync(plan[i].d_lr_weights, cluster_params[i]->lr_weights.data(), sizeof(double) * cluster_params[i]->lr_weights.size(), cudaMemcpyHostToDevice, plan[i].stream));
	}

	//omp_set_num_threads(numClusters);
	//#pragma omp parallel
	for (int i = 0; i < numClusters; i++)
	{
		//Find indices
		//Can be used on any GPU
		sample_sub_clusters_worker(i + 1, plan[i].d_indices, plan[i].indicesSize, plan[i].stream, plan[i].deviceId);

		//Return the likelihood in r vector.
		//Can be used on any GPU
		runCuda(cudaMallocAsync((void**)&(plan[i].d_r), sizeof(double) * plan[i].indicesSize * 2, plan[i].stream));

		log_likelihood_sub_labels(plan[i].d_r, 0, plan[i].d_indices, plan[i].indicesSize, dim, cluster_params[i]->l_dist, plan[i].stream, plan[i].deviceId);
		log_likelihood_sub_labels(plan[i].d_r, plan[i].indicesSize, plan[i].d_indices, plan[i].indicesSize, dim, cluster_params[i]->r_dist, plan[i].stream, plan[i].deviceId);
	}

	//omp_set_num_threads(numClusters);
	//#pragma omp parallel for
	for (int i = 0; i < numClusters; i++)
	{
		runCuda(cudaStreamSynchronize(plan[i].stream));
	}

	//omp_set_num_threads(numClusters);
	//#pragma omp parallel for
	for (int i = 0; i < numClusters; i++)
	{
		cudaSetDevice(plan[0].deviceId);

		int* d_indices;
		bool needToFree_d_indices;
		device_to_device_copy(plan[i].deviceId, plan[0].deviceId, plan[i].indicesSize, plan[i].d_indices, d_indices, false, needToFree_d_indices);

		double* d_lr_weights;
		bool needToFree_d_lr_weights;
		device_to_device_copy(plan[i].deviceId, plan[0].deviceId, (int)cluster_params[i]->lr_weights.size(), plan[i].d_lr_weights, d_lr_weights, false, needToFree_d_lr_weights);

		double* d_r;
		bool needToFree_d_r;
		device_to_device_copy(plan[i].deviceId, plan[0].deviceId, plan[i].indicesSize * 2, plan[i].d_r, d_r, false, needToFree_d_r);

		sample_log_cat_array_sub_cluster(d_r, plan[i].indicesSize, d_indices, plan[i].indicesSize, d_lr_weights, plan[0].stream, plan[0].deviceId);

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

void cudaKernel::create_clusters_labels(int numClusters, std::vector<std::shared_ptr<thin_cluster_params>>& cluster_params, std::vector<double>& weights, bool bFinal)
{
	int masterDevice = -1;
	clusters_labels_plan* plan = new clusters_labels_plan[numClusters];

	//Allocate memory for all streams
	for (int i = 0; i < numClusters; i++)
	{
		plan[i].deviceId = pick_any_device();

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
	}

	//omp_set_num_threads(numClusters);
	//#pragma omp parallel for
	for (int i = 0; i < numClusters; i++)
	{
		cudaSetDevice(plan[i].deviceId);
		if (masterDevice == plan[i].deviceId)
		{
			log_likelihood_labels(plan[0].d_r + i * numLabels, weights[i], cluster_params[i]->cluster_dist, plan[i].stream, plan[i].deviceId);
		}
		else
		{
			log_likelihood_labels(plan[i].d_r, weights[i], cluster_params[i]->cluster_dist, plan[i].stream, plan[i].deviceId);
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

	//Copy d_r from all streams and GPU to one
	//omp_set_num_threads(numClusters);
	//#pragma omp parallel for
	for (int i = 0; i < numClusters; i++)
	{
		if (masterDevice != plan[i].deviceId)
		{
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
		sample_log_cat_array(plan[0].d_r, numClusters, plan[0].stream, plan[0].deviceId);
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

void cudaKernel::checkCUDAError(cublasStatus_t err, const char* file, int line)
{
	if (CUBLAS_STATUS_SUCCESS != err)
	{
		printf("Cuda error: %s(%d):%d.\n", file, line, err);
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

void cudaKernel::sum_rowwise(double* d_A, double* d_B, int rows, int cols, cudaStream_t& stream)
{
	dim3 blocks_size = dim3(rows / threads.x + 1);

	sum_rowwise_kernel << <blocks_size, threads, 0, stream >> > (d_A, d_B, rows, cols);
	runCuda(cudaPeekAtLastError());
}

void cudaKernel::inverse_matrix(const MatrixXd &A, MatrixXd& B)
{
	int blocksize = 8;
	int n = (int)(A.rows());
	int ddsize = n * n * sizeof(double);
	double* d_A, * d_B, *I;

	I = new double[n * n];

	for (int i = 0; i < n; i++) 
	{
		for (int j = 0; j < n; j++) {
			if (i == j) I[i * n + i] = 1.0;
			else I[i * n + j] = 0.0;
		}
	}

	dim3 threadsPerBlock(blocksize, blocksize);
	dim3 numBlocks((n + blocksize - 1) / blocksize, (n + blocksize - 1) / blocksize);
	runCuda(cudaMalloc((void**)&d_A, ddsize));
	runCuda(cudaMemcpy(d_A, A.data(), ddsize, cudaMemcpyHostToDevice));

	runCuda(cudaMalloc((void**)&d_B, ddsize));
	runCuda(cudaMemcpy(d_B, I, ddsize, cudaMemcpyHostToDevice));

	for (int i = 0; i < n; i++) 
	{
		nodiag_normalize << <numBlocks, threadsPerBlock >> > (d_A, d_B, n, i);
		diag_normalize << <numBlocks, threadsPerBlock >> > (d_A, d_B, n, i);
		gaussjordan << <numBlocks, threadsPerBlock >> > (d_A, d_B, n, i);
		set_zero << <numBlocks, threadsPerBlock >> > (d_A, d_B, n, i);
	}

	B.resize(n, n);
	runCuda(cudaDeviceSynchronize());

	runCuda(cudaMemcpy(B.data(), d_B, ddsize, cudaMemcpyDeviceToHost));

	runCuda(cudaFree(d_A));
	runCuda(cudaFree(d_B));
}

#endif