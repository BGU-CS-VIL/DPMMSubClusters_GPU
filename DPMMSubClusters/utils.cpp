#include <iostream>
#include <fstream>
#include <ctime>
#include <random>
#include <chrono>
#include "utils.h"
#include "priors/niw.h"
#include "priors/multinomial_prior.h"
//#include <ppl.h>
#include "cudaKernel.cuh"


#include "distributions_util/pdflib.hpp"


// We expects the data to be in npy format, return a dict of{ group: items }, each file is a different group
void utils::load_data(std::string path, std::string prefix, Eigen::MatrixXd& mat_out, bool swapDimension)
{
	//std::string fullPath = prefix + path;
	//cnpy::NpyArray npy_data = cnpy::npy_load(fullPath);

	//int data_row = npy_data.shape[1];
	//int data_col = npy_data.shape[0];

	//float* data = npy_data.data<float>();
	//mat_out.resize(data_row, data_col);
	//int l = 0;
	//for (size_t i = 0; i < data_row; i++)
	//{
	//	for (size_t j = 0; j < data_col; j++)
	//	{
	//		mat_out(i, j) = data[l];
	//		l++;
	//	}
	//}

//	float* ptr = static_cast<float *>(malloc(data_row * data_col * sizeof(float)));
//	memcpy(ptr, npy_data.data<float>(), data_row * data_col * sizeof(float));
//	 new (&mat_out) Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>>(reinterpret_cast<double *>(ptr), data_col, data_row);


	if (swapDimension)
	{
//		mat_out.adjointInPlace();
	}
}

void utils::saveToFile(const LabelsType & mat, const char * filePrefixName)
{
	ofstream myfile;
	static int iter = 0;
	++iter;
	std::string str = filePrefixName + std::to_string(iter) + ".csv";
	myfile.open(str.c_str());


	for (LabelType i = 0; i < mat.size(); i++)
	{
		myfile << mat[i] << "\n";
	}
	myfile.close();
}

void utils::saveToFile(const MatrixXd & mat, const char * filePrefixName)
{
	ofstream myfile;
	static int iter = 0;
	++iter;
	std::string str = filePrefixName + std::to_string(iter) + ".csv";
	myfile.open(str.c_str());


	for (BaseType i = 0; i < mat.rows(); i++)
	{
		for (BaseType j = 0; j < mat.cols(); j++)
		{
			myfile << mat(i, j) << ",";

		}
		myfile << "\n";
	}
	myfile.close();
}

void utils::saveToFile(const VectorXd & mat, const char * filePrefixName)
{
	ofstream myfile;
	static int iter = 0;
	++iter;
	std::string str = filePrefixName + std::to_string(iter) + ".csv";
	myfile.open(str.c_str());


	for (BaseType i = 0; i < mat.rows(); i++)
	{
		for (BaseType j = 0; j < mat.cols(); j++)
		{
			myfile << mat(i, j) << ",";

		}
		myfile << "\n";
	}
	myfile.close();
}

void utils::saveToFile(const MatrixXd & mat1, const MatrixXd & mat2, const char * filePrefixName)
{
	ofstream myfile;
	static int iter = 0;
	++iter;
	std::string str = filePrefixName + std::to_string(iter) + ".csv";
	myfile.open(str.c_str());


	for (BaseType i = 0; i < mat1.rows(); i++)
	{
		for (BaseType j = 0; j < mat1.cols(); j++)
		{
			myfile << mat1(i, j) << ",";

		}
		myfile << "\n";
	}

	myfile << "\n\n";

	for (BaseType i = 0; i < mat2.rows(); i++)
	{
		for (BaseType j = 0; j < mat2.cols(); j++)
		{
			myfile << mat2(i, j) << ",";

		}
		myfile << "\n";
	}
	myfile.close();
}

prior* utils::create_sufficient_statistics(hyperparams* dist, sufficient_statistics **suff_statistics, global_params *globalParams, const MatrixXd &pts)
{
	//TODO - shold remove it. we should have prior already
	prior *pPrior = NULL;
	if (dynamic_cast<niw_hyperparams*>(dist))
	{
		pPrior = new niw();
	}
	else if (dynamic_cast<multinomial_hyper*>(dist))
	{
		pPrior = new multinomial_prior();
	}
	*suff_statistics = pPrior->create_sufficient_statistics(dist, dist, pts);

	return pPrior;
}
/*

function get_labels_histogram(labels)
hist_dict = Dict()
for v in labels
if haskey(hist_dict, v) == false
hist_dict[v] = 0
end
hist_dict[v] += 1
end
return sort(collect(hist_dict), by = x->x[1])
end
*/
/*
void get_node_leaders_dict()
{
	leader_dict = Dict();
	cur_leader = (nworkers() == 0 ? procs() : workers())[1];
	leader_dict[cur_leader] = [];
		for i in(nworkers() == 0 ? procs() : workers())
		{
			if i in procs(cur_leader)
			{
				push!(leader_dict[cur_leader], i);
			}
			else
			{
				cur_leader = i;
				leader_dict[cur_leader] = [i];
			}
		}
	return leader_dict;
}
*/

double utils::log_multivariate_gamma(double x, long D)
{
	double res = D * (D - 1) / 4.0 * log(EIGEN_PI);
	for (long d = 1; d <= D; ++d)
	{
		res += r8_gamma_log((x + (1 - d) / 2.0));
	}
	return res;
}


void utils::dcolwise_dot(VectorXd &r, const MatrixXd &a, const MatrixXd &b)
{
	/*LabelType n = r.size();
	for (LabelType j = 0; j < n; j++)
	{
		double v = 0;
		for (DimensionsType i = 0; i < a.rows(); i++)
		{
			v += a(i, j) * b(i, j);
		}
		r(j) = v;
	}*/
	r = (a.cwiseProduct(b)).colwise().sum();

}