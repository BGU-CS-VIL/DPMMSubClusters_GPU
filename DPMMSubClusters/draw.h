#pragma once


#include <random>
#include <map>
#include <vector>
#include <array>
#include "Eigen/Dense"
#include "moduleTypes.h"
#include "opencv2/opencv.hpp"

using namespace Eigen;

class draw
{
public:	
	static void draw_labels(const char* fileName, MatrixXd &x, LabelsType& labels)
	{
		std::vector<double> labelsVec;
		for (size_t i = 0; i < labels.size(); i++)
		{
			labelsVec.push_back(labels[i]);
		}
		draw_labels(fileName, x, labelsVec);
	}

	static void draw_labels(const char* window, MatrixXd &x, std::vector<double>& labels)
	{
		double width = 800;
		double height = 800;

		double minX = x.row(0).minCoeff();
		double maxX = x.row(0).maxCoeff();

		double minY = x.row(1).minCoeff();
		double maxY = x.row(1).maxCoeff();

		cv::Mat* pIimageMat = new cv::Mat(width, height, CV_8UC3, cvScalar(255, 255, 255));
		IplImage image = IplImage(*pIimageMat);

		typedef std::pair<std::vector<double>, std::vector<double>> Pair;

		std::map<int, int> clusters;
		static std::vector<cv::Scalar> colors;
		std::random_device rd; // obtain a random number from hardware
		std::mt19937 rng(rd());
		std::uniform_int_distribution<int> dist(0, 255);

		for (size_t i = 0; i < labels.size(); i++)
		{
			cv::Point p;
			p.x = ((x(0, i) - minX) / (maxX - minX))*width;
			p.y = ((x(1, i) - minY) / (maxY - minY))*height;

			if (labels[i] < 1)
			{
				throw std::invalid_argument("Invalid label value");
			}

			int colorIndex;

			while (labels[i] > colors.size())
			{
				colors.push_back(cv::Scalar(dist(rng), dist(rng), dist(rng)));
			}
			colorIndex = labels[i] - 1;

			cvCircle(&image, p, 1, colors[colorIndex]);

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
		CvFont font;
		cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
		cvPutText(&image, title.c_str(), cvPoint(10, 10), &font, cvScalar(0));

		cvNamedWindow(window, CV_WINDOW_AUTOSIZE);
		cvShowImage(window, &image);
		cvWaitKey(1);		
		delete pIimageMat;
	}
};

