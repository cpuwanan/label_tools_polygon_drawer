#include "editor.h"
#include "utils.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

MyPolygonDrawer::MyPolygonDrawer(int N)
	: max_n_(N)
	, last_active_region_("")
	, selected_pt_index_(-1)
{
	this->reset();
}

MyPolygonDrawer::~MyPolygonDrawer()
{
}

void MyPolygonDrawer::setImageSize(cv::Size size)
{
	image_size_ = size;
}

void MyPolygonDrawer::reset()
{
	polygons_.clear();
	image_size_ = cv::Size(0, 0);
	last_mouse_pt_ = cv::Point(0, 0);
	
	// Assume 20 regions
	for (int i=0; i<20; i++) {
		colors_.push_back(cv::Scalar(rand() % 200, rand() % 200, rand() % 200));
	}
}

void MyPolygonDrawer::addRegion(std::string id)
{
	if (!this->isOk(2)) return;
	
	if (id == "") return;
	
	std::vector<cv::Point2f> points;
	points.resize(max_n_);
	for (int i=0; i<max_n_; i++) {
		cv::Point2f pt(float(rand() % 50) / 100.0, float(rand() % 50) / 100.0);
		points[i] = pt;
	}
	
	polygons_.insert(std::pair<std::string, MyPolygon>(id, MyPolygon(id, points)));
	
	last_active_region_ = id;
	
	std::stringstream text;
	std::map<std::string, MyPolygon>::iterator it;
	for (it = polygons_.begin(); it != polygons_.end(); it++) {
		text << " |-- Id: " << it->first 
					<< ", size: " << it->second.points.size() << std::endl;
	}
	
	std::cout << " Add a new region: " << utils::getBashColorText(id, 'g', 'b') << std::endl;
	std::cout << text.str() << std::endl;
}

void MyPolygonDrawer::addRegion(std::string id, MyPolygon polygon)
{
	if (!this->isOk(2)) return;
	
	if (id == "") return;
	
	polygons_.insert(std::pair<std::string, MyPolygon>(id, polygon));
}

void MyPolygonDrawer::deleteLastRegion()
{
	if (!this->isOk()) return;
	
	std::map<std::string, MyPolygon>::iterator it = --polygons_.end();
	std::cout << utils::getBashColorText(" Deleting the region name: " + it->first, 'y', 'b') << std::endl;
	polygons_.erase(it);
}

void MyPolygonDrawer::deleteRegionById(std::string id)
{
	if (!this->isOk()) return;

	std::map<std::string, MyPolygon>::iterator it = polygons_.find(id);
	if (it != polygons_.end()) {
		polygons_.erase(it);
		std::cout << utils::getBashColorText(" Deleting the region name: " + it->first, 'y', 'b') << std::endl;
	}
}

void MyPolygonDrawer::editRegionById(std::string id, std::string name)
{
	if (!this->isOk()) return;

	if (name == "") return;
	
	std::map<std::string, MyPolygon>::iterator it = polygons_.find(id);
	if (it != polygons_.end()) {
		it->second.id = name;
		MyPolygon polygon = it->second;
		std::string text = cv::format("id '%s' was assigned a new name '%s'", it->first.c_str(), it->second.id.c_str());
		polygons_.erase(it);
		polygons_.insert(std::pair<std::string, MyPolygon>(name, polygon));
		std::cout << utils::getBashColorText(text, 'y', 'b') << std::endl;
	}
}

void MyPolygonDrawer::setActiveRegion(std::string id)
{
	last_active_region_ = id;
}

void MyPolygonDrawer::mouseSelectPoint(cv::Point pt)
{
	if (!this->isOk()) return;
	
	std::string selected_region("");
	selected_pt_index_ = -1;
	double min_dist = 40;
	std::map<std::string, MyPolygon>::iterator it;
	for (it = polygons_.begin(); it != polygons_.end(); it++) {
		for (int i=0; i<it->second.points.size(); i++) {
			cv::Point pt_i(it->second.points[i].x * image_size_.width, it->second.points[i].y * image_size_.height); 
			double dist = sqrt(pow(pt.x - pt_i.x, 2.0) + pow(pt.y - pt_i.y, 2.0));
			if (dist < min_dist) {
				min_dist = dist;
				selected_pt_index_ = i;
				selected_region = it->first;
			}
		}
	}
	
	if (selected_region != "" && selected_pt_index_ != -1) {
		last_active_region_ = selected_region;
	}
	
	last_mouse_pt_ = pt;
}

void MyPolygonDrawer::mouseMovePoint(cv::Point pt)
{
	if (!this->isOk()) return;
	
	if (last_active_region_ == "" || selected_pt_index_ < 0)  return;
	
	std::map<std::string, MyPolygon>::iterator it = polygons_.find(last_active_region_);
	
	if (it == polygons_.end())	return;
	
	if (selected_pt_index_ >= int(it->second.points.size())) return;
	
	float dx = float(pt.x - last_mouse_pt_.x) / image_size_.width;
	float dy = float(pt.y - last_mouse_pt_.y) / image_size_.height;
	it->second.points[selected_pt_index_] += cv::Point2f(dx, dy);
	
	last_mouse_pt_ = pt;
}

void MyPolygonDrawer::mouseRelease()
{
	selected_pt_index_ = -1;
}

void MyPolygonDrawer::draw(cv::Mat& image)
{
	if (polygons_.size() == 0) return;
	
	int region_index = 0;
	std::map<std::string, MyPolygon>::iterator it;
	for (it = polygons_.begin(); it != polygons_.end(); it++, region_index++) {
		if (true) {
			//cv::Scalar color = colors_[region_index % int(colors_.size())]; // cv::Scalar(255, 255, 255); //
			cv::Scalar color(0, 255, 0);
			
			int min_y = image.rows;
			int min_y_index = 0;
			for (int k = 0; k < it->second.points.size(); k++) {
				bool is_active_pt = (last_active_region_ == it->first) && selected_pt_index_ == k; 
				cv::Point2f ptf_1 = it->second.points[k];
				cv::Point2f ptf_2 = it->second.points[ (k+1) % int(it->second.points.size()) ];
				cv::Point pt1(int(ptf_1.x * image.cols), int(ptf_1.y * image.rows));
				cv::Point pt2(int(ptf_2.x * image.cols), int(ptf_2.y * image.rows));
				cv::line(image, pt1, pt2, color, 2, 8, 0);
				cv::circle(image, pt1, (is_active_pt ? 8 : 4), color, (is_active_pt ? 1 : -1));
				if (pt1.y < min_y) {
					min_y = pt1.y;
					min_y_index = k;
				}
				min_y = std::min(min_y, pt1.y);
			}
			int fontFace = cv::FONT_HERSHEY_SIMPLEX;
			double fontScale = 0.6;
			int thickness = 1;
			cv::Point textpt(
				it->second.points[min_y_index].x * image.cols,
				it->second.points[min_y_index].y * image.rows
			);
			int baseline = 0;
			cv::Size textsize = cv::getTextSize(it->second.id, fontFace, fontScale, thickness, &baseline);
			cv::Rect textRect(
				textpt.x, textpt.y - textsize.height - baseline,
				textsize.width, textsize.height + 2 * baseline
			);
			cv::rectangle(image, textRect, color, -1);
			cv::putText(image, it->second.id, textpt, fontFace, fontScale, cv::Scalar(0, 0, 0), thickness);
		}
	}	
}

std::string MyPolygonDrawer::getTextInfo()
{	
	std::string ids_str("");
	std::string polygons_str("");
	
	std::map<std::string, MyPolygon>::iterator it;
	for (it = polygons_.begin(); it != polygons_.end(); it++) {
		std::stringstream ss;
		ss << std::setprecision(3) << std::fixed;
		std::vector<cv::Point2f>::iterator it2;
		for (it2 = it->second.points.begin(); it2 != it->second.points.end(); it2++) {
			ss << "[" << it2->x << ", " << it2->y << "]";
			if (it2 != --it->second.points.end()) {
				ss << ", ";
			}
		}
		
		std::string sep1 = (it != --polygons_.end()) ? ", " : "";
		
		ids_str += ("'" + it->second.id + "'" + sep1);
		polygons_str += ("[" + ss.str() + "]" + sep1);
	}	
	return cv::format("w: %d, h: %d, ids: [%s], vertices: [%s]", image_size_.width, image_size_.height, ids_str.c_str(), polygons_str.c_str());
}

bool MyPolygonDrawer::isOk(int mode)
{
	bool ok1 = polygons_.size() > 0;
	bool ok2 = (image_size_.width > 0 && image_size_.height > 0);
	if (mode == 0) {
		return ok1 && ok2;
	} else if (mode == 1) {
		return ok1;
	} else if (mode == 2) {
		return ok2;
	}
	
	return false;
}

