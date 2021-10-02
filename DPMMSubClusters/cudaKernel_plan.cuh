#ifndef CudaKernel_plan_H
#define CudaKernel_plan_H

typedef struct
{
	//Stream for asynchronous command execution
	cudaStream_t stream;
	cudaStream_t stream1;
	cudaStream_t stream2;
	cudaStream_t stream3;
	std::shared_ptr<thin_suff_stats> tss;
	double* d_pts;
	double* d_pts1;
	double* d_pts2;
	int* d_indicesSize;
	LabelType indicesSize;
	int deviceId;
	int* d_j1;
	int* d_j2;
} sufficient_statistics_plan;

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

typedef struct
{
	//Stream for asynchronous command execution
	cudaStream_t stream;
	int deviceId;
	double* d_r;
} clusters_labels_plan;

#endif