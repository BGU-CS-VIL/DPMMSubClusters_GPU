#ifndef CudaKernel_CU
#define CudaKernel_CU

//#pragma warning( disable : 2886 )
//#pragma warning( disable : 2929)

#include <cuda_runtime.h>
#include <cublas_v2.h>
#include<curand.h>
#include<curand_kernel.h>

#include<time.h>
#include "cudaKernel.cuh"

// function to define seed
__global__ void initCurand(curandState *state, unsigned long long seed, int maxIdx) {
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < maxIdx)
	{
		curand_init(seed, idx, 0, &state[idx]);
	}
}

//__device__ void sample_by_probability(curandState *state, double *weight, int numClusters, int rows, int idx, int *index)
//{
//	int cluster;
//	double u;
//	do 
//	{
//		cluster = (int)(curand_uniform(state) * (numClusters - 0.00001));
//		u = curand_uniform(state);
//	} while (u > weight[cluster*rows+idx]);
//	*index = cluster + 1;
//}

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

__global__ void dcolwise_dot_all_kernel(int maxIdx, int rows, double* d_a, double* d_b, double scalar, double* d_r)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;

	if (idx < maxIdx)
	{
		double sum = 0;
		for (int i = 0; i < rows; i++)
		{
			sum += d_a[idx * rows + i] * d_b[idx * rows + i];
		}

		d_r[idx] = scalar - sum / 2;
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

__global__ void build_log_likelihood_array_sub_cluster_kernel_v2(int maxIdx, double* d_rl, double* d_rr, double* d_lr_weights)
{
	int idx = threadIdx.x + blockIdx.x * blockDim.x;

	if (idx < maxIdx)
	{
		double maxRow;
		double sum;

		d_rl[idx] += log(d_lr_weights[0]);
		d_rr[idx] += log(d_lr_weights[1]);
		maxRow = fmax(d_rl[idx], d_rr[idx]);

		d_rl[idx] = exp(d_rl[idx] - maxRow);
		d_rr[idx] = exp(d_rr[idx] - maxRow);
		sum = d_rl[idx] + d_rr[idx];

		d_rl[idx] = d_rl[idx] / sum;
		d_rr[idx] = d_rr[idx] / sum;
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

void cudaKernel::init(int numLabelsIn, MatrixXd &points, unsigned long long seed)
{
	printf("Init cuda\n");

//	cudaSetDevice(1);
	runCuda(cudaDeviceReset());
	numLabels = numLabelsIn;
	threads = dim3(512);
	blocks = dim3(numLabels / threads.x + 1);
	runCuda(cudaMalloc((void**)&devState, numLabels * sizeof(curandState)));
	initCurand << <blocks, threads >> > (devState, seed, numLabels);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaPeekAtLastError());

	runCuda(cudaMalloc((void**)&d_labels, numLabels * sizeof(int)));
	runCuda(cudaMalloc((void**)&d_sub_labels, numLabels * sizeof(int)));
	runCuda(cudaMalloc((void**)&d_points, points.size() * sizeof(double)));
	runCuda(cudaMemcpy(d_points, points.data(), points.size() * sizeof(double), cudaMemcpyHostToDevice));
}

void cudaKernel::release()
{
	printf("Release cuda\n");

	if (devState != NULL)
	{
		runCuda(cudaFree(devState));
	}

	if (d_labels != NULL)
	{
		runCuda(cudaFree(d_labels));
	}

	if (d_sub_labels != NULL)
	{
		runCuda(cudaFree(d_sub_labels));
	}

	if (d_points != NULL)
	{
		runCuda(cudaFree(d_points));
	}
}

int cudaKernel::sample_log_cat_array_sub_cluster(LabelType *indices, int labelsSize, Eigen::MatrixXd &log_likelihood_array, std::vector<double>& lr_weights)
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
	
	sample_log_cat_array_sub_cluster_all << <blocks_size, threads >> > (devState, d_sub_labels, labelsSize, log_likelihood_array.cols(), d_log_likelihood_array, d_indices, d_y, d_a, d_b);
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

int cudaKernel::sample_log_cat_array_sub_cluster_v2(double* d_r, int* d_indices, int indicesSize, std::vector<double>& lr_weights, cudaStream_t& stream)
{
	double* d_y;
	int* d_a;
	int* d_b;
	double* d_lr_weights;

	runCuda(cudaMalloc((void**)&d_lr_weights, sizeof(double) * lr_weights.size()));
	runCuda(cudaMemcpy(d_lr_weights, lr_weights.data(), sizeof(double) * lr_weights.size(), cudaMemcpyHostToDevice));

	dim3 blocks_size = dim3(indicesSize / threads.x + 1);

	//int n = log_likelihood_array.size();
	//double* d_log_likelihood_array;
	//runCuda(cudaMalloc((void**)&d_log_likelihood_array, sizeof(double) * n));
	//runCuda(cudaMemcpy(d_log_likelihood_array, log_likelihood_array.data(), sizeof(double) * n, cudaMemcpyHostToDevice));
	runCuda(cudaMalloc((void**)&d_y, sizeof(double) * indicesSize * (2 + 2)));
	runCuda(cudaMalloc((void**)&d_a, sizeof(int) * indicesSize * (2 + 2)));
	runCuda(cudaMalloc((void**)&d_b, sizeof(int) * indicesSize * (2 + 2)));

	//	int dev = 0;//GPU index. need to loop for all GPUs

	build_log_likelihood_array_sub_cluster_kernel_v2 << <blocks_size, threads, 0, stream >> > (indicesSize, d_r, d_r + indicesSize, d_lr_weights);
	runCuda(cudaPeekAtLastError());

	sample_log_cat_array_sub_cluster_all << <blocks_size, threads, 0, stream >> > (devState, d_sub_labels, indicesSize, 2, d_r, d_indices, d_y, d_a, d_b);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaStreamSynchronize(stream));

	//free memory
	runCuda(cudaFree(d_lr_weights));
//	runCuda(cudaFree(d_log_likelihood_array));
	runCuda(cudaFree(d_y));
	runCuda(cudaFree(d_a));
	runCuda(cudaFree(d_b));
	return 0;
}

int cudaKernel::sample_log_cat_array(Eigen::MatrixXd &log_likelihood_array)
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

	sample_log_cat_array_all <<<blocks, threads>>>(devState, d_labels, numLabels, log_likelihood_array.cols(), d_log_likelihood_array, d_y, d_a, d_b);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());

	//free memory
	runCuda(cudaFree(d_log_likelihood_array));
	runCuda(cudaFree(d_y));
	runCuda(cudaFree(d_a));
	runCuda(cudaFree(d_b));
	return 0;

}

void cudaKernel::sample_sub_clusters_worker(LabelType label, LabelType* &indices, LabelType &indicesSize)
{
	int *d_indices;
	runCuda(cudaMalloc((void **)&d_indices, sizeof(int)*numLabels));

	int *d_indicesSize;
	runCuda(cudaMalloc(&d_indicesSize, sizeof(int)));
	runCuda(cudaMemset(d_indicesSize, 0, sizeof(int)));

	find_indices << <blocks, threads >> > (d_labels, numLabels, label, d_indices, d_indicesSize);

	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());

	runCuda(cudaMemcpy(&indicesSize, d_indicesSize, sizeof(int), cudaMemcpyDeviceToHost));
	runCuda(cudaMemcpy(indices, d_indices, indicesSize * sizeof(int), cudaMemcpyDeviceToHost));

	runCuda(cudaFree(d_indicesSize));
	runCuda(cudaFree(d_indices));
}

void cudaKernel::sample_sub_clusters_worker_v2(LabelType label, int* d_indices, int* d_indicesSize, int& indicesSize, cudaStream_t& stream)
{
	runCuda(cudaMemsetAsync(d_indicesSize, 0, sizeof(int), stream));

	find_indices << <blocks, threads, 0, stream >> > (d_labels, numLabels, label, d_indices, d_indicesSize);

	runCuda(cudaPeekAtLastError());
	//	runCuda(cudaDeviceSynchronize());

	runCuda(cudaMemcpyAsync(&indicesSize, d_indicesSize, sizeof(int), cudaMemcpyDeviceToHost, stream));
}

void cudaKernel::create_suff_stats_dict_worker(
	LabelType label,
	LabelType &indicesSize,
	Eigen::MatrixXd &group_pts,
	Eigen::MatrixXd* &pts,
	Eigen::MatrixXd* &pts1,
	Eigen::MatrixXd* &pts2)
{
	int *d_indices;
	runCuda(cudaMalloc((void **)&d_indices, sizeof(int)*numLabels));

	int *d_indicesSize;
	runCuda(cudaMalloc(&d_indicesSize, sizeof(int)));
	runCuda(cudaMemset(d_indicesSize, 0, sizeof(int)));

	find_indices << <blocks, threads >> > (d_labels, numLabels, label, d_indices, d_indicesSize);
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
		d_sub_labels,
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

void cudaKernel::sample_sub_labels()
{
	sample_sub_labels_all << <blocks, threads >> > (devState, d_sub_labels, numLabels);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());
}

void cudaKernel::sample_labels(int initial_clusters, double outlier_mod)
{
	sample_labels_all << <blocks, threads >> > (devState, d_labels, numLabels, initial_clusters, outlier_mod);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());
}

void cudaKernel::get_sub_labels(LabelsType &subLabels)
{
	int *h_subLabels;
	h_subLabels = (int*)malloc(numLabels * sizeof(int));
	runCuda(cudaMemcpy(h_subLabels, d_sub_labels, numLabels * sizeof(int), cudaMemcpyDeviceToHost));

	subLabels.resize(numLabels);
	for (size_t i = 0; i < numLabels; i++)
	{
		subLabels[i] = h_subLabels[i];
	}

	free(h_subLabels);
}

void cudaKernel::get_labels(LabelsType &labels)
{
	int *h_labels;
	h_labels = (int*)malloc(numLabels * sizeof(int));
	runCuda(cudaMemcpy(h_labels, d_labels, numLabels * sizeof(int), cudaMemcpyDeviceToHost));

	labels.resize(numLabels);
	for (size_t i = 0; i < numLabels; i++)
	{
		labels[i] = h_labels[i];
	}

	free(h_labels);
}

void cudaKernel::update_labels(int *updateLabels, int numLabels)
{
	runCuda(cudaMemcpy(d_labels, updateLabels, sizeof(int)*numLabels, cudaMemcpyHostToDevice));
}

void cudaKernel::remove_empty_clusters_worker(int limit)
{
	remove_empty_clusters_worker_all << <blocks, threads >> > (d_labels, numLabels, limit);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());
}

void cudaKernel::split_cluster_local_worker(LabelType index, LabelType newIndex)
{
	split_cluster_local_worker_all << <blocks, threads >> > (devState, d_labels, numLabels, d_sub_labels, index, newIndex);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());
}

void cudaKernel::merge_clusters_worker(LabelType index, LabelType newIndex)
{
	merge_clusters_worker_all << <blocks, threads >> > (devState, d_labels, numLabels, d_sub_labels, index, newIndex);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());
}

void cudaKernel::reset_bad_clusters_worker(LabelType index)
{
	reset_bad_clusters_worker_all << <blocks, threads >> > (devState, d_labels, numLabels, d_sub_labels, index);
	runCuda(cudaPeekAtLastError());
	runCuda(cudaDeviceSynchronize());
}

void cudaKernel::get_sub_labels_count(int &l, int &r)
{
	int *d_l;
	runCuda(cudaMalloc((void **)&d_l, sizeof(int)));
	runCuda(cudaMemset(d_l, 0, sizeof(int)));
	
	int *d_r;
	runCuda(cudaMalloc((void **)&d_r, sizeof(int)));
	runCuda(cudaMemset(d_r, 0, sizeof(int)));

	get_sub_labels_count_all << <blocks, threads >> > (d_sub_labels, numLabels, d_l, d_r);
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

void cudaKernel::dcolwise_dot_all(int maxIdx, int rows, double* d_a, double* d_b, double scalar, double* d_r)
{
	dcolwise_dot_all_kernel << <blocks, threads >> > (maxIdx, rows, d_a, d_b, scalar, d_r);
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

	int* d_indicesSize;

} subclusters_labels_plan;

void cudaKernel::create_subclusters_labels(int numClusters, std::vector<thin_cluster_params*> &cluster_params, int dim)
//LabelType* indices, LabelType indicesSize, distribution_sample* l_dist, distribution_sample* r_dist, std::vector<double> &lr_weights)
{
	subclusters_labels_plan *plan = new subclusters_labels_plan[numClusters];
	
	for (int i = 0; i < numClusters; i++)
	{
		runCuda(cudaMalloc((void**)&(plan[i].d_indices), sizeof(int) * numLabels));
		runCuda(cudaMalloc(&(plan[i].d_indicesSize), sizeof(int)));
	}

	for (int i = 0; i < numClusters; i++)
	{
		runCuda(cudaStreamCreate(&(plan[i].stream)));
		sample_sub_clusters_worker_v2(i + 1, plan[i].d_indices, plan[i].d_indicesSize, plan[i].indicesSize, plan[i].stream);
	}

	for (int i = 0; i < numClusters; i++)
	{
		runCuda(cudaStreamSynchronize(plan[i].stream));
		runCuda(cudaMalloc((void**)&(plan[i].d_r), sizeof(double) * plan[i].indicesSize * 2));
		log_likelihood_v2(plan[i].d_r, plan[i].d_indices, plan[i].indicesSize, dim, cluster_params[i]->l_dist, plan[i].stream);
		log_likelihood_v2(plan[i].d_r + plan[i].indicesSize, plan[i].d_indices, plan[i].indicesSize, dim, cluster_params[i]->r_dist, plan[i].stream);
		sample_log_cat_array_sub_cluster_v2(plan[i].d_r, plan[i].d_indices, plan[i].indicesSize, cluster_params[i]->lr_weights, plan[i].stream);
	}

	//Wait for all operations to finish
	for (int i = 0; i < numClusters; i++)
	{
		runCuda(cudaStreamSynchronize(plan[i].stream));
	}

	for (int i = 0; i < numClusters; i++)
	{
		runCuda(cudaStreamDestroy(plan[i].stream));

		runCuda(cudaFree(plan[i].d_indices));
		runCuda(cudaFree(plan[i].d_indicesSize));
		runCuda(cudaFree(plan[i].d_r));
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

#endif