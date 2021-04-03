/*
 * Copyright 1993-2015 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

/*
 * This application demonstrates how to use the CUDA API to use multiple GPUs,
 * with an emphasis on simple illustration of the techniques (not on performance).
 *
 * Note that in order to detect multiple GPUs in your system you have to disable
 * SLI in the nvidia control panel. Otherwise only one GPU is visible to the
 * application. On the other side, you can still extend your desktop to screens
 * attached to both GPUs.
 */

// System includes
#include <stdio.h>
#include <assert.h>

// CUDA runtime
#include <cuda_runtime.h>

// helper functions and utilities to work with CUDA
#include <helper_timer.h>
#include <helper_cuda.h>

#include "DPMMSubClusters.h"
#include "ds.h"
#include "module_tests.h"

#include <random>
#include "curand.h"
#include "cudaKernel.cuh"

////////////////////////////////////////////////////////////////////////////////
// Data configuration
////////////////////////////////////////////////////////////////////////////////
const int MAX_GPU_COUNT = 32;
const int DATA_N        = 1048576 * 32;

////////////////////////////////////////////////////////////////////////////////
// Simple reduction kernel.
// Refer to the 'reduction' CUDA Sample describing
// reduction optimization strategies
////////////////////////////////////////////////////////////////////////////////
__global__ static void reduceKernel(float *d_Result, float *d_Input, int N)
{
    const int     tid = blockIdx.x * blockDim.x + threadIdx.x;
    const int threadN = gridDim.x * blockDim.x;
    float sum = 0;

    for (int pos = tid; pos < N; pos += threadN)
        sum += d_Input[pos];

    d_Result[tid] = sum;
}

//std::mt19937_64 eng(rand()); //Use the 64-bit Mersenne Twister 19937 generator
							   //and seed it with entropy.

//__global__ void sample_log_cat_array_gpu(double *log_likelihood_array, int cols, int rows, double *labels)
//{
//	int idx = blockIdx.x * blockDim.x + threadIdx.x;
//
//	//Define the distribution, by default it goes from 0 to MAX(unsigned long long)
//
//		//Setup the weights (in this case linearly weighted)
//
//	std::vector<double> weights(cols);
//	for (int j = 0; j < cols; ++j)
//	{
//		weights[j] = log_likelihood_array[rows*j+idx];
//	}
//
//	// Create the distribution with those weights
//	std::discrete_distribution<int> distr(weights.begin(), weights.end());
//	//Generate random numbers
//	labels[idx] = distr(eng) + 1;
//}

/*
init_model()

Initialize the model, loading the data from external 'npy' files, specified in the params file.
All prior data as been included previously, and is globaly accessed by the function.

Returns an 'dp_parallel_sampling' (e.g.the main data structure) with the configured parameters and data.
*/

//dp_parallel_sampling init_model()
//{
//	dp_parallel_sampling result;
	//if random_seed != nothing
	//	@eval @everywhere seed!($random_seed)
	//	end
	//	if (use_verbose)
	//	{
//	println("Loading and distributing data:")
	//		@time data = distribute(Float32.(load_data(data_path, prefix = data_prefix)))
	//	else
	//	{	data = distribute(Float32.(load_data(data_path, prefix = data_prefix)))
	//		}
	//		total_dim = size(data, 2)
	//		model_hyperparams = model_hyper_params(hyper_params, ?, total_dim)

	//		labels = distribute(rand(1:initial_clusters, (size(data, 2))) . + ((outlier_mod > 0) ? 1 : 0))
	//		labels_subcluster = distribute(rand(1:2, (size(data, 2))))
	//		group = local_group(model_hyperparams, data, labels, labels_subcluster, local_cluster[], Float32[])
	//		return dp_parallel_sampling(model_hyperparams, group)
//	return result;
//}

/*
dp_parallel(model_params::String; verbose = true, save_model = true, burnout = 5, gt = nothing)

Run the model in advanced mode.
# Args and Kwargs
- 'model_params::String' A path to a parameters file(see below)
- 'verbose' will perform prints on every iteration.
- 'save_model' will save a checkpoint every 'X' iterations, where 'X' is specified in the parameter file.
- 'burnout' how long to wait after creating a cluster, and allowing it to split / merge
- 'gt' Ground truth, when supplied, will perform NMI and VI analysis on every iteration.

# Return values
dp_model, iter_count, nmi_score_history, liklihood_history, cluster_count_history
- 'dp_model' The DPMM model inferred
- 'iter_count' Timing for each iteration
- 'nmi_score_history' NMI score per iteration(if gt suppled)
- 'likelihood_history' Log likelihood per iteration.
- 'cluster_count_history' Cluster counts per iteration.
*/

/*
void dp_parallel(char* model_params, bool verbose = true, char* gt = NULL)
{
	include(model_params)
		global use_verbose = verbose
		dp_model = init_model()
		global leader_dict = get_node_leaders_dict()
		global should_save_model = enable_saving
		global ground_truth = gt
		global burnout_period = burnout_period
		global max_num_of_clusters = max_clusters
		init_first_clusters!(dp_model, initial_clusters)
		if (use_verbose)
		{
			println("Node Leaders:")
				println(leader_dict)
		}
	@eval @everywhere global hard_clustering = $hard_clustering
		return run_model(dp_model, 1, model_params)
}
*/


int main3(int argc, char **argv)
{
    //Solver config
    TGPUplan      plan[MAX_GPU_COUNT];

    //GPU reduction results
    float     h_SumGPU[MAX_GPU_COUNT];

    float sumGPU;

    int i, j, gpuBase, GPU_N;

    const int  BLOCK_N = 32;
    const int THREAD_N = 256;
    const int  ACCUM_N = BLOCK_N * THREAD_N;

    printf("Starting DPMMSubClusters\n");
    checkCudaErrors(cudaGetDeviceCount(&GPU_N));

    if (GPU_N > MAX_GPU_COUNT)
    {
        GPU_N = MAX_GPU_COUNT;
    }

    printf("CUDA-capable device count: %i\n", GPU_N);

    printf("Generating input data...\n\n");

    //Subdividing input data across GPUs
    //Get data sizes for each GPU
    for (i = 0; i < GPU_N; i++)
    {
        plan[i].dataN = DATA_N / GPU_N;
    }

    //Take into account "odd" data sizes
    for (i = 0; i < DATA_N % GPU_N; i++)
    {
        plan[i].dataN++;
    }

    //Assign data ranges to GPUs
    gpuBase = 0;

    for (i = 0; i < GPU_N; i++)
    {
        plan[i].h_Sum = h_SumGPU + i;
        gpuBase += plan[i].dataN;
    }

    //Create streams for issuing GPU command asynchronously and allocate memory (GPU and System page-locked)
    for (i = 0; i < GPU_N; i++)
    {
        checkCudaErrors(cudaSetDevice(i));
        checkCudaErrors(cudaStreamCreate(&plan[i].stream));
        //Allocate memory
        checkCudaErrors(cudaMalloc((void **)&plan[i].d_Data, plan[i].dataN * sizeof(float)));
        checkCudaErrors(cudaMalloc((void **)&plan[i].d_Sum, ACCUM_N * sizeof(float)));
        checkCudaErrors(cudaMallocHost((void **)&plan[i].h_Sum_from_device, ACCUM_N * sizeof(float)));
        checkCudaErrors(cudaMallocHost((void **)&plan[i].h_Data, plan[i].dataN * sizeof(float)));

        for (j = 0; j < plan[i].dataN; j++)
        {
            plan[i].h_Data[j] = (float)rand() / (float)RAND_MAX;
        }
    }

    //Start timing and compute on GPU(s)
    printf("Computing with %d GPUs...\n", GPU_N);
    // create and start timer
    StopWatchInterface *timer = NULL;
    sdkCreateTimer(&timer);

    // start the timer
    sdkStartTimer(&timer);

    //Copy data to GPU, launch the kernel and copy data back. All asynchronously
    for (i = 0; i < GPU_N; i++)
    {
        //Set device
        checkCudaErrors(cudaSetDevice(i));

        //Copy input data from CPU
        checkCudaErrors(cudaMemcpyAsync(plan[i].d_Data, plan[i].h_Data, plan[i].dataN * sizeof(float), cudaMemcpyHostToDevice, plan[i].stream));

        //Perform GPU computations
        reduceKernel<<<BLOCK_N, THREAD_N, 0, plan[i].stream>>>(plan[i].d_Sum, plan[i].d_Data, plan[i].dataN);
        getLastCudaError("reduceKernel() execution failed.\n");

        //Read back GPU results
        checkCudaErrors(cudaMemcpyAsync(plan[i].h_Sum_from_device, plan[i].d_Sum, ACCUM_N *sizeof(float), cudaMemcpyDeviceToHost, plan[i].stream));
    }

    //Process GPU results
    for (i = 0; i < GPU_N; i++)
    {
        float sum;

        //Set device
        checkCudaErrors(cudaSetDevice(i));

        //Wait for all operations to finish
        cudaStreamSynchronize(plan[i].stream);

        //Finalize GPU reduction for current subvector
        sum = 0;

        for (j = 0; j < ACCUM_N; j++)
        {
            sum += plan[i].h_Sum_from_device[j];
        }

        *(plan[i].h_Sum) = (float)sum;

        //Shut down this GPU
        checkCudaErrors(cudaFreeHost(plan[i].h_Sum_from_device));
        checkCudaErrors(cudaFree(plan[i].d_Sum));
        checkCudaErrors(cudaFree(plan[i].d_Data));
        checkCudaErrors(cudaStreamDestroy(plan[i].stream));
    }

    sumGPU = 0;

    for (i = 0; i < GPU_N; i++)
    {
        sumGPU += h_SumGPU[i];
    }

    sdkStopTimer(&timer);
    printf("  GPU Processing time: %f (ms)\n\n", sdkGetTimerValue(&timer));
    sdkDeleteTimer(&timer);
  
    // GPU results
    printf("  GPU sum: %f\n", sumGPU);

    // Cleanup and shutdown
    for (i = 0; i < GPU_N; i++)
    {
        checkCudaErrors(cudaSetDevice(i));
        checkCudaErrors(cudaFreeHost(plan[i].h_Data));
    }

    exit(EXIT_SUCCESS);
}

int main(int argc, char** argv)
{
 //   setNbThreads(12);
 //   Eigen::initParallel();
    printf("Eigen uses %ld threads\n", Eigen::nbThreads());

    module_tests mt;
    //	mt.RunModuleFromFile("mnm_data.npy", "E:\\VIL\\DPMMSubClusters\\x64\\Debug\\");
    //	MatrixXd mat;
    //	mt.ReadPnyFileIntoData("mnm_data.npy", "E:\\VIL\\DPMMSubClusters\\x64\\Debug\\", mat);
        //mt.RandomMess();
    mt.RandomMessHighDim();

    return main3(argc, argv);
}