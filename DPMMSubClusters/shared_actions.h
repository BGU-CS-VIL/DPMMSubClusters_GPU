#pragma once

#include <random>
#include "ds.h"
#include "global_params.h"

class shared_actions
{
public:
	shared_actions(std::shared_ptr<global_params>& globalParamsIn)
	{
		globalParams = globalParamsIn;
	}
	void sample_cluster_params(std::shared_ptr<splittable_cluster_params>& params, double alpha, bool first);
	void create_splittable_from_params(std::shared_ptr<cluster_parameters>& params, double alpha, std::shared_ptr<splittable_cluster_params>& scpOut);
	void should_merge(bool& should_merge, std::shared_ptr<cluster_parameters>& cpl, std::shared_ptr<cluster_parameters>& cpr, double alpha, bool bFinal);
	void merge_clusters_to_splittable(std::shared_ptr<splittable_cluster_params>& scpl, std::shared_ptr<cluster_parameters>& cpr, double alpha);

protected:
	virtual std::vector<double> get_dirichlet_distribution(std::vector<double>& points_count);
	virtual double get_uniform_real_distribution();

private:
	std::shared_ptr<global_params> globalParams;
};