#pragma once
 
struct Data {
	std::string value;
	std::string type;
	std::string rangeMin;
	std::string rangeMax;
	std::string defaultValue;
	std::string localName;
	std::string localDescription;
	std::string localDescriptionAdjusted;
	bool localNameWasFound;
	bool localDescriptionWasFound;
};
std::map<std::string, Data> configs;