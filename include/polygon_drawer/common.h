#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

struct MyPolygon {
	std::string id;
	std::vector<cv::Point2f> points;
	
	MyPolygon(){}
	
	MyPolygon(std::string _id, std::vector<cv::Point2f> _points) {
		id = _id;
		points = _points;
	}
};


struct LabelImageInfo {
	std::string name;
	std::string filename;
	cv::Mat image;
};

#endif
