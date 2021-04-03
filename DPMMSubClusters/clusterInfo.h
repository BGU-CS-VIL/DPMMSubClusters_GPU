#pragma once

#include <string>
#include <vector>

using namespace std;

enum ClusterOperation
{
	Split,
	Remove,
	Merge
};

//class clusterInfo
//{
//
//public:
//	clusterInfo(long srcIn, long trgIn, long pointsSrcIn, long pointsTrgIn, ClusterOperation operationIn): src(srcIn), trg(trgIn), pointsSrc(pointsSrcIn), pointsTrg(pointsTrgIn), operation(operationIn){}
//	long src;
//	long trg;
//	long pointsSrc;
//	long pointsTrg;
//	ClusterOperation operation;
//};
//
//typedef vector<clusterInfo> ClusterInfoList;
//
//class clusterInfos
//{
//public:
//	void split(long from, long to, long points)
//	{
//		infos.push_back(clusterInfo(from, to, points, -1, ClusterOperation::Split));
//	}
//
//	void remove(long index, long points)
//	{
//		infos.push_back(clusterInfo(index, -1, points, -1, ClusterOperation::Remove));
//	}
//
//	void merge(long from, long to, long points, long pointsTo)
//	{
//		infos.push_back(clusterInfo(from, to, points, pointsTo, ClusterOperation::Merge));
//	}
//
//	void print() 
//	{
//		for (size_t i = 0; i < infos.size(); i++)
//		{
//			if (infos[i].operation == ClusterOperation::Split)
//			{
//				std::string str = "Split:" + std::to_string(infos[i].src) + "(" + std::to_string(infos[i].pointsSrc) + " points)=>" + std::to_string(infos[i].trg);
//				logger::debug(str.c_str());
//			}
//			else if (infos[i].operation == ClusterOperation::Remove)
//			{
//				std::string str = "Remove:" + std::to_string(infos[i].src) + "(" + std::to_string(infos[i].pointsSrc) + " points)";
//				logger::debug(str.c_str());
//			}
//			else if (infos[i].operation == ClusterOperation::Merge)
//			{
//				std::string str = "Merge:" + std::to_string(infos[i].src) + "(" + std::to_string(infos[i].pointsSrc) + " points)=>" + std::to_string(infos[i].trg) + "(" + std::to_string(infos[i].pointsTrg) + " points)";
//				logger::debug(str.c_str());
//			}
//		}
//	}
//private:
//	ClusterInfoList infos;
//
//};
