#pragma once

#include <random>
#include "ds.h"
#include "global_params.h"

class shared_actions
{
public:
	shared_actions(global_params *globalParamsIn)
	{
		globalParams = globalParamsIn;
	}
	void sample_cluster_params(splittable_cluster_params* &params, float alpha, bool first, prior *pPrior);
	void create_splittable_from_params(prior *pPrior, cluster_parameters* &params, double alpha, splittable_cluster_params* &scpOut);
	void should_merge(prior *pPrior, bool &should_merge, cluster_parameters* &cpl, cluster_parameters* &cpr, double alpha, bool bFinal);
	void merge_clusters_to_splittable(prior *pPrior, splittable_cluster_params* &scpl, cluster_parameters* &cpr, double alpha);

protected:
	virtual std::vector<double> get_dirichlet_distribution(std::vector<double> &points_count);

private:
	global_params *globalParams;
};

