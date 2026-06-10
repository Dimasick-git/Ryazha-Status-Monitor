#pragma once

class ConfigurationServiceCheck : public tsl::Gui {
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
	ConfigurationServiceCheck(std::string name) {
		m_name = name;
		footerBackup = defaultButtonView;
		defaultButtonView = locale["Footer"];
	}

	~ConfigurationServiceCheck() {
		defaultButtonView = footerBackup;
	}

	virtual tsl::elm::Element* createUI() override {
		rootFrame = new tsl::elm::OverlayFrame(APP_TITLE, m_name);
		auto list = new tsl::elm::List();

		for (auto& svc : smseGetAllServices()) {
			std::string descriptionAdjusted = "";
			bool errorFound = false;
			for (size_t i = 0; i < svc.initErrors.size(); i++) {
				if (svc.initErrors[i].ok() == false) {
					if (svc.initErrors[i].detail.length() > 0) descriptionAdjusted = wrapText(resolveHexEscapes(svc.initErrors[0].detail), 360, descriptionSize);
					else descriptionAdjusted = "Init error with no desc";
					errorFound = true;
				}
            }				
			if (errorFound == false && svc.execErrors -> size() > 0) {
				for (size_t i = 0; i < svc.execErrors -> size(); i++) {
					for (size_t x = 0; x < svc.execErrors[i].size(); x++) {
						if (svc.execErrors[i][x].ok() == false) {
							if (svc.execErrors[i][x].detail.size() > 0) descriptionAdjusted = wrapText(resolveHexEscapes(svc.execErrors[i][x].detail), 360, descriptionSize);
							else descriptionAdjusted = "Exec error with no desc";
							errorFound = true;
							break;
						}
					}
				}
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


            auto Item = new tsl::elm::ListItem(svc.name, svc.connected ? "\uE14B" : "\uE14C", !svc.connected);
            if (errorFound) addDescription();
            list->addItem(Item);
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
