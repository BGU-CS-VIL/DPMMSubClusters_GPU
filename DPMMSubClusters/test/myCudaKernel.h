#pragma once

#include "cudaKernel_multinomial.cuh"
#include "cudaKernel_gaussian.cuh"

class myCudaKernel_multinomial : public cudaKernel_multinomial
{
public:
	void set_labels(LabelsType& labels)
	{
		cudaMemcpy(d_labels, labels.data(), sizeof(int) * numLabels, cudaMemcpyHostToDevice);
	}

	void set_sub_labels(LabelsType& subLabels)
	{
		cudaMemcpy(d_sub_labels, subLabels.data(), sizeof(int) * numLabels, cudaMemcpyHostToDevice);
	}
};

class myCudaKernel_gaussian : public cudaKernel_gaussian
{
public:
	void set_labels(LabelsType& labels)
	{
		cudaMemcpy(d_labels, labels.data(), sizeof(int) * numLabels, cudaMemcpyHostToDevice);
	}

	void set_sub_labels(LabelsType& subLabels)
	{
		cudaMemcpy(d_sub_labels, subLabels.data(), sizeof(int) * numLabels, cudaMemcpyHostToDevice);
	}
};