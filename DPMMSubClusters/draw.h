#pragma once


#include <random>
#include <map>
#include <vector>
#include <array>
#include <memory>
#include "Eigen/Dense"
#include "moduleTypes.h"
#ifdef WIN32
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#endif //WIN32
using namespace Eigen;

class draw
{
public:	
	static void draw_labels(const char* window, MatrixXd &x, std::shared_ptr<LabelsType> &labels)
	{
#ifdef WIN32
		const int width = 800;
		const int height = 800;

		double minX = x.row(0).minCoeff();
		double maxX = x.row(0).maxCoeff();

		double minY = x.row(1).minCoeff();
		double maxY = x.row(1).maxCoeff();

		cv::Mat image = cv::Mat(width, height, CV_8UC3, cv::Scalar(255, 255, 255));

		typedef std::pair<std::vector<double>, std::vector<double>> Pair;

		std::map<int, int> clusters;
		static std::vector<cv::Scalar> colors;
		std::random_device rd; // obtain a random number from hardware
		std::mt19937 rng(rd());
		std::uniform_int_distribution<int> dist(0, 255);

		for (size_t i = 0; i < labels->size(); i++)
		{
			cv::Point p;
			p.x = (int)(((x(0, i) - minX) / (maxX - minX)) * width);
			p.y = (int)(((x(1, i) - minY) / (maxY - minY)) * height);

			if ((*labels)[i] < 1)
			{
				throw std::invalid_argument("Invalid label value");
			}

			int colorIndex;

			while ((*labels)[i] > colors.size())
			{
				colors.push_back(cv::Scalar(dist(rng), dist(rng), dist(rng)));
			}
			colorIndex = (*labels)[i] - 1;

			cv::circle(image, p, 1, colors[colorIndex]);

			auto iter = clusters.find(colorIndex);
			if (iter == clusters.end())
			{
				clusters[colorIndex] = 1;
			}
			else 
			{
				clusters[colorIndex] += 1;
			}
		}

		std::string title = std::to_string(clusters.size()) + " Clusters";
		std::string title2;
		for (std::map<int, int>::const_iterator iter = clusters.begin(); iter != clusters.end(); iter++)
		{
			if (title2.size() > 0)
			{
				title2 += ",";
			}
			title2 += std::to_string(iter->first) + ":" + std::to_string(iter->second);
		}
		title += ": " + title2;
		int fontFace = cv::FONT_HERSHEY_SIMPLEX;
		double fontScale = 0.5;
		int thickness = 3;
		cv::putText(image, title.c_str(), cv::Point(10, 15), fontFace, fontScale, cv::Scalar::all(0));
		cv::namedWindow(window, cv::WINDOW_AUTOSIZE);
		cv::imshow(window, image);
		cv::waitKey(1);		
#endif //WIN32
	}
};

