#include "pch.h"
#include "gtest/gtest.h"
#include "priors/niw.h"
#include "Eigen/Dense"
#include "dp_parallel_sampling.h"
#include "data_generators.h"
#include "distributions/multinomial_dist.h"
#include "cudaKernel_multinomial.cuh"
#include "myCudaKernel.h"

namespace DPMMSubClustersTest
{
}