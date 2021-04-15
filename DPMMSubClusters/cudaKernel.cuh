#ifndef CudaKernel_H
#define CudaKernel_H

#include<curand_kernel.h>
#include "moduleTypes.h"
#include "Eigen/Dense"
#include "distribution_sample.h"
#include "ds.h"
#include <map>

struct gpuCapability
{
	int* d_labels;
	int* d_sub_labels;
	double* d_points;
	curandState* devState;
	int pointsRows;
	int pointsCols;
};

//extern "C"
class cudaKernel
{
public:
	virtual ~cudaKernel() {}
	void init(int numLabels, MatrixXd& points, unsigned long long seed);
	void release();
	int peak_first_device();
	int peak_any_device();
	void sample_log_cat_array_sub_cluster(
		double* d_r,
		int r_offset,
		int* d_indices,
		int indicesSize,
		double* d_lr_weights,
		cudaStream_t& stream,
		int deviceId);
	void sample_log_cat_array(
		double* d_r,
		int dim,
		cudaStream_t& stream,
		int deviceId);

	void update_labels_to_all_other_devices(int srcDeviceId);
	void update_labels_to_all_other_devices(int srcDeviceId, cudaStream_t& stream);
	void update_sub_labels_to_all_other_devices(int srcDeviceId);
	void update_sub_labels_to_all_other_devices(int srcDeviceId, cudaStream_t& stream);

	void sample_sub_clusters_worker(LabelType label, int* d_indices, int& indicesSize, cudaStream_t& stream, int deviceId);
	void sample_sub_labels();
	void sample_labels(int initial_clusters, double outlier_mod);
	void get_sub_labels(LabelsType& labels);
	void get_labels(LabelsType& labels);
	void update_labels(int* updateLabels, int numLabels, int deviceId);
	void update_labels_by_max_index(double* parr, int dim, cudaStream_t& stream, int deviceId);
	void remove_empty_clusters_worker(int limit);
	void split_cluster_local_worker(LabelType index, LabelType newIndex);
	void merge_clusters_worker(LabelType index, LabelType newIndex);
	void reset_bad_clusters_worker(LabelType index);
	void get_sub_labels_count(int& l, int& r);

	virtual void create_suff_stats_dict_worker(
		LabelType label,
		LabelType& indicesSize,
		Eigen::MatrixXd& pts,
		Eigen::MatrixXd& pts1,
		Eigen::MatrixXd& pts2);

	void create_subclusters_labels(int numClusters, std::vector<std::shared_ptr<thin_cluster_params>>& cluster_params, int dim);
	void create_clusters_labels(int numClusters, std::vector<std::shared_ptr<thin_cluster_params>>& cluster_params, std::vector<double>& weights, bool bFinal);

protected:
	dim3 threads;
	dim3 blocks;
	int numLabels;
	std::map<int, gpuCapability> gpuCapabilities;
	int lastDevice;

	void naive_matrix_multiply(double* A, double* B, double* C, int m, int n, int k, cudaStream_t& stream);
	void dcolwise_dot_all_sub_labels(int maxIdx, int rows, double* d_a, double* d_b, double scalar, double* d_r, int r_offset, cudaStream_t& stream);
	void dcolwise_dot_all_labels(int maxIdx, int rows, double* d_a, double* d_b, double scalar, double* d_r, double weight, cudaStream_t& stream);

	static void checkCUDAError(cudaError_t err, const char* file, int line);
	virtual void log_likelihood_sub_labels(
		double* d_r,
		int r_offset,
		int* d_indices,
		int indicesSize,
		int dim,
		const std::shared_ptr<distribution_sample>& distribution_sample,
		cudaStream_t& stream,
		int deviceId) = 0;
	virtual void log_likelihood_labels(
		double* d_r,
		double weight,
		const std::shared_ptr<distribution_sample>& distribution_sample,
		cudaStream_t& stream,
		int deviceId) = 0;
	template<typename T>
	void device_to_device_copy(int srcDeviceId, int trgDeviceId, int dataSize, T* srcData, T*& trgData, bool alreadyAllocated, bool &needToFree);
	template<typename T>
	void device_to_device_copy(int srcDeviceId, int trgDeviceId, int dataSize, T* srcData, T*& trgData, bool alreadyAllocated, bool& needToFree, cudaStream_t& stream);

};



#define IDX2C(i,j,ld) ((j)*(ld) + (i))
#define runCuda(ans) { cudaKernel::checkCUDAError((ans), __FILE__, __LINE__); }

#endif //CudaKernel_H
