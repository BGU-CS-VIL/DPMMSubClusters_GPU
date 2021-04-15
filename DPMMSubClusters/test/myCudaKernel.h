#pragma once

#include "cudaKernel_multinomial.cuh"
#include "cudaKernel_gaussian.cuh"

class myCudaKernel_multinomial : public cudaKernel_multinomial
{
public:
	void set_labels(LabelsType& labels)
	{
		cudaMemcpy(gpuCapabilities[0].d_labels, labels.data(), sizeof(int) * numLabels, cudaMemcpyHostToDevice);
	}

	void set_sub_labels(LabelsType& subLabels)
	{
		cudaMemcpy(gpuCapabilities[0].d_sub_labels, subLabels.data(), sizeof(int) * numLabels, cudaMemcpyHostToDevice);
	}


	void my_log_likelihood_labels(
		double* d_r,
		int dim,
		const std::shared_ptr<distribution_sample>& distribution_sample,
		cudaStream_t& stream,
		int deviceId)
	{
		log_likelihood_labels(d_r, dim, 0, distribution_sample, stream, deviceId);
	}

	void allocate_in_device(const MatrixXd& data, double*& d_data)
	{
		runCuda(cudaMalloc((void**)&d_data, sizeof(double) * data.size()));
		cudaMemcpy(d_data, data.data(), sizeof(double) * data.size(), cudaMemcpyHostToDevice);
	}

	void allocate_in_device(int size, double*& d_data)
	{
		runCuda(cudaMalloc((void**)&d_data, sizeof(double) * size));
	}

	void create_stream(cudaStream_t& stream)
	{
		runCuda(cudaStreamCreate(&stream));
	}

	void release_stream(cudaStream_t& stream)
	{
		runCuda(cudaStreamSynchronize(stream));
		runCuda(cudaStreamDestroy(stream));
	}

	void copy_from_device(double* d_data, int rows, int cols, MatrixXd& data)
	{
		data.resize(rows, cols);
		cudaMemcpy(data.data(), d_data, sizeof(double) * data.size(), cudaMemcpyDeviceToHost);
	}

	void copy_from_device(double* d_data, int size, VectorXd& data)
	{
		data.resize(size);
		cudaMemcpy(data.data(), d_data, sizeof(double) * data.size(), cudaMemcpyDeviceToHost);
	}

	void my_naive_matrix_multiply(double* A, double* B, double* C, int m, int n, int k, cudaStream_t& stream)
	{
		naive_matrix_multiply(A, B, C, m, n, k, stream);
	}

	void release_in_device(double* d_data)
	{
		runCuda(cudaFree(d_data));
	}
};

class myCudaKernel_gaussian : public cudaKernel_gaussian
{
public:
	void set_labels(LabelsType& labels)
	{
		cudaMemcpy(gpuCapabilities[0].d_labels, labels.data(), sizeof(int) * numLabels, cudaMemcpyHostToDevice);
	}

	void set_sub_labels(LabelsType& subLabels)
	{
		cudaMemcpy(gpuCapabilities[0].d_sub_labels, subLabels.data(), sizeof(int) * numLabels, cudaMemcpyHostToDevice);
	}

	void my_log_likelihood_labels(
		double* d_r,
		int dim,
		double weight,
		const std::shared_ptr<distribution_sample>& distribution_sample,
		cudaStream_t& stream,
		int deviceId)
	{
		log_likelihood_labels(d_r, dim, weight, distribution_sample, stream, deviceId);
	}

	void my_divide_points_by_mu_all(int dim, const mv_gaussian* dist, double* d_z, cudaStream_t& stream, int deviceId)
	{
		divide_points_by_mu_all(dim, dist, d_z, stream, deviceId);
	}

	void my_naive_matrix_multiply(double* A, double* B, double* C, int m, int n, int k, cudaStream_t& stream)
	{
		naive_matrix_multiply(A, B, C, m, n, k, stream);
	}

	void my_dcolwise_dot_all_labels(int maxIdx, int rows, double* d_a, double* d_b, double scalar, double* d_r, double weight, cudaStream_t& stream)
	{
		dcolwise_dot_all_labels(maxIdx, rows, d_a, d_b, scalar, d_r, weight, stream);
	}

	void allocate_in_device(const MatrixXd& data, double*& d_data)
	{
		runCuda(cudaMalloc((void**)&d_data, sizeof(double) * data.size()));
		cudaMemcpy(d_data, data.data(), sizeof(double) * data.size(), cudaMemcpyHostToDevice);
	}

	template<typename T>
	void allocate_in_device(int size, T*& d_data)
	{
		runCuda(cudaMalloc((void**)&d_data, sizeof(T) * size));
	}

	void allocate_in_device(const LabelsType& data, LabelType*& d_data)
	{
		runCuda(cudaMalloc((void**)&d_data, sizeof(LabelType) * data.size()));
		cudaMemcpy(d_data, data.data(), sizeof(LabelType) * data.size(), cudaMemcpyHostToDevice);
	}

	void create_stream(cudaStream_t& stream)
	{
		runCuda(cudaStreamCreate(&stream));
	}

	void release_stream(cudaStream_t& stream)
	{
		runCuda(cudaStreamSynchronize(stream));
		runCuda(cudaStreamDestroy(stream));
	}

	void copy_from_device(double* d_data, int rows, int cols, MatrixXd& data)
	{
		data.resize(rows, cols);
		cudaMemcpy(data.data(), d_data, sizeof(double) * data.size(), cudaMemcpyDeviceToHost);
	}

	void copy_from_device(double* d_data, int size, VectorXd& data)
	{
		data.resize(size);
		cudaMemcpy(data.data(), d_data, sizeof(double) * data.size(), cudaMemcpyDeviceToHost);
	}

	template<typename T>
	void release_in_device(T* d_data)
	{
		runCuda(cudaFree(d_data));
	}
};