#pragma once
#include <unordered_set>

class EditConfigOrdering : public tsl::Gui {
private:
	std::vector<std::pair<std::string, bool>> toRender;
	std::vector<std::string> defaultList;
	std::string m_key;
	tsl::elm::ListItem* m_item;
	std::string m_localName;
	std::string footerBackup;
public:
	EditConfigOrdering(std::string key, std::string value, std::string defaultValue, tsl::elm::ListItem* item, std::string localName) {
		footerBackup = defaultButtonView;
		defaultButtonView = locale["FooterMoveUpDown"];
		if (localName.length() > 0) m_localName = localName;
		else m_localName = key;
		tsl::hlp::requestForeground(true);
		m_item = item;
		m_key = key;
		if (value.starts_with("LIST")) {
			value = listToFlatList(value);
		}
		if (defaultValue.starts_with("LIST")) {
			defaultValue = listToFlatList(defaultValue);
		}		
		auto parts = value | std::views::split('+');
		auto defaultParts = defaultValue | std::views::split('+');
		for (auto&& part : defaultParts) {
			std::string partStr(part.begin(), part.end());
        	defaultList.emplace_back(partStr);
   		}
		std::unordered_set<std::string> activeParts;
		for (auto&& part : parts) {
			activeParts.emplace(part.begin(), part.end());
		}

		size_t lastIdx = 0;

		for (auto&& part : parts) {
			std::string partStr(part.begin(), part.end());
			if (std::find(defaultList.begin(), defaultList.end(), partStr) == defaultList.end()) continue;

			// Find where this part sits in defaultList
			size_t idx = std::find(defaultList.begin(), defaultList.end(), partStr) - defaultList.begin();

			// Flush any false items that come before this part in defaultList
			for (size_t j = lastIdx; j < idx; j++) {
				if (activeParts.count(defaultList[j]) == 0)
					toRender.emplace_back(defaultList[j], false);
			}
			lastIdx = std::max(lastIdx, idx + 1);

			toRender.emplace_back(partStr, true);
		}

		// Flush any remaining false items at the end
		for (size_t j = lastIdx; j < defaultList.size(); j++) {
			if (activeParts.count(defaultList[j]) == 0)
				toRender.emplace_back(defaultList[j], false);
		}
	}

	~EditConfigOrdering() {
		defaultButtonView = footerBackup;
	}

	virtual tsl::elm::Element* createUI() override {
		rootFrame = new tsl::elm::OverlayFrame(APP_TITLE, m_localName);
		auto list = new tsl::elm::List();
		size_t listSize = toRender.size();
		for (size_t i = 0; i < listSize; i++) {
			const auto& [key, value] = toRender[i];
			auto Item = new tsl::elm::ToggleListItem(key, value);
			Item->setClickListener([this, key, i, Item, listSize, list](uint64_t keys) {
				if (keys & KEY_A) {
					toRender[i] = std::make_pair(key, Item->getState());
					return true;
				}
				if (keys & KEY_ZL) {
					size_t cdx = list->getCurrentFocus();
					if (cdx > 0) std::swap(toRender[cdx], toRender[cdx-1]);
					list->moveUp();
					return true;
				}
				else if (keys & KEY_ZR) {
					size_t cdx = list->getCurrentFocus();
					if (cdx+1 < listSize) std::swap(toRender[cdx], toRender[cdx+1]);
					list->moveDown();
					return true;
				}
				return false;
			});
			list->addItem(Item, 40);
		}
		rootFrame->setContent(list);
		return rootFrame;
	}

	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		if (keysDown & KEY_B) {
			std::string out = "";
			for (const auto& [key, value] : toRender) {
				if (value == true) out += key + "+";
			}
			if (out.empty() == false) out.pop_back();
			configs[m_key].value = flatListToList(out);
			tsl::hlp::requestForeground(true);
			tsl::goBack();
			return true;
		}
		return false;
	}
};
