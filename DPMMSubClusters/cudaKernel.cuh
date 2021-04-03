#ifndef CudaKernel_H
#define CudaKernel_H

#include<curand_kernel.h>
#include "moduleTypes.h"
#include "Eigen/Dense"
#include "distribution_sample.h"
#include "ds.h"

//extern "C"
class cudaKernel
{
public:
	void init(int numLabels, MatrixXd& points, unsigned long long seed);
	void release();
	int sample_log_cat_array_sub_cluster(LabelType *indices, int labelsSize, Eigen::MatrixXd &log_likelihood_array, std::vector<double>& lr_weights);
	int sample_log_cat_array_sub_cluster_v2(double* d_r, int* d_indices, int indicesSize, std::vector<double>& lr_weights, cudaStream_t& stream);

	int sample_log_cat_array(Eigen::MatrixXd &log_likelihood_array);
	void sample_sub_clusters_worker(LabelType label, LabelType* &indices, LabelType &indicesSize);
	void sample_sub_clusters_worker_v2(LabelType label, int* d_indices, int* d_indicesSize, int &indicesSize, cudaStream_t& stream);
	void sample_sub_labels();
	void sample_labels(int initial_clusters, double outlier_mod);
	void get_sub_labels(LabelsType &labels);
	void get_labels(LabelsType &labels);
	void update_labels(int *updateLabels, int numLabels);
	void remove_empty_clusters_worker(int limit);
	void split_cluster_local_worker(LabelType index, LabelType newIndex);
	void merge_clusters_worker(LabelType index, LabelType newIndex);
	void reset_bad_clusters_worker(LabelType index);
	void get_sub_labels_count(int &l, int &r);
		
	virtual void create_suff_stats_dict_worker(
		LabelType label,
		LabelType &indicesSize,
		Eigen::MatrixXd &group_pts,
		Eigen::MatrixXd* &pts,
		Eigen::MatrixXd* &pts1,
		Eigen::MatrixXd* &pts2);
//	void dcolwise_dot(Eigen::VectorXd& r, const Eigen::MatrixXd& a, const Eigen::MatrixXd& b);
	virtual void log_likelihood(Eigen::VectorXd& r, const Eigen::MatrixXd& x, const distribution_sample *distribution_sample) = 0;
	void create_subclusters_labels(int numClusters, std::vector<thin_cluster_params*>& cluster_params, int dim);

protected:
	dim3 threads;
	dim3 blocks;
	int numLabels;
	int *d_labels;
	int *d_sub_labels;
	double *d_points;
	curandState *devState;
	void naive_matrix_multiply(const double* A, const double* B, double* C, int m, int n, int k);
	void dcolwise_dot_all(int maxIdx, int rows, double* d_a, double* d_b, double scalar, double* d_r);
	static void checkCUDAError(cudaError_t err, const char* file, int line);
	virtual void log_likelihood_v2(double* d_r, int* d_indices, int indicesSize, int dim, const distribution_sample* distribution_sample, cudaStream_t& stream) = 0;
};



#define IDX2C(i,j,ld) ((j)*(ld) + (i))
#define runCuda(ans) { cudaKernel::checkCUDAError((ans), __FILE__, __LINE__); }

#endif //CudaKernel_H
