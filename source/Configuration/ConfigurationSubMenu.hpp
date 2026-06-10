#pragma once

class ConfigurationSubMenu : public tsl::Gui {
private:
	std::string m_type;
	std::string showType;
	std::string m_name;
	std::string footerBackup;
	static constexpr size_t descriptionSize = 18;

	std::string wrapText(const std::string& text, u32 maxWidth, u32 fontsize) {
		std::string wrappedText;
		std::string currentLine;
		std::string currentWord;
		auto renderer = tsl::gfx::Renderer::getRenderer();

		// Helper lambda to measure string width invisibly
		auto measureWidth = [&](const std::string& str) {
			// Passing 0, 0 for x/y since we only care about the returned width
			
			const auto [width, height] = renderer.drawString(str.c_str(), false, 0, fontsize, fontsize, renderer.a(0x0000));
			return width;
		};

		for (char c : text) {
			if (c == ' ') {
				// Check if adding the current word exceeds our max width
				if (measureWidth(currentLine + currentWord) > maxWidth && !currentLine.empty()) {
					// Drop the trailing space from the previous line before breaking
					if (!currentLine.empty() && currentLine.back() == ' ') {
						currentLine.pop_back();
					}
					wrappedText += currentLine + "\n";
					currentLine = currentWord + " ";
				} else {
					currentLine += currentWord + " ";
				}
				currentWord.clear();
			} else if (c == '\n') {
				// Respect any explicit newlines already in the string
				wrappedText += currentLine + currentWord + "\n";
				currentLine.clear();
				currentWord.clear();
			} else {
				currentWord += c;
			}
		}

		// Append whatever is left over at the end of the loop
		if (!currentWord.empty()) {
			if (measureWidth(currentLine + currentWord) > maxWidth && !currentLine.empty()) {
				if (!currentLine.empty() && currentLine.back() == ' ') {
					currentLine.pop_back();
				}
				wrappedText += currentLine + "\n" + currentWord;
			} else {
				wrappedText += currentLine + currentWord;
			}
		} else if (!currentLine.empty()) {
			wrappedText += currentLine;
		}

		return wrappedText;
	}

	size_t GetHeight(const std::string& text, size_t fontsize) {
		auto renderer = tsl::gfx::Renderer::getRenderer();
		const auto [width, height] = renderer.drawString(text.c_str(), false, 0, fontsize, fontsize, renderer.a(0x0000));
		return height;
	}

public:
	ConfigurationSubMenu(std::string type, std::string name) {
		footerBackup = defaultButtonView;
		defaultButtonView = locale["Footer"];
		m_type = type;
		if (m_type.compare("bool") == 0) {
			showType = "\uE142\uE14B\uE14C";
		}
		else if (m_type.compare("int") == 0) {
			showType = "1\uE08C60";
		}
		else if (m_type.compare("color") == 0) {
			showType = "\uE135";
		}
		else if (m_type.compare("list") == 0) {
			showType = "\uE047\uE048";
		}
		m_name = name + "\n" + showType;
	}

	~ConfigurationSubMenu() {
		defaultButtonView = footerBackup;
	}

	virtual tsl::elm::Element* createUI() override {
		rootFrame = new tsl::elm::OverlayFrame(APP_TITLE, m_name);
		auto list = new tsl::elm::List();

		for (const auto& [key, data] : configs) {
			std::string descriptionAdjusted = data.localDescriptionAdjusted;
			
			if (data.localDescription.length() > 0 && descriptionAdjusted.length() == 0) {
				configs[key].localDescriptionAdjusted = wrapText(resolveHexEscapes(data.localDescription), 360, descriptionSize);
				descriptionAdjusted = configs[key].localDescriptionAdjusted;
			}

			auto addDescription = [&]() {
				if (descriptionAdjusted.length() > 0) {
					auto height = GetHeight(descriptionAdjusted, descriptionSize) + descriptionSize;
					auto Status = new tsl::elm::CustomDrawer([this, descriptionAdjusted, height](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
						renderer->drawString(descriptionAdjusted.c_str(), false, x+5, y+descriptionSize+5, descriptionSize, 0xFCCC);
						renderer->drawRect(x, y+5, 2, height-descriptionSize+5, 0xFCCC);
						renderer->drawRect(x, y+5, 5, 2, 0xFCCC);
					});
					list->addItem(Status, height);
				}
			};

			if ((data.type.compare(m_type) == 0) || (data.type.compare("font") == 0 && m_type.compare("int") == 0)) {
				if (m_type.compare("int") == 0) {
					int64_t temp;
					if (isNumeric(data.rangeMin, &temp) == true && isNumeric(data.rangeMax, &temp) == true) {
						auto Item = new tsl::elm::ListItem(data.localName.length() > 0 ? data.localName : key, data.value, false);
						Item->setClickListener([this, key, data, Item](uint64_t keys) {
							if (keys & KEY_A) {
								tsl::hlp::requestForeground(true);
								tsl::changeTo<EditConfigInt>(key, configs.at(key).value, data.rangeMin, data.rangeMax, data.defaultValue, Item, data.localName, data.type);
								return true;
							}
							return false;
						});
						addDescription();
						list->addItem(Item);
					}
					else {
						auto Item = new tsl::elm::ListItem(key, data.type.c_str(), true);
						list->addItem(Item);
					}
				}
				else if (m_type.compare("list") == 0) {
					auto Item = new tsl::elm::ListItem(data.localName.length() > 0 ? data.localName : key);
					Item->setClickListener([this, key, data, Item](uint64_t keys) {
						if (keys & KEY_A) {
							std::string value = configs[key].value;
							std::string defaultValue = data.defaultValue;
							if (value.starts_with("LIST")) value = listToFlatList(value);
							if (defaultValue.starts_with("LIST")) defaultValue = listToFlatList(defaultValue);
							tsl::hlp::requestForeground(true);
							tsl::changeTo<EditConfigOrdering>(key, value, defaultValue, Item, data.localName);
							return true;
						}
						return false;
					});
					addDescription();
					list->addItem(Item);
				}
				else if (data.type.compare("bool") == 0) {
					bool isTrue = data.value.compare("true") == 0;
					auto Item = new tsl::elm::ToggleListItem(data.localName.length() > 0 ? data.localName : key, isTrue);
					Item->setClickListener([this, key, Item](uint64_t keys) {
						if (keys & KEY_A) {
							configs.at(key).value = Item->getState() ? "true" : "false";
							return true;
						}
						return false;
					});
					addDescription();
					list->addItem(Item);
				}
				else if (data.type.compare("color") == 0) {
					bool isValid = false;
					u16 color;
					std::string error;
					if (data.value.length() == 13) {
						std::string hexColor = data.value.substr(6);
						hexColor = hexColor.substr(0, 6);
						isValid = isValid4444HexColor(hexColor);
						if (isValid) {
							std::string r = hexColor.substr(2, 1);
							std::string g = hexColor.substr(3, 1);
							std::string b = hexColor.substr(4, 1);
							std::string a = hexColor.substr(5, 1);
							color = tsl::gfx::Color((u8)std::stoi(r, nullptr, 16), (u8)std::stoi(g, nullptr, 16), (u8)std::stoi(b, nullptr, 16), (u8)std::stoi(a, nullptr, 16)).rgba;
						}
						else error = "invalid color";
					}
					if (isValid) {
						auto Item = new tsl::elm::ColorListItem(data.localName.length() > 0 ? data.localName : key, color);
						Item->setClickListener([this, key, Item, data](uint64_t keys) {
							if (keys & KEY_A) {
								tsl::hlp::requestForeground(true);
								tsl::changeTo<EditConfigColor>(key, Item, data.localName);
								return true;
							}
							return false;
						});
						addDescription();
						list->addItem(Item);
					}
					else {
						auto Item = new tsl::elm::ListItem(error, data.type.c_str(), true);
						list->addItem(Item);
					}
				}
				else {
					auto Item = new tsl::elm::ListItem(key, "\uE150");
					list->addItem(Item);
				}
			}
		}
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
