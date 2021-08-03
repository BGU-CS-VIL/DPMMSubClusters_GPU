#include <fstream>
#include "global_params.h"
#include "multinomial_prior.h"
#include "niw.h"
#include "check_time.h"
#include "niw_hyperparams.h"
#include "multinomial_hyper.h"

void global_params::init(std::string modelParamsFileName, prior_type priorType)
{
	Json::Value root;
	std::ifstream ifs;
	ifs.open(modelParamsFileName);

	Json::CharReaderBuilder builder;
	builder["collectComments"] = true;
	JSONCPP_STRING errs;
	if (!parseFromStream(builder, ifs, &root, &errs))
	{
		printf("Failed to read file!\n");
		exit(EXIT_FAILURE);
	}

	init_prior(priorType);

	alpha = root.get("alpha", alpha).asDouble();
	Json::Value hyper_params_value = root["hyper_params"];
	if (!hyper_params_value.isNull())
	{
		hyper_params = pPrior->create_hyperparams(hyper_params_value);
	}
	else
	{
		hyper_params = pPrior->create_hyperparams();
	}

	iterations = root.get("iterations", iterations).asInt();
	initial_clusters = root.get("initial_clusters", initial_clusters).asInt();
	use_verbose = root.get("use_verbose", use_verbose).asBool();
	draw_labels = root.get("draw_labels", draw_labels).asBool();
	should_save_model = root.get("should_save_model", should_save_model).asBool();
	burnout_period = root.get("burnout_period", burnout_period).asInt();
	max_num_of_clusters = root.get("max_num_of_clusters", max_num_of_clusters).asDouble();
	outlier_mod = root.get("outlier_mod", outlier_mod).asDouble();
	Json::Value outlier_hyper_params_value = root["outlier_hyper_params"];
	if (!outlier_hyper_params_value.isNull())
	{
		outlier_hyper_params = pPrior->create_hyperparams(outlier_hyper_params_value);
	}
	else
	{
		outlier_hyper_params = pPrior->create_hyperparams();
	}
	unsigned long long randomSeed = root.get("randomSeed", 0).asUInt64();
	init_random(randomSeed);

	Json::Value gt_value = root["gt"];
	if (!gt_value.isNull())
	{
		ground_truth = std::make_shared<LabelsType>();
		for (Json::Value::iterator iter = gt_value.begin(); iter != gt_value.end(); iter++)
		{
			ground_truth->push_back(iter->asInt());
		}
	}

	cuda = pPrior->get_cuda();
	numLabels = points.cols();
	cuda->init(numLabels, points, random_seed);
}

void global_params::init(int numLabels, MatrixXd& all_data, unsigned long long randomSeed, prior_type priorType)
{
	CHECK_TIME("global_params::init");
	init_prior(priorType);
	init_random(randomSeed);
	hyper_params = pPrior->create_hyperparams();
	outlier_hyper_params = pPrior->create_hyperparams();

	cuda = pPrior->get_cuda();
	cuda->init(numLabels, all_data, random_seed);
	numLabels = numLabels;
	points = all_data;
}

void global_params::init_prior(prior_type priorType)
{
	if (priorType == prior_type::Gaussian)
	{
		pPrior = std::make_unique<niw>();
	}
	else if (priorType == prior_type::Multinomial)
	{
		pPrior = std::make_unique<multinomial_prior>();
	}
}


void global_params::init_random(unsigned long long randomSeed)
{
	random_seed = randomSeed;//When nothing, a random seed will be used.
	gen = std::make_unique<std::mt19937>(random_seed);
}

global_params::~global_params()
{
	if (cuda != NULL)
	{
		cuda->release();
	}
}
