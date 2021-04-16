#include <json/json.h>
#include <fstream>
#include "global_params.h"
#include "multinomial_prior.h"
#include "niw.h"

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

	Json::Value hyper_params_value = root["hyper_params"];
	if (hyper_params_value != NULL)
	{
		hyper_params = pPrior->create_hyperparams(hyper_params_value);
	}

	iterations = root["iterations"].asInt();
	initial_clusters = root["initial_clusters"].asInt();
	alpha = root["alpha"].asDouble();
	use_verbose = root["use_verbose"].asBool();
	draw_labels = root["draw_labels"].asBool();
	should_save_model = root["should_save_model"].asBool();
	burnout_period = root["burnout_period"].asInt();
	max_num_of_clusters = root["max_num_of_clusters"].asDouble();
	outlier_mod = root["outlier_mod"].asDouble();
	Json::Value outlier_hyper_params_value = root["outlier_hyper_params"];
	if (outlier_hyper_params_value != NULL)
	{
		outlier_hyper_params = pPrior->create_hyperparams(outlier_hyper_params_value);
	}
	unsigned long long randomSeed = root["randomSeed"].asUInt64();

	init_random(randomSeed);
	cuda = pPrior->get_cuda();
	numLabels = points.cols();
	cuda->init(numLabels, points, random_seed);
}

void global_params::init(int numLabels, MatrixXd& all_data, unsigned long long randomSeed, prior_type priorType)
{
	init_prior(priorType);
	init_random(randomSeed);

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
	rd = std::make_unique<std::random_device>(std::to_string(random_seed));
	std::random_device(std::to_string(random_seed));
	gen = std::make_unique<std::mt19937>((*rd)());
}

global_params::~global_params()
{
	if (cuda != NULL)
	{
		cuda->release();
	}
}
