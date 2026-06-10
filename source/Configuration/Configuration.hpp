#pragma once
#include "EditConfigOrdering.hpp"
#include "EditConfigColor.hpp"
#include "ConfigurationSubMenu.hpp"

class Configuration : public tsl::Gui {
private:
	char* buffer = nullptr;
	size_t buffer_size = 0;
	bool isBool = false;
	bool isInt = false;
	bool isColor = false;
	bool isError = false;
	bool isOrdering = false;
	bool skipSavingConfig = false;
	void FindConfigs(const char* data, size_t size) {
		size_t lineStart = 0;
		for (size_t i = 0; i < size; ++i) {
			if (data[i] != '\n') continue;

			// Slice the current line [lineStart, i), peeling off a trailing \r.
			size_t end = i;
			while (end > lineStart && data[end - 1] == '\r') --end;
			std::string rawLine(data + lineStart, end - lineStart);
			lineStart = i + 1;

			// 2. Standard configuration line processing
			std::string line = StripLineComment(rawLine);
			line = trim(line);
			if (line.empty()) continue;

			if (line == "Start:" || line == "Start: ") break;

			size_t sep = std::string::npos;
			{
				int depth = 0; bool inStr = false;
				for (size_t j = 0; j < line.size(); ++j) {
					char c = line[j];
					if (inStr) {
						if (c == '\\' && j + 1 < line.size()) { ++j; continue; }
						if (c == '"') inStr = false;
						continue;
					}
					if (c == '"') { inStr = true; continue; }
					if (c == '{') ++depth;
					else if (c == '}') --depth;
					else if (depth == 0 && c == '=') { sep = j; break; }
				}
			}
			if (sep == std::string::npos) continue;

			std::string key  = trim(line.substr(0, sep));
			std::string rest = trim(line.substr(sep + 1));

			if (key.starts_with("User_") == false) continue;
			auto sub_key = key.substr(5);

			if (configs[sub_key].value.empty()) {
				configs[sub_key].localNameWasFound = false;
				configs[sub_key].localDescriptionWasFound = false;
				configs[sub_key].value = rest;
				
				// Deduce type from value prefix if explicit _Range was not provided
				if (configs[sub_key].type.empty()) {
					if (rest.starts_with("COLOR")) {
						configs[sub_key].type = "color";
					} else if (rest.starts_with("LIST")) {
						configs[sub_key].type = "list";
					}
					else if (rest.compare("true") == 0 || rest.compare("false") == 0) {
						configs[sub_key].type = "bool";
					}
				}
			}
		}

		lineStart = 0;
		for (size_t i = 0; i < size; ++i) {
			if (data[i] != '\n') continue;

			// Slice the current line [lineStart, i), peeling off a trailing \r.
			size_t end = i;
			while (end > lineStart && data[end - 1] == '\r') --end;
			std::string rawLine(data + lineStart, end - lineStart);
			if (rawLine.starts_with("Start:")) break;
			lineStart = i + 1;
			std::string trimmedRaw = trim(rawLine);

			// 1. Intercept metadata lines
			if (trimmedRaw.starts_with(";;User_")) {
				size_t sep = std::string::npos;
				int depth = 0; bool inStr = false;
				
				for (size_t j = 0; j < trimmedRaw.size(); ++j) {
					char c = trimmedRaw[j];
					if (inStr) {
						if (c == '\\' && j + 1 < trimmedRaw.size()) { ++j; continue; }
						if (c == '"') inStr = false;
						continue;
					}
					if (c == '"') { inStr = true; continue; }
					if (c == '{') ++depth;
					else if (c == '}') --depth;
					else if (depth == 0 && c == '=') { sep = j; break; }
				}

				if (sep != std::string::npos) {
					std::string metaKey = trim(trimmedRaw.substr(2, sep - 2)); 
					std::string rest = trim(trimmedRaw.substr(sep + 1));

					std::string localNameSearch = "_Name_" + overrideLanguage;
					std::string localDescriptionSearch = "_Description_" + overrideLanguage;

					if (metaKey.ends_with("_Range")) {
						std::string sub_key = metaKey.substr(5, metaKey.size() - strlen("User_") - strlen("_Range"));
						
						if (!rest.empty() && rest.front() == '{') rest = rest.substr(1);
						if (!rest.empty() && rest.back() == '}') rest.pop_back();
						
						size_t c1 = rest.find(',');
						if (c1 != std::string::npos) {
							size_t c2 = rest.find(',', c1 + 1);
							if (c2 != std::string::npos) {
								if (configs.find(sub_key) != configs.end()) {
									configs[sub_key].type = trim(rest.substr(0, c1));
									configs[sub_key].rangeMin = trim(rest.substr(c1 + 1, c2 - c1 - 1));
									configs[sub_key].rangeMax = trim(rest.substr(c2 + 1));
								}
							}
						}
					} 
					else if (metaKey.ends_with("_DefaultValue")) {
						std::string sub_key = metaKey.substr(5, metaKey.size() - strlen("User_") - strlen("_DefaultValue"));
						if (configs.find(sub_key) != configs.end())
							configs[sub_key].defaultValue = rest;
					}
					else if (metaKey.ends_with("_Name")) {
						std::string sub_key = metaKey.substr(5, metaKey.size() - strlen("User_") - strlen("_Name"));
						if (configs.find(sub_key) != configs.end()) {
							if (configs[sub_key].localNameWasFound == false) configs[sub_key].localName = rest;
						}
					}
					else if (metaKey.ends_with(localNameSearch)) {
						std::string sub_key = metaKey.substr(5, metaKey.size() - strlen("User_") - localNameSearch.length());
						if (configs.find(sub_key) != configs.end()) {
							configs[sub_key].localName = rest;
							configs[sub_key].localNameWasFound = true;
						}
					}
					else if (metaKey.ends_with("_Description")) {
						std::string sub_key = metaKey.substr(5, metaKey.size() - strlen("User_") - strlen("_Description"));
						if (configs.find(sub_key) != configs.end()) {
							if (configs[sub_key].localDescriptionWasFound == false) {
								configs[sub_key].localDescription = rest;
							}
						}
					}
					else if (metaKey.ends_with(localDescriptionSearch)) {
						std::string sub_key = metaKey.substr(5, metaKey.size() - strlen("User_") - localDescriptionSearch.length());
						if (configs.find(sub_key) != configs.end()) {
							configs[sub_key].localDescription = rest;
							configs[sub_key].localDescriptionWasFound = true;
						}
					}
				}
				continue; 
			}
		}

		// 3. Fallback check: If type is still completely blank, default to "bool"
		for (auto& [key, data] : configs) {
			if (data.type.empty()) {
				data.type = "error";
			}
		}

		return;
	}

	void setDataToIniFile(const std::string& fileToEdit, const std::string& desiredSection) {
		FILE* configFile = fopen(fileToEdit.c_str(), "r");
		if (!configFile) {
			configFile = fopen(fileToEdit.c_str(), "w");
			if (!configFile) return;
			fprintf(configFile, "[%s]\n", desiredSection.c_str());
			for (const auto& [key, data] : configs)
				fprintf(configFile, "User_%s = %s\n", key.c_str(), trim(data.value).c_str());
			fclose(configFile);
			return;
		}

		std::string updatedContent;
		std::string currentSection;
		std::set<std::string> keysFound;
		char line[131072];

		bool sectionFound = false;
		bool addNewLine   = false;

		// Appends any keys from configs that haven't been written yet.
		auto appendMissingKeys = [&]() {
			for (const auto& [key, data] : configs) {
				if (keysFound.count(key)) continue;
				if (!updatedContent.empty() && updatedContent.substr(updatedContent.length() - 2) == "\n\n") {
					updatedContent = updatedContent.substr(0, updatedContent.length() - 1);
					addNewLine = true;
				}
				updatedContent += "User_" + key + " = " + trim(data.value) + "\n";
				keysFound.insert(key);
				if (addNewLine) { updatedContent += "\n"; addNewLine = false; }
			}
		};

		while (fgets(line, sizeof(line), configFile)) {
			std::string trimmedLine = trim(line);

			// Entering a new section header.
			if (trimmedLine.size() >= 2 && trimmedLine[0] == '[' && trimmedLine.back() == ']') {
				// Leaving the desired section — flush any keys not yet written.
				if (sectionFound && trim(currentSection) == trim(desiredSection))
					appendMissingKeys();

				currentSection = removeQuotes(trimmedLine.substr(1, trimmedLine.length() - 2));
			}

			// Inside the desired section — check if this line holds one of our keys.
			if (trim(currentSection) == trim(desiredSection)) {
				sectionFound = true;
				std::string::size_type delimiterPos = trimmedLine.find('=');

				if (delimiterPos != std::string::npos) {
					std::string lineKey = trim(trimmedLine.substr(0, delimiterPos));

					// Strip prefix for map lookup — file stores "User_key", configs is keyed by "key"
					std::string lookupKey = (lineKey.substr(0, 5) == "User_") ? lineKey.substr(5) : lineKey;
					auto it = configs.find(lookupKey);

					if (it != configs.end()) {
						keysFound.insert(lookupKey); // track by unprefixed key, same as configs
						if (!updatedContent.empty() && updatedContent.substr(updatedContent.length() - 2) == "\n\n") {
							updatedContent = updatedContent.substr(0, updatedContent.length() - 1);
							addNewLine = true;
						}
						updatedContent += "User_" + lookupKey + " = " + trim(it->second.value) + "\n";
						if (addNewLine) { updatedContent += "\n"; addNewLine = false; }
						continue;
					}
				}
			}

			updatedContent += line;
		}

		fclose(configFile);

		// EOF while still inside the desired section.
		if (sectionFound)
			appendMissingKeys();

		// Section was never encountered — append it at the end.
		if (!sectionFound) {
			updatedContent += "\n[" + desiredSection + "]\n";
			for (const auto& [key, data] : configs)
				updatedContent += "User_" + key + " = " + trim(data.value) + "\n";
		}

		configFile = fopen(fileToEdit.c_str(), "w");
		if (!configFile) return;
		fprintf(configFile, "%s", updatedContent.c_str());
		fclose(configFile);
	}
public:
	std::string filepath;
	std::string m_name;
	std::string m_show;
	std::string section_name;
	std::vector<std::string> keys_to_convert;
	std::string footerBackup;
	Configuration(std::string path, std::string name) {
		footerBackup = defaultButtonView;
		defaultButtonView = locale["Footer"];
		tsl::hlp::requestForeground(true);
		filepath = path;
		FILE* file = fopen(filepath.c_str(), "rb");
		if (file) {
			fseek(file, 0, 2);
			buffer_size = ftell(file);
			fseek(file, 0, 0);
			buffer = (char*)malloc(buffer_size);
			fread(buffer, 1, buffer_size, file);
			fclose(file);
			FindConfigs(buffer, buffer_size);
			free(buffer);
		}
		for (const auto& [key, data] : configs) {
			if (data.type.compare("bool") == 0) {
				isBool = true;
			}
			else if (data.type.compare("int") == 0 || data.type.compare("font") == 0) {
				isInt = true;
			}
			else if (data.type.compare("color") == 0) {
				isColor = true;
			}
			else if (data.type.compare("list") == 0) {
				isOrdering = true;
			}
			else isError = true;
		}
		m_name = name;
		m_show = std::string("\uE130 ") + m_name;
		section_name = filepath.substr(strlen("sdmc:/config/status-monitor-deux/modes/"));
		auto it = config.find(section_name);
		if (it == config.end()) {
			config[section_name] = std::map<std::string, std::string>();
			return; //nie znaleziono sekcji w pliku ini
		}
		auto settings = it->second; //to co znaleziono w pliku ini
		for (const auto& [key, data] : configs) { //To co znaleziono w pliku SMD
			auto it2 = settings.find("User_" + key);
			if (it2 == settings.end()) continue;
			std::string temp = it2->second;
			int64_t value;
			if (data.value.compare("true") == 0 || data.value.compare("false") == 0) {
				if (temp.compare("true") != 0 && temp.compare("false") != 0) continue;
			}
			else if (data.value.starts_with("COLOR{") && data.value.length() == 13 && data.value.substr(6, 2).compare("0x") == 0 && data.value.substr(12).compare("}") == 0) {
				if (temp.starts_with("COLOR{") == false || temp.length() != 13 || temp.substr(6, 2).compare("0x") || temp.substr(12).compare("}")) continue;
			}
			else if (isNumeric(data.value, &value) == true) {
				if (isNumeric(temp, &value) == false) continue;
			}
			else if (data.value.starts_with("LIST{str, {\"") && data.value.ends_with("\"}}")) {
				std::string temp2 = temp;
				if (temp.starts_with("LIST{str, {\"") == false) {
					temp2 = flatListToList(temp);
				}
				temp = temp2;
				keys_to_convert.emplace_back(key);

			}
			else continue;
			configs[key].value = temp;
		}
	}

	~Configuration() {
		defaultButtonView = footerBackup;
		if (skipSavingConfig == false) {
			if (keys_to_convert.empty()) for (const auto& [key, data] : configs) {
				if (configs[key].value.starts_with("LIST")) {
					keys_to_convert.emplace_back(key);
					std::string list = listToFlatList(configs[key].value);
					configs[key].value = list;
				}
			}
			else if (configs.size() > 0) for (size_t i = 0; i < keys_to_convert.size(); i++) {
				auto it = configs.find(keys_to_convert[i]);
				if (it != configs.end()) {
					std::string list = listToFlatList(it->second.value);
					it->second.value = list;
				}
			}
			setDataToIniFile("sdmc:/config/status-monitor-deux/config.ini", section_name);

			auto settings = config.find(section_name);
			if (settings == config.end()) {
				configs.clear();
				return;
			}
			
			for (size_t i = 0; i < keys_to_convert.size(); i++) {
				auto it = configs.find(keys_to_convert[i]);
				if (it != configs.end()) {
					std::string list = flatListToList(it->second.value);
					it->second.value = list;
				}
			}
			for (const auto& [key, data] : configs) {
				std::string m_key = "User_" + key;
				settings->second[m_key] = configs[key].value;
			}
			configs.clear();
		}
	}

	virtual tsl::elm::Element* createUI() override {

		rootFrame = new tsl::elm::OverlayFrame(APP_TITLE, m_show);
		auto list = new tsl::elm::List();
		if (isBool == true) {
			auto Item = new tsl::elm::ListItem(locale["Bools"], "\uE142\uE14B\uE14C");
			Item->setClickListener([this](uint64_t keys) {
				if (keys & KEY_A) {
					tsl::hlp::requestForeground(true);
					tsl::changeTo<ConfigurationSubMenu>("bool", m_show);
					return true;
				}
				return false;
			});
			list->addItem(Item);
		}
		if (isInt == true) {
			auto Item = new tsl::elm::ListItem(locale["Ints"], "1\uE08C60");
			Item->setClickListener([this](uint64_t keys) {
				if (keys & KEY_A) {
					tsl::hlp::requestForeground(true);
					tsl::changeTo<ConfigurationSubMenu>("int", m_show);
					return true;
				}
				return false;
			});
			list->addItem(Item);
		}
		if (isColor == true) {
			auto Item = new tsl::elm::ColorListItem(locale["Colors"], 0xF808, true);
			Item->setClickListener([this](uint64_t keys) {
				if (keys & KEY_A) {
					tsl::hlp::requestForeground(true);
					tsl::changeTo<ConfigurationSubMenu>("color", m_show);
					return true;
				}
				return false;
			});
			list->addItem(Item);
		}
		if (isOrdering == true) {
			auto Item = new tsl::elm::ListItem(locale["List"], "\uE047\uE048");
			Item->setClickListener([this](uint64_t keys) {
				if (keys & KEY_A) {
					tsl::hlp::requestForeground(true);
					tsl::changeTo<ConfigurationSubMenu>("list", m_show);
					return true;
				}
				return false;
			});
			list->addItem(Item);
		}

		auto Item = new tsl::elm::ListItem(locale["reset_settings"], "\uE141");
		Item->setClickListener([this](uint64_t keys) {
			if (keys & KEY_A) {
				tsl::hlp::requestForeground(true);
				removeIniSection("sdmc:/config/status-monitor-deux/config.ini", section_name);
				config.extract(section_name);
				tsl::goBack();
				return true;
			}
			return false;
		});
		list->addItem(Item);		

		rootFrame->setContent(list);
		return rootFrame;
	}

	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		if (keysDown & KEY_B) {
			tsl::hlp::requestForeground(true);
			tsl::goBack();
			return true;
		}
		return false;
	}
};
