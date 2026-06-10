#pragma once

class EditConfigKeyCombo : public tsl::Gui {
private:
	std::string m_key;
	std::string m_localName;
	std::string m_value;
	std::string m_valueToShow;
	std::string m_footerValue;
	u64 keyMapping;
	bool m_mainCombo;
	bool changingCombo = false;
	std::string footerBackup;
	std::string footerTarget;
	u64 timer = 0;
	u64 maxTimer = 3 * systemtickfrequency;
	std::map<std::string, std::string>* m_configs;
	std::string* m_out;
public:
	EditConfigKeyCombo(bool isMainCombo, std::string key, std::string value, std::string localName, std::map<std::string, std::string>* configs, std::string* out) {
		m_out = out;
		m_configs = configs;
		m_mainCombo = isMainCombo;
		m_localName = localName;
		footerBackup = defaultButtonView;
		defaultButtonView = locale["FooterModify"];
		footerTarget = defaultButtonView;
		m_key = key;
		if (m_mainCombo == true) {
			if (value.length() > 0) m_value = value;
			else {
				m_value = keyCombo;
				if (ultrahandCombo == true) {
					m_footerValue = locale["UltrahandCombo"];
				}
				else if (teslaCombo == true) {
					m_footerValue = locale["TeslaMenuCombo"];
				}
			}
		}
		else {
			m_value = value;
		}
		m_valueToShow = m_value;
		formatButtonCombination(m_valueToShow);
	}

	~EditConfigKeyCombo() {
		defaultButtonView = footerBackup;
	}

	virtual tsl::elm::Element* createUI() override {
		rootFrame = new tsl::elm::OverlayFrame(APP_TITLE, m_localName);
		auto Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
			if (changingCombo == false) {
				const size_t fontsize2 = 40;
				const size_t m_height = (360+fontsize2) - (fontsize2 / 2);
				auto [width, height] = renderer->drawString(m_valueToShow.c_str(), false, 0, fontsize2, fontsize2, renderer->a(0x0000), true);
				renderer->drawString(m_valueToShow.c_str(), false, (tsl::cfg::FramebufferWidth - width) / 2, m_height, fontsize2, renderer->a(0xFFFF), true);
				if (m_mainCombo == true && (m_footerValue.length() > 0)) {
					const size_t fontsize1 = 30;
					auto [width2, height2] = renderer->drawString(m_footerValue.c_str(), false, 0, fontsize1, fontsize1, renderer->a(0x0000), true);
					if (m_footerValue.length() > 0) renderer->drawString(m_footerValue.c_str(), false, (tsl::cfg::FramebufferWidth - width2) / 2, m_height+fontsize2, fontsize1, renderer->a(0xFFFF), true);
				}
			}
			else {
				const size_t fontsize2 = 40;
				const size_t m_height = (360+fontsize2) - (fontsize2 / 2);
				auto [width, height] = renderer->drawString(m_valueToShow.c_str(), false, 0, fontsize2, fontsize2, renderer->a(0x0000), true);
				renderer->drawString(m_valueToShow.c_str(), false, (tsl::cfg::FramebufferWidth - width) / 2, m_height, fontsize2, renderer->a(0xFFFF), true);
				renderer->drawEmptyRect(20, m_height+fontsize2, 408, 40, 0xFFFF);
				size_t recWidth = (size_t)((float)timer / (float)maxTimer * 404);
				renderer->drawRect(22, m_height+fontsize2+2, recWidth, 37, 0xFFFF);
			}
        });
		rootFrame->setContent(Status);
		return rootFrame;
	}

	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		static u64 last_keysHeld = 0;
		static u64 last_tick = 0;
		if (changingCombo == false) {
			if (keysDown & KEY_X) {
				changingCombo = true;
				m_footerValue = "";
				last_keysHeld = 0;
				defaultButtonView = " \uE0E0\uE0E1\uE0E2\uE0E3\uE0E4\uE0E5\uE0E6\uE0E7\uE0EB\uE0EC\uE0ED\uE0EE\uE0EF\uE0F0\uE104\uE105";
				tsl::hlp::requestForeground(true);
			}
			else if (keysDown & KEY_B) {
				tsl::hlp::requestForeground(true);
				tsl::goBack();
				return true;
			}
			else if (keysDown & KEY_A) {
				if (timer != 0) {
					ultrahandCombo = false;
					teslaCombo = false;
					m_configs->at(m_key) = m_value;
					m_out->assign(m_value);
				}
				tsl::hlp::requestForeground(true);
				tsl::goBack();
				return true;
			}
		}
		else {
			if (keysHeld != last_keysHeld) {
				last_keysHeld = keysHeld;
				last_tick = svcGetSystemTick();
			}
			else if (keysHeld == 0) {
				last_keysHeld = 0;
				last_tick = 0;
				timer = 0;
			}
			else if (last_tick > 0) {
				timer = svcGetSystemTick() - last_tick;
				if (timer > maxTimer) {
					changingCombo = false;
					defaultButtonView = footerTarget;
				}
			}
			convertHidnpadKeyToButtonCombination(keysHeld, m_valueToShow, m_value);
		}
		return false;
	}
};
