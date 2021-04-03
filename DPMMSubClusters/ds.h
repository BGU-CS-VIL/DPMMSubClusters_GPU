#pragma once

//#include <iostream>
//#include <stdio.h>
#include <vector>
#include "distribution_sample.h"
#include "moduleTypes.h"
#include "Eigen/Dense"
#include "hyperparams.h"
#include "sufficient_statistics.h"

using namespace Eigen;

//Remove - use niw and multinomial_prior instead
//class distribution_hyper_params
//{
//};



//import Base.copy
//

struct model_hyper_params
{
	model_hyper_params() {}
	model_hyper_params(hyperparams* distribution_hyper_params, float alpha, DimensionsType total_dim) : distribution_hyper_params(distribution_hyper_params), alpha(alpha), total_dim(total_dim) {}
	hyperparams*  distribution_hyper_params;
	float alpha;
	DimensionsType total_dim;
	friend std::ostream& operator<<(std::ostream& os, const model_hyper_params& mhp)
	{
		os << "{distribution_hyper_params: " << mhp.distribution_hyper_params <<
			" || alpha: " << mhp.alpha <<
			" || total_dim: " << mhp.total_dim <<
			"}" << std::endl;
		return os;
	}
};

class cluster_parameters
{
public:
	cluster_parameters() : prior_hyperparams(NULL), distribution(NULL), suff_statistics(NULL), posterior_hyperparams(NULL) {}

	cluster_parameters(const cluster_parameters &cp)
	{
		copy(cp);
	}

	cluster_parameters *clone()
	{
		cluster_parameters* cp = new cluster_parameters();
		cp->copy(*this);
		return cp;
	}

	cluster_parameters& operator=(cluster_parameters other)
	{
		copy(other);
		return *this;
	}
	~cluster_parameters()
	{
		//raz  - to enable back
		if (prior_hyperparams != NULL)
		{
			delete prior_hyperparams;
			prior_hyperparams = NULL;
		}
		if (distribution != NULL)
		{
			delete distribution;
			distribution = NULL;
		}
		if (suff_statistics != NULL)
		{
			delete suff_statistics;
			suff_statistics = NULL;
		}
		//raz  - to enable back

		if (posterior_hyperparams != NULL)
		{
			delete posterior_hyperparams;
			posterior_hyperparams = NULL;
		}
	}

	void copy(const cluster_parameters &cp)
	{
		if (cp.prior_hyperparams != NULL)
		{
			prior_hyperparams = cp.prior_hyperparams->clone();
		}
		else
		{
			prior_hyperparams = NULL;
		}

		if (cp.distribution != NULL)
		{
			distribution = cp.distribution->clone();
		}
		else
		{
			distribution = NULL;
		}

		if (cp.suff_statistics != NULL)
		{
			suff_statistics = cp.suff_statistics->clone();
		}
		else
		{
			suff_statistics = NULL;
		}

		if (cp.posterior_hyperparams != NULL)
		{
			posterior_hyperparams = cp.posterior_hyperparams->clone();
		}
		else
		{
			posterior_hyperparams = NULL;
		}
	}

	hyperparams* prior_hyperparams;
	distribution_sample* distribution;
	sufficient_statistics *suff_statistics;
	hyperparams* posterior_hyperparams;
};

typedef std::vector<std::pair<bool, float>> Logsublikelihood_hist;

class splittable_cluster_params
{
public:
	splittable_cluster_params() : cluster_params(NULL), cluster_params_l(NULL), cluster_params_r(NULL), splittable(false) {}

	~splittable_cluster_params()
	{
		if (cluster_params != NULL)
		{
			delete cluster_params;
			cluster_params = NULL;
		}

		if (cluster_params_l != NULL)
		{
			delete cluster_params_l;
			cluster_params_l = NULL;
		}

		if (cluster_params_r != NULL)
		{
			delete cluster_params_r;
			cluster_params_r = NULL;
		}
	}

	splittable_cluster_params(const splittable_cluster_params &scp)
	{
		copy(scp);
	}

	splittable_cluster_params *clone()
	{
		splittable_cluster_params* scp = new splittable_cluster_params();
		scp->copy(*this);
		return scp;
	}

	splittable_cluster_params& operator=(splittable_cluster_params other)
	{
		copy(other);
		return *this;
	}

	void copy(const splittable_cluster_params &scp)
	{
		if (scp.cluster_params != NULL)
		{
			cluster_params = scp.cluster_params->clone();
		}
		else
		{
			cluster_params = NULL;
		}

		if (scp.cluster_params_l != NULL)
		{
			cluster_params_l = scp.cluster_params_l->clone();
		}
		else
		{
			cluster_params_l = NULL;
		}

		if (scp.cluster_params_r != NULL)
		{
			cluster_params_r = scp.cluster_params_r->clone();
		}
		else
		{
			cluster_params_r = NULL;
		}

		lr_weights = scp.lr_weights;
		splittable = scp.splittable;
		logsublikelihood_hist = scp.logsublikelihood_hist;
	}

	cluster_parameters *cluster_params;
	cluster_parameters *cluster_params_l;
	cluster_parameters *cluster_params_r;
	std::vector<double> lr_weights;
	bool splittable;
	Logsublikelihood_hist logsublikelihood_hist;//bool - true is set. false not set
};

class thin_cluster_params
{
public:
	thin_cluster_params(distribution_sample* cluster_dist, distribution_sample* l_dist, distribution_sample* r_dist, const std::vector<double> &lr_weights):cluster_dist(cluster_dist), l_dist(l_dist), r_dist(r_dist), lr_weights(lr_weights)
	{
	}
	~thin_cluster_params()
	{
	}
	distribution_sample* cluster_dist;
	distribution_sample* l_dist;
	distribution_sample* r_dist;
	std::vector<double> lr_weights;//between 0 to 1
};

struct thin_suff_stats
{
	sufficient_statistics *cluster_suff;
	sufficient_statistics *l_suff;
	sufficient_statistics *r_suff;
};

struct local_cluster
{
	local_cluster() : cluster_params(NULL), total_dim(0), points_count(0), l_count(0), r_count(0) {}

	~local_cluster()
	{
		if (cluster_params != NULL)
		{
			delete cluster_params;
			cluster_params = NULL;
		}
	}

	local_cluster(const local_cluster &lc)
	{
		copy(lc);
	}

	local_cluster *clone()
	{
		local_cluster* lc = new local_cluster();
		lc->copy(*this);
		return lc;
	}

	local_cluster& operator=(local_cluster other)
	{
		copy(other);
		return *this;
	}

	void copy(const local_cluster &lc)
	{
		if (lc.cluster_params != NULL)
		{
			cluster_params = lc.cluster_params->clone();
		}
		else
		{
			cluster_params = NULL;
		}

		total_dim = lc.total_dim;
		points_count = lc.points_count;
		l_count = lc.l_count;
		r_count = lc.r_count;
	}

	splittable_cluster_params *cluster_params;
	DimensionsType total_dim;
	PointType points_count;
	LabelType l_count;
	LabelType r_count;
};

struct local_group
{
	local_group() {}
	local_group(model_hyper_params model_hyperparams, MatrixXd points, LabelsType &labels_subcluster, std::vector<local_cluster*> local_clusters, std::vector<double> weights) :
		model_hyperparams(model_hyperparams), points(points), local_clusters(local_clusters), weights(weights) {}
	~local_group()
	{
		for (std::vector<local_cluster*>::iterator iter = local_clusters.begin(); iter != local_clusters.end(); iter++)
		{
			delete *iter;
		}
		local_clusters.clear();
	}

	LabelType num_labels()
	{
		return points.cols();
	}

	model_hyper_params model_hyperparams;
	MatrixXd points;
	std::vector<local_cluster*> local_clusters;
	std::vector<double> weights;
};

//
class pts_less_group
{
public:
	pts_less_group(model_hyper_params model_hyperparam, std::vector<local_cluster*> local_clusters, std::vector<double> weights) :
		model_hyperparams(model_hyperparams), local_clusters(local_clusters), weights(weights) {}
	model_hyper_params model_hyperparams;
//	LabelsType labels;
	std::vector<local_cluster*> local_clusters;
	std::vector<double> weights;

	friend std::ostream& operator<<(std::ostream& os, const pts_less_group& plg)
	{
		os << "{model_hyperparams: " << plg.model_hyperparams <<
			//" || labels: " << plg.labels.size() <<
			//" || labels_subcluster: " << plg.labels_subcluster.size() <<
			" || local_clusters: " << plg.local_clusters.size() <<
			" || weights: " << plg.weights.size() <<
			"}" << std::endl;
		return os;
	}
};

//
//mutable struct local_group_stats
//labels::AbstractArray {
//	Int64, 1
//}
//labels_subcluster::AbstractArray{ Int64,1 }
//local_clusters::Vector{ local_cluster }
//end
//
//

struct dp_parallel_sampling
{
	dp_parallel_sampling() {}
	dp_parallel_sampling(model_hyper_params model_hyperparams, local_group &group) :model_hyperparams(model_hyperparams), group(group) {}

	model_hyper_params model_hyperparams;
	local_group group;
};

//function copy_local_cluster(c::local_cluster)
//return deepcopy(c)
//end
//
//

class ds
{
public:
	static pts_less_group* create_pts_less_group(local_group &group)
	{
		return new pts_less_group(group.model_hyperparams, group.local_clusters, group.weights);
	}
};


//
//function create_model_from_saved_data(group::pts_less_group, points::AbstractArray{ Float32,2 }, model_hyperparams::model_hyper_params)
//group = local_group(group.model_hyperparams, points, group.labels, group.labels_subcluster, group.local_clusters, group.weights)
//return dp_parallel_sampling(model_hyperparams, group)
//end

