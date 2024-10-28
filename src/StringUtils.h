#pragma once

#include <string>
#include <vector>

class StringUtils {
public:
	static std::vector<std::string> split(const std::string& s, char separator) {
		std::vector<std::string> output;
		std::string::size_type prev_pos = 0, pos = 0;

		while((pos = s.find(separator, pos)) != std::string::npos) {
			std::string substring( s.substr(prev_pos, pos-prev_pos) );
			output.push_back(substring);
			prev_pos = ++pos;
		}

		output.push_back(s.substr(prev_pos, pos-prev_pos)); // Last word

		return output;
	}

};
