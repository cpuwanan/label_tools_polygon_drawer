#ifndef EDITOR_H
#define EDITOR_H

#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include "polygon_drawer/common.h"

class MyPolygonDrawer {
public:
	
	MyPolygonDrawer(int N = 4);
	~MyPolygonDrawer();
	void reset();
	void setImageSize(cv::Size size);
	void addRegion(std::string id);
	void addRegion(std::string id, MyPolygon polygon);
	void draw(cv::Mat &image);
	void deleteLastRegion();
	void deleteRegionById(std::string id);
	void editRegionById(std::string id, std::string name);
	std::string getTextInfo();
	void setActiveRegion(std::string id);
	void mouseSelectPoint(cv::Point pt);
	void mouseMovePoint(cv::Point pt);
	void mouseRelease();
	bool isOk(int mode = 0);
	
	std::map<std::string, MyPolygon> getPolygons() { return polygons_; }

private:
	std::vector<cv::Scalar> colors_;
	std::map<std::string, MyPolygon> polygons_;	
	int max_n_;
	std::string last_active_region_;
	int selected_pt_index_ = -1;
	cv::Size image_size_;
	cv::Point last_mouse_pt_;
};

#endif
