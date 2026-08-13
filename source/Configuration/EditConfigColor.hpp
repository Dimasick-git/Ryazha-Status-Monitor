#pragma once

class EditConfigColor : public tsl::Gui {
private:
	std::string m_key;
	tsl::elm::ColorListItem* m_item;
	u16 m_color;
	std::string m_localName;
	std::string footerBackup;
public:
	uint64_t start_tick;
	int32_t row = 0;
	uint32_t selected_r;
	uint32_t selected_b;
	uint32_t selected_g;
	uint32_t selected_a;
	int32_t selected_predefined;
	std::array<u16, 16> predefinedColors = {0xFF7A, 0xFFF0, 0xFF80, 0xFF8E, 0xF053, 0xF0FF, 0xFFDC, 0xF808, 0xFEEE, 0xF80F, 0xF6CC, 0xF0F8, 0xF744, 0xFAAF, 0xF080, 0xF82E};

	EditConfigColor(std::string key, tsl::elm::ColorListItem* item, std::string localName) {
		footerBackup = defaultButtonView;
		defaultButtonView = locale["Footer"];
		if (localName.length() > 0) m_localName = localName;
		else m_localName = key;
		tsl::hlp::requestForeground(true);
		m_item = item;
		m_key = key;
		m_color = item->getColor().rgba;
		start_tick = svcGetSystemTick();
		uint32_t r = m_color & 0xF;
		uint32_t g = (m_color >> 4) & 0xF;
		uint32_t b = (m_color >> 8) & 0xF;
		uint32_t a = (m_color >> 12) & 0xF;

		selected_r = 15 - r;
		selected_g = 15 - g;
		selected_b = 15 - b;
		selected_a = a;
		selected_predefined = -1;
	}

	~EditConfigColor() {
		defaultButtonView = footerBackup;
	}

	virtual tsl::elm::Element* createUI() override {
		rootFrame = new tsl::elm::OverlayFrame(APP_TITLE, m_localName);
		auto Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
			const size_t position_y = 120;
			const size_t position_x = 60;
			const size_t radius = 18;
			const size_t border_size = 10;
			renderer->drawCircle(position_x, position_y, radius + border_size, true, 0xFFFF);
			renderer->drawCircle(position_x, position_y, radius, true, (m_color & 0xFFF) + 0xF000);
			renderer->drawCircle(position_x+80, position_y, radius + border_size, true, 0xF000);
			renderer->drawCircle(position_x+80, position_y, radius, true, (m_color & 0xFFF) + 0xF000);
			renderer->drawCircle(position_x+160, position_y, radius + border_size, true, 0xF00F);
			renderer->drawCircle(position_x+160, position_y, radius, true, (m_color & 0xFFF) + 0xF000);
			renderer->drawCircle(position_x+240, position_y, radius + border_size, true, 0xF0F0);
			renderer->drawCircle(position_x+240, position_y, radius, true, (m_color & 0xFFF) + 0xF000);
			renderer->drawCircle(position_x+320, position_y, radius + border_size, true, 0xFF00);
			renderer->drawCircle(position_x+320, position_y, radius, true, (m_color & 0xFFF) + 0xF000);

			const size_t square_position_y = 160;
			u16 colorRed = 0xF00F;
			u16 colorGreen = 0xF0F0;
			u16 colorBlue = 0xFF00;
			u16 transparency = 0x0000;
			const size_t square_size = 27;
			for(size_t i = 0; i < 16; i++) {
				renderer->drawRect(position_x+147, square_position_y+(i * (square_size+2)), square_size, square_size, colorRed);
				colorRed--;
				renderer->drawRect(position_x+227, square_position_y+(i * (square_size+2)), square_size, square_size, colorGreen);
				colorGreen -= 0x10;
				renderer->drawRect(position_x+308, square_position_y+(i * (square_size+2)), square_size, square_size, colorBlue);
				colorBlue -= 0x100;
				renderer->drawRect(position_x+67, square_position_y+(i * (square_size+2)), square_size, square_size, transparency);
				transparency += 0x1000;
				renderer->drawRect(position_x-13, square_position_y+(i * (square_size+2)), square_size, square_size, predefinedColors[i]);
			}
			
			const double cycleDuration = 1.0;  // 1 second for one full sine wave
			u64 delta_tick = svcGetSystemTick() - start_tick;
			double seconds = (double)delta_tick / (double)systemtickfrequency;
			double timeCounter = fmod(seconds, cycleDuration);
			float progress = (std::sin(2 * M_PI * timeCounter / cycleDuration) + 1) / 2;

			tsl::gfx::Color highlightColor1 = tsl::gfx::Color(0xFC82);
			tsl::gfx::Color highlightColor2 = tsl::gfx::Color(0xFFF8);

			tsl::gfx::Color highlightColor = {
                static_cast<u8>((highlightColor1.r - highlightColor2.r) * progress + highlightColor2.r),
                static_cast<u8>((highlightColor1.g - highlightColor2.g) * progress + highlightColor2.g),
                static_cast<u8>((highlightColor1.b - highlightColor2.b) * progress + highlightColor2.b),
                0xF
            };

			renderer->drawEmptyRect(position_x+146, square_position_y+(selected_r * (square_size+2))-1, square_size + 2, square_size + 2, highlightColor);
			renderer->drawEmptyRect(position_x+145, square_position_y+(selected_r * (square_size+2))-2, square_size + 4, square_size + 4, highlightColor);
			renderer->drawEmptyRect(position_x+226, square_position_y+(selected_g * (square_size+2))-1, square_size + 2, square_size + 2, highlightColor);
			renderer->drawEmptyRect(position_x+225, square_position_y+(selected_g * (square_size+2))-2, square_size + 4, square_size + 4, highlightColor);
			renderer->drawEmptyRect(position_x+307, square_position_y+(selected_b * (square_size+2))-1, square_size + 2, square_size + 2, highlightColor);
			renderer->drawEmptyRect(position_x+306, square_position_y+(selected_b * (square_size+2))-2, square_size + 4, square_size + 4, highlightColor);
			renderer->drawEmptyRect(position_x+66, square_position_y+(selected_a * (square_size+2))-1, square_size + 2, square_size + 2, highlightColor);
			renderer->drawEmptyRect(position_x+65, square_position_y+(selected_a * (square_size+2))-2, square_size + 4, square_size + 4, highlightColor);

			if (selected_predefined >= 0) {
				renderer->drawEmptyRect(position_x-14, square_position_y+(selected_predefined * (square_size+2))-1, square_size + 2, square_size + 2, highlightColor);
				renderer->drawEmptyRect(position_x-15, square_position_y+(selected_predefined * (square_size+2))-2, square_size + 4, square_size + 4, highlightColor);				
			}

			renderer->drawEmptyRect(position_x+60 + (row * 80), square_position_y-9, 40, (square_size+3) * 16, highlightColor);
        });
		rootFrame->setContent(Status);
		return rootFrame;
	}

	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		if (keysDown & KEY_B) {
			tsl::hlp::requestForeground(true);
			tsl::goBack();
			return true;
		}
		else if (keysDown & KEY_DRIGHT) {
			if (row == -1) {
				auto it = std::find(predefinedColors.cbegin(), predefinedColors.cend(), m_color);
				if (it == predefinedColors.cend()) selected_predefined = -1;
			}
			if (row < 3) row++;
		}
		else if (keysDown & KEY_DLEFT) {
			if (row == -1) {
				m_color = predefinedColors[selected_predefined];
			}
			if (row > -1) row--;
			if (row == -1) {
				auto it = std::find(predefinedColors.cbegin(), predefinedColors.cend(), m_color);
				if (it == predefinedColors.cend()) selected_predefined = 0;
			}
		}
		else if (keysDown & KEY_DDOWN) {
			switch(row) {
				case -1: 
					if (selected_predefined < 15) {
						selected_predefined++;
						m_color = predefinedColors[selected_predefined];
					}
					break;
				case 0: if (selected_a < 15) selected_a++; break;
				case 1: if (selected_r < 15) selected_r++; break;
				case 2: if (selected_g < 15) selected_g++; break;
				case 3: if (selected_b < 15) selected_b++; break;
			}
			if (row > -1) {
				m_color = (selected_a << 12) + ((15 - selected_b) << 8) + ((15 - selected_g) << 4) + (15 - selected_r);
				auto it = std::find(predefinedColors.cbegin(), predefinedColors.cend(), m_color);
				if (it == predefinedColors.cend()) selected_predefined = -1;
			}
			else {
				uint32_t r = m_color & 0xF;
				uint32_t g = (m_color >> 4) & 0xF;
				uint32_t b = (m_color >> 8) & 0xF;

				selected_r = 15 - r;
				selected_g = 15 - g;
				selected_b = 15 - b;
			}
		}
		else if (keysDown & KEY_DUP) {
			switch(row) {
				case -1: 
					if (selected_predefined > 0) {
						selected_predefined--;
						m_color = predefinedColors[selected_predefined];
					}
					break;
				case 0: if (selected_a > 0) selected_a--; break;
				case 1: if (selected_r > 0) selected_r--; break;
				case 2: if (selected_g > 0) selected_g--; break;
				case 3: if (selected_b > 0) selected_b--; break;
			}
			if (row > -1) {
				m_color = (selected_a << 12) + ((15 - selected_b) << 8) + ((15 - selected_g) << 4) + (15 - selected_r);
				auto it = std::find(predefinedColors.cbegin(), predefinedColors.cend(), m_color);
				if (it == predefinedColors.cend()) selected_predefined = -1;
			}
			else {
				uint32_t r = m_color & 0xF;
				uint32_t g = (m_color >> 4) & 0xF;
				uint32_t b = (m_color >> 8) & 0xF;

				selected_r = 15 - r;
				selected_g = 15 - g;
				selected_b = 15 - b;
			}
		}
		if (keysDown & KEY_A) {
			char buffer[16] = "";
			uint32_t r = m_color & 0xF;
			uint32_t g = (m_color >> 4) & 0xF;
			uint32_t b = (m_color >> 8) & 0xF;
			uint16_t in_color = (r << 12) + (g << 8) + (b << 4) + selected_a;
			snprintf(buffer, sizeof(buffer), "COLOR{0x%04X}", in_color);
			configs.at(m_key).value = buffer;
			m_item->setColor((m_color & 0xFFF) + (selected_a << 12));
			tsl::hlp::requestForeground(true);
			tsl::goBack();
			return true;
		}
		return false;
	}
};
