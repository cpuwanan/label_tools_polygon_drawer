#include <opencv2/opencv.hpp>

#include <iostream>
#include <vector>
#include <sstream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <ctime>
#include <fstream>
#include <map>

#include <yaml-cpp/yaml.h>
#include <boost/filesystem.hpp>
#include <polygon_drawer/editor.h>

#include "utils.h"

const std::string CONFIG_FILE = "../config/polygon_drawer.yaml";

void checkResultDir(std::string target_dir) {
	boost::filesystem::path path(target_dir);
	if (!boost::filesystem::exists(path)) {
		std::cout << "NOT FOUND result dir: " << utils::getBashColorText(target_dir, 'r', 'b') << std::endl;
		std::cout << "Creating a new one ..." << std::endl;
		boost::filesystem::create_directories(path);
	}
}

class ImageEditor {
public:
	ImageEditor(std::string source_image_dir, std::string results_dir, std::string winname) 
		: appname_(winname), results_dir_(results_dir)
	{
		polygon_data_filename_ = cv::format("%s/polygon_drawer.yaml", results_dir_.c_str());
		is_ok_ = true;
		
		if (!this->setImageList(source_image_dir)) {
			std::cout << utils::getBashColorText("[Error] Failed loading images from " + source_image_dir, 'r', 'b') << std::endl;
			is_ok_ = false;
			return;
		}
		
		this->loadPreviousPolygonData(polygon_data_filename_);
		cv::namedWindow(appname_, 1);
		cv::setMouseCallback(appname_, &ImageEditor::onMouse, this);
	}
	
	static void onMouse(int event, int x, int y, int flags, void *param) {
		ImageEditor *pThis = (ImageEditor *) param;
		if (event == cv::EVENT_LBUTTONDOWN) {
			pThis->mouseClick(cv::Point(x, y));
		} else if (event == cv::EVENT_MOUSEMOVE) {
			pThis->mouseMove(cv::Point(x, y));
		} else if (event == cv::EVENT_LBUTTONUP) {
			pThis->mouseRelease();
		}
	}
	
	void mouseClick(cv::Point pt) {
		current_drawer_.mouseSelectPoint(pt);
	}
	
	void mouseMove(cv::Point pt) {
		current_drawer_.mouseMovePoint(pt);
	}
	
	void mouseRelease() {
		current_drawer_.mouseRelease();
	}
	
	void loadPreviousPolygonData(std::string file) {
		std::ifstream reader(file);
		if (!reader.is_open()) {
			std::cout << utils::getBashColorText("[Warning] Polygon data file is not available: " + file, 'y', 'b') << std::endl;
			std::cout << "[Ok] Skip initialization of polygon data" << std::endl;
			return;
		}
		reader.close();
		
		std::cout << "[Ok] Initialized polygon data" << std::endl;
		YAML::Node node = YAML::LoadFile(polygon_data_filename_);
		if (node["polygons"]) {
			if (node["polygons"].size() > 0) {
				for (int i=0; i<(int)node["polygons"].size(); i++) {
					MyPolygonDrawer drawer;

					auto data = node["polygons"][i];
					std::string name = data["name"].as<std::string>();
					cv::Size image_size(data["w"].as<int>(), data["h"].as<int>());
					
					drawer.setImageSize(image_size);
					
					std::cout << " " << name << ", Size: " << image_size.width << " x " << image_size.height << std::endl;
					if (data["ids"] && data["vertices"]) {
						if (data["ids"].size() == data["vertices"].size()) {
							
							int index = 0;
							for (auto single_box : data["vertices"]) {
								std::stringstream ss;
								ss << std::setprecision(3) << std::fixed;
								std::vector<cv::Point2f> points;
								for (auto vertice : single_box) {
									cv::Point2f pt(vertice[0].as<double>(), vertice[1].as<double>());
									points.push_back(pt);
									ss << "(" << pt.x << ", " << pt.y << ") ";
								}
								std::string id = data["ids"][index].as<std::string>();
								std::cout << "    >> " << id << ": " << ss.str() << std::endl;
								index++;
								drawer.addRegion(id, MyPolygon(id, points));
							}
						}	
					}
					
					drawer_list_.insert(std::pair<std::string, MyPolygonDrawer>(name, drawer));
				}
			}
		}
	
		std::cout << utils::getBashColorText(cv::format("[Ok] Successfully set %d drawers", int(drawer_list_.size())), 'g', 'b') << std::endl;

	}
	
	void run() {
		if (!is_ok_) { return; }
		
		int index = 0;		
		while (is_ok_) {
					
			std::cout << "\n------------------------- " << std::endl;
			std::cout << "Index: " << index << std::endl;
			auto item = image_list_[index];
			cv::Mat image = item.image.clone();
			
			if (image.empty()) { 
				std::cout << " .. Error: Invalid image for " << utils::getBashColorText(image_list_[index].name, 'r', 'b') << std::endl;
				sleep(1);
				continue; 
			}
			
			bool is_drawing_ = true;
			
			std::map<std::string, MyPolygonDrawer>::iterator it = drawer_list_.find(item.name);
			if (it != drawer_list_.end()) {
				std::cout << "Found previous polygons: " << utils::getBashColorText(item.name, 'g', 'b') << std::endl;
				current_drawer_ = it->second;
			} else {
				std::cout << "Created a new polygon" << std::endl;
				current_drawer_ = MyPolygonDrawer();
			}
	
			current_drawer_.setImageSize(image.size());
			this->drawImageHeader(image, item.name);
			
			while (is_drawing_) {
				
				cv::Mat frame = image.clone();
				current_drawer_.draw(frame);
				cv::imshow(appname_, frame);
				char key = cv::waitKey(10);

				
				if (key == 27) {
					is_ok_ = false;
					is_drawing_ = false;
					std::cout << " >> Action: " << utils::getBashColorText("Quiting the software ...", 'y', 'b') << std::endl;
				} else if (key == '0') {
					std::cout << " >> Action: " << utils::getBashColorText("go to the first image", 'g', 'b') << std::endl;
					index = 0;
					is_drawing_ = false;
				} else if (key == '1') {
					std::cout << " >> Action: " << utils::getBashColorText("go back to previous image", 'y', 'b') << std::endl;			
					index = (index + 1) % int(image_list_.size());
					is_drawing_ = false;
				} else if (key == '2') {
					std::cout << " >> Action: " << utils::getBashColorText("proceed to next image", 'g', 'b') << std::endl;
					index = (int(image_list_.size()) + (index - 1)) % int(image_list_.size());
					is_drawing_ = false;
				}	else {				
					switch (key) {
						case 'a': {
							current_drawer_.addRegion(randomId());
							break;
						}
						case 'd': {
							current_drawer_.deleteLastRegion();
							break;
						}
						case 'e': {
							std::string id, name;
							std::cout << "Enter a region id: ";
							std::cin >> id;
							std::cout << "Enter a new name (spacebar is not allowed !!!) : ";
							std::cin >> name;
							current_drawer_.editRegionById(id, name);
							break;
						}
					}
				}
			}
			
			std::map<std::string, MyPolygonDrawer>::iterator it2 = drawer_list_.find(item.name);
			if (it2 == drawer_list_.end()) {
				if (current_drawer_.getPolygons().size() > 0) {
					drawer_list_.insert(std::pair<std::string, MyPolygonDrawer>(item.name, current_drawer_));
					std::cout << " Added a new drawer: " << utils::getBashColorText(item.name, 'g', 'b') << std::endl;
					for (it2 = drawer_list_.begin(); it2 != drawer_list_.end(); it2++) {
						std::cout << " |-- " << it2->first << ", Polygons: " << it2->second.getPolygons().size() << std::endl;
					}
				}
			} else {
				it2->second = current_drawer_;
				std::cout << " .. Updated drawer: " << utils::getBashColorText(item.name, 'g', 'b') << std::endl;
			}
		}
		
		this->savePolygons();
	}
	
	void savePolygons() {
		if (drawer_list_.size() == 0) { return; }
		
		std::cout << "\nAvailable of " << utils::getBashColorText(cv::format("%d drawer-sets", int(drawer_list_.size())), 'g', 'b') << std::endl;
		std::map<std::string, MyPolygonDrawer>::iterator it;
		std::stringstream ss;
		ss << "polygons:" << std::endl;
		for (it = drawer_list_.begin(); it != drawer_list_.end(); it++) {
			ss << " - { name: " << it->first << ", " << it->second.getTextInfo() << "}" << std::endl;
		}
		std::cout << utils::getBashColorText(ss.str(), 'y', '0') << std::endl;
		
		std::ofstream writer;
		writer.open(polygon_data_filename_);
		writer << "appname: " << appname_ << std::endl;
		writer << "\ndatetime: " << utils::getLocaltime(0) << std::endl;
		writer << "\n" << ss.str();
		writer.close();
		
		std::cout << utils::getBashColorText("[Ok] Saved polygon data successfully: " + polygon_data_filename_, 'g', 'b') << std::endl;
	}
	
private:
	
	std::string randomId() {
		int idx = int(rand() % 100);
		int idy = int(rand() % 100);
		std::string idx_str = std::to_string(idx);
		std::string idy_str = std::to_string(idy);
		if (idx_str.size() == 1) { idx_str = "0" + idx_str; }
		if (idy_str.size() == 1) { idy_str = "0" + idy_str; }	
		return idx_str + idy_str;
	}
	
	void drawImageHeader(cv::Mat &image, std::string name) {
		std::vector<std::string> texts;
		texts.push_back(utils::getLocaltime(1));
		texts.push_back(cv::format("File: %s", name.c_str()));
		texts.push_back(cv::format("Size: %d x %d", image.cols, image.rows));
		
		int fontface = cv::FONT_HERSHEY_SIMPLEX;
		double fontscale = 0.5;
		int thickness = 1;
		
		for (int i=0; i<texts.size(); i++) {
			cv::Point pt(10, 15 + 30 * (texts.size() - i));
			cv::putText(image, texts[i], pt, fontface, fontscale, cv::Scalar(255, 255, 255), thickness);
		}
	}
	
	bool setImageList(std::string dir) {
		boost::filesystem::path path(dir);
		boost::filesystem::recursive_directory_iterator it_end;
		for (boost::filesystem::recursive_directory_iterator it(path); it != it_end; it++) {
			const boost::filesystem::path full_path = (*it);
			std::string filename = full_path.string();
			
			std::size_t found = filename.find(dir);
			if (found != std::string::npos) {
				std::string image_name = filename.substr(int(found) + dir.size() + 1, filename.size() - dir.size() - 1);
				LabelImageInfo info;
				info.name = image_name;
				info.filename = filename;
				info.image = cv::imread(filename, cv::IMREAD_COLOR);
				image_list_.push_back(info);
			}
		}
		
		return (int)image_list_.size() > 0;
	}
	
	MyPolygonDrawer current_drawer_;
	bool is_ok_;
	
	std::map<std::string, MyPolygonDrawer> drawer_list_;
	std::vector<LabelImageInfo> image_list_;
	std::string appname_;
	std::string results_dir_;
	std::string polygon_data_filename_;
};

int main(int argc, char **argv) {
	
	std::cout << "Reading config from " << utils::getBashColorText(CONFIG_FILE, 'l', 'b') << std::endl;
	std::ifstream reader(CONFIG_FILE);
	if (!reader.is_open()) {
		std::cout << utils::getBashColorText("Failed to read from the config file", 'r', 'b') << std::endl;
		return -1;
	}
	reader.close();
	
	YAML::Node node = YAML::LoadFile(CONFIG_FILE);
	std::string source_image_dir = node["source_image_dir"].as<std::string>();
	std::string results_dir = node["results_dir"].as<std::string>();
	
	std::cout << " -- Source image : " << utils::getBashColorText(source_image_dir, 'l', 'b') << std::endl;
	std::cout << " -- Results      : " << utils::getBashColorText(results_dir, 'l', 'b') << std::endl;
	
	checkResultDir(results_dir);
	
	ImageEditor editor(source_image_dir, results_dir, argv[0]);
	editor.run();
	
	return 0;
}
