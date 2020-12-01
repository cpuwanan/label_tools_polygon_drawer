#include "utils.h"

#include <stdio.h>
#include <string>
#include <sstream>
#include <unistd.h>

#include <termios.h>    //termios, TCSANOW, ECHO, ICANON

/* According to POSIX.1-2001 */
#include <sys/select.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>     //STDIN_FILENO

#include <ctime>
#include <boost/filesystem.hpp>

namespace utils {
const std::string COLOR_OFF = "\033[0m";	
const std::string SUCCESS = "\033[0;32m";
const std::string DANGER = "\033[0;31m";	
const std::string WARNING = "\033[0;33m";
const std::string SECONDARY = "\033[0;35m";
const std::string CYAN = "\033[0;36m";
const std::string SUCCESS2 = "\033[1;32m";
const std::string DANGER2 = "\033[1;31m";
const std::string WARNING2 = "\033[1;33m";
const std::string SECONDARY2 = "\033[1;35m";	
const std::string CYAN2 = "\033[1;36m";	
}

std::string utils::colorText(int state, std::string text)
{
	switch (state) {
		case TextType::INFO: { text = utils::CYAN + text + utils::COLOR_OFF; break; }
		case TextType::INFO_B: { text = utils::CYAN2 + text + utils::COLOR_OFF; break; }
		case TextType::SUCCESS: { text = utils::SUCCESS + text + utils::COLOR_OFF; break; }
		case TextType::SUCCESS_B: { text = utils::SUCCESS2 + text + utils::COLOR_OFF; break; }
		case TextType::WARNING: { text = utils::WARNING + text + utils::COLOR_OFF; break; }
		case TextType::WARNING_B: { text = utils::WARNING2 + text + utils::COLOR_OFF; break; }
		case TextType::DANGER: { text = utils::DANGER + text + utils::COLOR_OFF; break; }
		case TextType::DANGER_B: { text = utils::DANGER2 + text + utils::COLOR_OFF; break; }
	}
	return text;
}

void utils::findFiles(const std::string path, std::string filter, std::vector<std::string>& outputs)
{
	outputs.clear();
	for (auto &p : boost::filesystem::recursive_directory_iterator(path)) {
		std::stringstream value;
		value << p.path().c_str();
		if (value.str().find(filter) != std::string::npos) {
			outputs.push_back(value.str());
		}
	}
}

std::string utils::getStrId(int id, int N, char prefix)
{
	std::string text = std::to_string(id);
	while (text.size() < N) {
		text = std::string(1, prefix) + text;
	}
	return text;
}

std::string utils::getBashColorText(std::string text, char color, char style, bool background)
{
	std::string output("");
	switch (style) {
		case 'b': output += "\033[1;"; break;   // bold
		case 'u': output += "\033[4;"; break;   // underline
		case 'f': output += "\033[5;"; break;   // flash, blink
		case 'n': output += "\033[7;"; break;   // negative
		default:  output += "\033[0;";          // reset
	}
        
	output += (background ? "4" : "3");
			
	switch (color) {
		case 'b': output += "0m"; break;    // black
		case 'r': output += "1m"; break;    // red
		case 'g': output += "2m"; break;    // green
		case 'y': output += "3m"; break;    // brown, orange
		case 'l': output += "4m"; break;    // blue
		case 'm': output += "5m"; break;    // magenta
		case 'c': output += "6m"; break;    // cyan
		default:  output += "7m";           // light gray
	}
	output += text + "\033[0m";
	return output;
}

char utils::nonBlockingKeyboardEvent()
{
	fd_set set;
	struct timeval timeout;
	int rv;
	char buff = 0;
	int len = 1;
	int filedesc = 0;
	FD_ZERO(&set);
	FD_SET(filedesc, &set);
        
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;

	rv = select(filedesc + 1, &set, NULL, NULL, &timeout);

	struct termios old = {0};
	if (tcgetattr(filedesc, &old) < 0)
		std::cout << "Error: tcsetattr()" << std::endl;
	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	if (tcsetattr(filedesc, TCSANOW, &old) < 0)
		std::cout << "Error: tcsetattr ICANON" << std::endl;

	if(rv == -1)
		std::cout << "Error: select" << std::endl;
	else if(rv == 0) {
		// no key pressed
	}
	else
		read(filedesc, &buff, len );
	
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	if (tcsetattr(filedesc, TCSADRAIN, &old) < 0)
		std::cout << "Error: tcsetattr ~ICANON" << std::endl;
        
	return (buff);
}

std::string utils::getLocaltime(int mode) {
	time_t now = time(0);
	tm *mytime = localtime(&now);
		
	std::string day_sep("-");
	std::string time_sep("-");
	std::string gap("_");
	
	if (mode == 1) {
		day_sep = "/";
		time_sep = ":";
		gap = " ";
	}
	
	std::stringstream ss;
	ss << "Y" << (mytime->tm_year + 1900) << day_sep << (mytime->tm_mon + 1) << day_sep << (mytime->tm_mday); 
	ss << gap;
	ss << "T" << (mytime->tm_hour + 1) << time_sep << (mytime->tm_min + 1) << time_sep << (mytime->tm_sec + 1);
	return ss.str();
}
