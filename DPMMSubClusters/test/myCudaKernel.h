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


	void my_log_likelihood_v3(
		double* d_r,
		int dim,
		const distribution_sample* distribution_sample,
		cudaStream_t& stream,
		int deviceId)
	{
		log_likelihood_v3(d_r, dim, 0, distribution_sample, stream, deviceId);
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
		naive_matrix_multiply_v3(A, B, C, m, n, k, stream);
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

	void my_naive_matrix_multiply(double* A, double* B, double* C, int m, int n, int k, cudaStream_t& stream)
	{
		naive_matrix_multiply_v3(A, B, C, m, n, k, stream);
	}

	void release_in_device(double* d_data)
	{
		runCuda(cudaFree(d_data));
	}
};