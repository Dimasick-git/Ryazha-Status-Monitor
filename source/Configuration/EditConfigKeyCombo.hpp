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
				constexpr s16 valueFontSize = 40;
				constexpr s16 footerFontSize = 30;
				const s16 valueBaseline = (360 + valueFontSize) - (valueFontSize / 2);
				auto [valueWidth, valueHeight] = renderer->drawString(m_valueToShow.c_str(), false, 0, valueFontSize, valueFontSize, renderer->a(0x0000), true);
				const s16 valueX = static_cast<s16>((tsl::cfg::FramebufferWidth - valueWidth) / 2);

				if (ult::useSwitch2Style) {
					const s16 panelX = std::max<s16>(20, valueX - 18);
					const s16 panelWidth = std::min<s16>(tsl::cfg::FramebufferWidth - 40, static_cast<s16>(valueWidth + 36));
					renderer->drawSwitch2Panel(panelX, valueBaseline - valueFontSize - 14, panelWidth, valueFontSize + 28, tsl::gfx::Color(ult::switch2::FooterBorder), tsl::gfx::Color(ult::switch2::FooterFill));
				}
				renderer->drawString(m_valueToShow.c_str(), false, valueX, valueBaseline, valueFontSize, renderer->a(0xFFFF), true);

				if (!changingCombo) {
					if (m_mainCombo && !m_footerValue.empty()) {
						auto [footerWidth, footerHeight] = renderer->drawString(m_footerValue.c_str(), false, 0, footerFontSize, footerFontSize, renderer->a(0x0000), true);
						renderer->drawString(m_footerValue.c_str(), false, (tsl::cfg::FramebufferWidth - footerWidth) / 2, valueBaseline + valueFontSize, footerFontSize, renderer->a(0xFFFF), true);
					}
					return;
				}

				const s16 progressX = 20;
				const s16 progressY = valueBaseline + valueFontSize;
				const s16 progressWidth = 408;
				const s16 progressHeight = 40;
				const float progress = std::min(1.0f, static_cast<float>(timer) / static_cast<float>(maxTimer));
				if (ult::useSwitch2Style) {
					renderer->drawSwitch2Panel(progressX, progressY, progressWidth, progressHeight, tsl::gfx::Color(ult::switch2::FooterBorder), tsl::gfx::Color(ult::switch2::FooterFill), 3);
					const s16 fillWidth = static_cast<s16>((progressWidth - 6) * progress);
					if (fillWidth > 0)
						renderer->drawRoundRect(progressX + 3, progressY + 3, fillWidth, progressHeight - 6, 1.0f, 1.0f, 1.0f, 1.0f, renderer->a(tsl::gfx::Color(ult::switch2::FocusAccent)));
				} else {
					renderer->drawEmptyRect(progressX, progressY, progressWidth, progressHeight, 0xFFFF);
					const s16 fillWidth = static_cast<s16>((progressWidth - 4) * progress);
					if (fillWidth > 0)
						renderer->drawRect(progressX + 2, progressY + 2, fillWidth, progressHeight - 3, 0xFFFF);
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
