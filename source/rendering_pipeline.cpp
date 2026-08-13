#include <tesla.hpp>
#include "rendering_pipeline.hpp"
#include "Utils.hpp"
#include "Extensions/smse.hpp"

// Globals defined in main.cpp that rendering_pipeline needs
extern tsl::elm::OverlayFrame* rootFrame;
extern HidSixAxisSensorHandle sixaxisHandles[Controller_Max];

// ─── Private static methods ──────────────────────────────────────────────────

void RenderingPipeline::TrackRect(int64_t x, int64_t y, int64_t w, int64_t h) {
	if (w <= 0 || h <= 0) return;
	for (const auto& r : s_rects) {
		if (x >= r.x && y >= r.y
		 && x + w <= r.x + r.w && y + h <= r.y + r.h) return;
	}
	s_rects.push_back({x, y, w, h});
}

void RenderingPipeline::RenderGraphLineChart(smd::RenderCommand& cmd, tsl::gfx::Renderer* m_renderer) {
	uint16_t lineColor = cmd.color;
	uint16_t fillColor = cmd.fillColor;
	uint16_t previousLineColor1 = lineColor;
	uint16_t previousLineColor2 = fillColor;

	const int64_t denomX = std::max<int64_t>((int64_t)cmd.sampleCapacity - 1, 1);
	const int64_t denomY = std::max<int64_t>(cmd.maxClamp - cmd.minClamp, 1);
	const int64_t spanX  = std::max<int64_t>(cmd.width  - 1, 0);
	auto scaleY = [&](int64_t v) -> int64_t {
		return (v - cmd.minClamp) * cmd.height / denomY;
	};

	auto scaleX_LTR = [&](int64_t i) -> int64_t {
		int64_t age = (int64_t)cmd.sampleCount - 1 - i;
		return age * spanX / denomX;
	};
	auto scaleX_RTL = [&](int64_t i) -> int64_t {
		int64_t age = (int64_t)cmd.sampleCount - 1 - i;
		return spanX - age * spanX / denomX;
	};

	auto drawing_impl = [&](smd::GraphDirection direction, int64_t i) {
		bool conditionMet = false;
		for (size_t c = 0; c < cmd.condCount; c++) {
			const auto& cond = cmd.conditions[c];
			if (cond.matches(cond.state, cmd.sampleType == smd::HistorySampleType::Int ? cmd.samples[i] : cmd.samplesD[i])) {
				conditionMet = true;
				if (cond.lineColor == previousLineColor1 && cond.fillColor == previousLineColor2) {
					lineColor = cond.lineColor;
					fillColor = cond.fillColor;
				}
				previousLineColor1 = cond.lineColor;
				previousLineColor2 = cond.fillColor;
				break;
			}
		}
		if (!conditionMet) {
			lineColor = cmd.color;
			fillColor = cmd.fillColor;
			previousLineColor1 = lineColor;
			previousLineColor2 = fillColor;
		}
		int64_t x1 = COMMON_MARGIN + cmd.x + (cmd.direction == smd::GraphDirection::LeftToRight ? scaleX_LTR(i) : scaleX_RTL(i));
		int64_t x2 = COMMON_MARGIN + cmd.x + (cmd.direction == smd::GraphDirection::LeftToRight ? scaleX_LTR(i+1) : scaleX_RTL(i+1));
		if (cmd.sampleType == smd::HistorySampleType::Int) {
			auto value1 = std::clamp(cmd.samples[i], cmd.minClamp, cmd.maxClamp);
			auto value2 = std::clamp(cmd.samples[i+1], cmd.minClamp, cmd.maxClamp);
			int64_t y1 = cmd.y + (cmd.height - scaleY(value1));
			int64_t y2 = cmd.y + (cmd.height - scaleY(value2));
			m_renderer->drawLine(x1, y1, x2, y2, lineColor);
		}
		else {
			auto value1 = std::round(std::clamp(cmd.samplesD[i], (double)cmd.minClamp, (double)cmd.maxClamp));
			auto value2 = std::round(std::clamp(cmd.samplesD[i+1], (double)cmd.minClamp, (double)cmd.maxClamp));
			int64_t y1 = cmd.y + (cmd.height - scaleY((int64_t)value1));
			int64_t y2 = cmd.y + (cmd.height - scaleY((int64_t)value2));
			m_renderer->drawLine(x1, y1, x2, y2, lineColor);
		}
	};

	if (cmd.direction == smd::GraphDirection::LeftToRight) {
		for (int64_t i = 0; i < (int64_t)cmd.sampleCount - 1; i++) {
			drawing_impl(cmd.direction, i);
		}
	}
	else for (int64_t i = cmd.sampleCount - 2; i >= 0; i--) {
		drawing_impl(cmd.direction, i);
	}
}

void RenderingPipeline::PreRecordCallback(std::string& outLocaleCode, void* user) {
	outLocaleCode = overrideLanguage;
}

void RenderingPipeline::RecordCallback(smd::RenderCommand& cmd, void* user) {
	auto* m_renderer = static_cast<tsl::gfx::Renderer*>(user);

	const int64_t orig_x = cmd.x, orig_y = cmd.y;
	cmd.x += m_obj_offset_x_screen;
	cmd.y += m_obj_offset_y_screen;
	const int64_t orig_x2 = cmd.x2, orig_y2 = cmd.y2;
	if (cmd.type == smd::RenderCmdType::DashedLine) {
		cmd.x2 += m_obj_offset_x_screen;
		cmd.y2 += m_obj_offset_y_screen;
	}
	switch(cmd.type) {
		case smd::RenderCmdType::Text: {
			auto dims = m_renderer->drawString(cmd.text.c_str(), false, COMMON_MARGIN + cmd.x, cmd.y, cmd.fontSize, m_renderer->a(cmd.color), cmd.matchLineHeight);
			TrackRect(COMMON_MARGIN + orig_x, orig_y, dims.first, dims.second);
			break;
		}
		case smd::RenderCmdType::Box:
			m_renderer->drawRect(COMMON_MARGIN + cmd.x, cmd.y, cmd.width, cmd.height, m_renderer->a(cmd.color));
			TrackRect(COMMON_MARGIN + orig_x, orig_y, cmd.width, cmd.height);
			break;
		case smd::RenderCmdType::RoundedBox: {
			const auto radius = static_cast<s32>(std::max(
				std::max(cmd.roundnessTl, cmd.roundnessTr),
				std::max(cmd.roundnessBl, cmd.roundnessBr)
			));
			m_renderer->drawRoundedRect(
				static_cast<s32>(COMMON_MARGIN + cmd.x), static_cast<s32>(cmd.y),
				static_cast<s32>(cmd.width), static_cast<s32>(cmd.height),
				radius, m_renderer->a(cmd.color)
			);
			TrackRect(COMMON_MARGIN + orig_x, orig_y, cmd.width, cmd.height);
			break;
		}
		case smd::RenderCmdType::EmptyBox:
			m_renderer->drawEmptyRect(COMMON_MARGIN + cmd.x, cmd.y, cmd.width, cmd.height, m_renderer->a(cmd.color));
			TrackRect(COMMON_MARGIN + orig_x, orig_y, cmd.width, cmd.height);
			break;
		case smd::RenderCmdType::DashedLine: {
			m_renderer->drawDashedLine(
				static_cast<s32>(COMMON_MARGIN + cmd.x), static_cast<s32>(cmd.y),
				static_cast<s32>(cmd.x2), static_cast<s32>(cmd.y2),
				static_cast<s32>(std::max<int64_t>(1, cmd.dashOn)),
				m_renderer->a(cmd.color)
			);

			int64_t a = COMMON_MARGIN + orig_x, b = orig_x2;
			int64_t c = orig_y,                 d = orig_y2;
			int64_t x0 = a < b ? a : b, x1 = a < b ? b : a;
			int64_t y0 = c < d ? c : d, y1 = c < d ? d : c;
			TrackRect(x0, y0, x1 - x0 > 0 ? x1 - x0 : 1, y1 - y0 > 0 ? y1 - y0 : 1);

			break;
		}
		case smd::RenderCmdType::GetDimensions: {
			auto dimensions = m_renderer->drawString(cmd.text.c_str(), false, 0, cmd.fontSize, cmd.fontSize, m_renderer->a(0x0000), cmd.matchLineHeight);
			cmd.outDims->x = dimensions.first;
			cmd.outDims->y = dimensions.second;
			break;
		}
		case smd::RenderCmdType::GraphLineChart:
			RenderGraphLineChart(cmd, m_renderer);
			TrackRect(COMMON_MARGIN + orig_x, orig_y, cmd.width, cmd.height);
			break;
		default:
			break;
	}
	cmd.x = orig_x; cmd.y = orig_y;
	cmd.x2 = orig_x2; cmd.y2 = orig_y2;
}

void RenderingPipeline::DryRunCallback(smd::RenderCommand& cmd, void* user) {
	auto* m_renderer = static_cast<tsl::gfx::Renderer*>(user);
	switch(cmd.type) {
		case smd::RenderCmdType::Text: {
			auto dims = m_renderer->drawString(cmd.text.c_str(), false, COMMON_MARGIN + cmd.x, cmd.y, cmd.fontSize, m_renderer->a(0x0000), cmd.matchLineHeight);
			TrackRect(COMMON_MARGIN + cmd.x, cmd.y, dims.first, dims.second);
			break;
		}
		case smd::RenderCmdType::Box:
		case smd::RenderCmdType::RoundedBox:
		case smd::RenderCmdType::EmptyBox:
			TrackRect(COMMON_MARGIN + cmd.x, cmd.y, cmd.width, cmd.height);
			break;
		case smd::RenderCmdType::DashedLine: {
			int64_t a = COMMON_MARGIN + cmd.x, b = cmd.x2;
			int64_t c = cmd.y,                 d = cmd.y2;
			int64_t x0 = a < b ? a : b, x1 = a < b ? b : a;
			int64_t y0 = c < d ? c : d, y1 = c < d ? d : c;
			TrackRect(x0, y0, x1 - x0 > 0 ? x1 - x0 : 1, y1 - y0 > 0 ? y1 - y0 : 1);
			break;
		}
		case smd::RenderCmdType::GetDimensions: {
			auto dimensions = m_renderer->drawString(cmd.text.c_str(), false, 0, cmd.fontSize, cmd.fontSize, m_renderer->a(0x0000), cmd.matchLineHeight);
			cmd.outDims->x = dimensions.first;
			cmd.outDims->y = dimensions.second;
			break;
		}
		case smd::RenderCmdType::GraphLineChart:
			TrackRect(COMMON_MARGIN + cmd.x, cmd.y, cmd.width, cmd.height);
			break;
		default:
			break;
	}
}

// ─── Private non-static method ───────────────────────────────────────────────

bool RenderingPipeline::IsInsideTouchRange(int64_t screen_x, int64_t screen_y, int64_t pad) const {
	if (screen_x < 0 || screen_y < 0) return false;

	int64_t lx = screen_x - (m_layer_pos_x_window * 2 / 3) - m_obj_offset_x_screen;
	int64_t ly = screen_y - (m_layer_pos_y_window * 2 / 3) - m_obj_offset_y_screen;
	if (lx < -pad || ly < -pad) return false;
	// Relocation may only start from a rendered monitor widget. This remains true
	// even though every mode now uses an opaque background.
	for (const auto& r : s_rects)
		if (lx >= r.x-pad && ly >= r.y-pad && lx < r.x+r.w+pad && ly < r.y+r.h+pad) return true;
	return false;
}

bool RenderingPipeline::isExitComboHeld(uint64_t keysHeld, uint64_t comboBitmask, uint64_t holdDurationNs) {
	const uint64_t physicalKeys = keysHeld & ALL_KEYS_MASK;
	const bool comboMatches = comboBitmask != 0
		&& (physicalKeys & comboBitmask) == comboBitmask
		&& (physicalKeys & ~comboBitmask) == 0;
	if (!comboMatches) {
		m_exitComboHoldStartedNs = 0;
		return false;
	}

	const uint64_t nowNs = armTicksToNs(svcGetSystemTick());
	if (m_exitComboHoldStartedNs == 0) {
		m_exitComboHoldStartedNs = nowNs;
		return false;
	}
	return nowNs - m_exitComboHoldStartedNs >= holdDurationNs;
}

inline uint64_t get_current_heap_position() {
    return (uintptr_t)(sbrk(0));
}

size_t RenderingPipeline::getFreeHeapMemory() const {
	uint64_t heap_pos = get_current_heap_position();
	MemoryInfo info;
	u32 dummy;
	svcQueryMemory(&info, &dummy, heap_pos);
	return (info.addr+info.size) - heap_pos;
}

void RenderingPipeline::persistMovablePosition() {
	if (!Movable || !saveAndLoadMovableOverlayPosition || rel_filepath.empty()) return;

	const uint32_t savedX = reachedMaxX ? 1280U : m_base_x;
	const uint32_t savedY = reachedMaxY ? 720U : m_base_y;
	char hashBuffer[10] = {};
	auto [hashEnd, hashError] = std::to_chars(
		&hashBuffer[0], &hashBuffer[sizeof(hashBuffer)], smd_hash, 16
	);
	if (hashError != std::errc{}) return;

	setIniFile("sdmc:/config/status-monitor/config.ini", rel_filepath, "x", std::to_string(savedX), "");
	setIniFile("sdmc:/config/status-monitor/config.ini", rel_filepath, "y", std::to_string(savedY), "");
	setIniFile(
		"sdmc:/config/status-monitor/config.ini", rel_filepath, "hash",
		std::string(hashBuffer, hashEnd), ""
	);
}

// ─── Constructor ─────────────────────────────────────────────────────────────

RenderingPipeline::RenderingPipeline(std::string filepath, bool double_back) {
	footerBackup = defaultButtonView;
	defaultButtonView = locale["Footer"];
	m_double_back = double_back;
	m_menuBackgroundColor = tsl::defaultBackgroundColor;
	leftJoyconMotionMappedButtons   = MapButtons(leftJoyconMotionKeyCombo);
	rightJoyconMotionMappedButtons  = MapButtons(rightJoyconMotionKeyCombo);
	proControllerMotionMappedButtons = MapButtons(proControllerMotionKeyCombo);
	m_layer_pos_x_window  = 0;
	m_layer_pos_y_window  = 0;
	m_obj_offset_x_screen = 0;
	m_obj_offset_y_screen = 0;
	tsl::gfx::Renderer::get().setLayerPos(0, 0);
	if (SaltySD) {
		uintptr_t base = (uintptr_t)shmemGetAddr(&_sharedmemory);
		if (base) displayRefreshRate = (uint8_t*)(base + 1);
	}
	ApmPerformanceMode performanceMode;
	SystemData.IsDocked = false;
	if (R_SUCCEEDED(apmGetPerformanceMode(&performanceMode))) {
		if (performanceMode == ApmPerformanceMode_Boost) SystemData.IsDocked = true;
	}
	std::string formattedKeyCombo = keyCombo;
	formatButtonCombination(formattedKeyCombo);
	SystemData.formattedKeyCombo = formattedKeyCombo;
	smd::Document::PeekInfo info;
	smd::Document::Peek(filepath.c_str(), info, overrideLanguage.c_str());
	name = info.name;
	doc.Free();
	doc.SetRecordCallback(PreRecordCallback, nullptr);
	if (doc.LoadFromFile(filepath.c_str()) == false) {
		error = doc.LastError();
		return;
	}
	BindAllPredefined(doc);
	for (auto& v : smseGetAllVars()) {
		if (v.accessor.isInt()) doc.BindInt64(v.name.c_str(), v.accessor.intPtr());
		else if (v.accessor.isFloat()) doc.BindFloat(v.name.c_str(), v.accessor.f32Ptr());
		else if (v.accessor.isDouble()) doc.BindDouble(v.name.c_str(), v.accessor.f64Ptr());
	}
	if (doc.Compile() == false) {
		error = doc.LastError();
		return;
	}
	Movable = doc.GetConfigBool("Movable", false);
	rel_filepath = filepath.substr(strlen("sdmc:/config/status-monitor/modes/"));
	if (Movable && saveAndLoadMovableOverlayPosition) {
		smd_hash = doc.GetFileHash();
		uint16_t saved_x_pos;
		uint16_t saved_y_pos;
		bool wasSuccess = ProcessSmdSettings(rel_filepath, smd_hash, &saved_x_pos, &saved_y_pos);
		if (wasSuccess == false) {
			saved_x_pos = 0;
			saved_y_pos = 0;
			char buffer[10] = "";
			auto [ptr, ec] = std::to_chars(&buffer[0], &buffer[sizeof(buffer)], smd_hash, 16);
			if (ec == std::errc{}) {
				std::string value = std::string(&buffer[0], ptr - &buffer[0]);
				setIniFile("sdmc:/config/status-monitor/config.ini", rel_filepath, "hash", value, "");
				setIniFile("sdmc:/config/status-monitor/config.ini", rel_filepath, "x", "0", "");
				setIniFile("sdmc:/config/status-monitor/config.ini", rel_filepath, "y", "0", "");
			}
		}
		else if (saved_x_pos > 1280 || saved_y_pos > 720) {
			saved_x_pos = 0;
			saved_y_pos = 0;
		}
		m_saved_base_x = (int64_t)saved_x_pos;
		m_saved_base_y = (int64_t)saved_y_pos;
		if (saved_x_pos == 1280) reachedMaxX = true;
		if (saved_y_pos == 720) reachedMaxY = true;



	}
	HeaderText = doc.GetConfigBool("HeaderText", true);
	FooterText = doc.GetConfigBool("FooterText", true);
	bool focus = doc.GetConfigBool("EnableControls", true);
	tsl::hlp::requestForeground(focus);
	UseCustomExitCombo = doc.GetConfigBool("UseCustomExitCombo", false);
	if (FooterText == false) {
		deactivateOriginalFooter = true;
		FullMode = false;
	}
	if (UseCustomExitCombo == true) {
		deactivateOriginalFooter = true;
		mappedButtons = MapButtons(keyCombo);
		if (doc.FormatConfigString("ComboButtonFooter", ComboButtonFooter) == false) {
			ComboButtonFooter = "Hold ";
			ComboButtonFooter += keyCombo + " to Exit+";
		}
	}
	COMMON_MARGIN = doc.GetConfigInt("COMMON_MARGIN", 20);
	std::string section_name = rel_filepath;
	auto section = config.find(section_name);

	if (section != config.end()) {
		for (const auto& [m_key, value] : section->second) {
			auto key = m_key.c_str();
			const smd::ConfigValue* entry = doc.GetConfig(key);
			if (entry != nullptr) {
				switch(entry->kind) {
					case smd::ConfigKind::Integer: {
						int64_t int_value;
						bool good = isNumeric(value, &int_value);
						if (good == true) doc.SetConfigInt(key, int_value);
						else if (value.starts_with("COLOR")) doc.SetConfigColor(key, value.c_str());
						break;
					}
					case smd::ConfigKind::Bool: {
						bool isTrue = value.compare("true") == 0;
						bool isFalse = value.compare("false") == 0;
						if (isTrue || isFalse) doc.SetConfigBool(key, isTrue);
						break;
					}
					case smd::ConfigKind::List: {
						if (value.starts_with("LIST")) doc.SetConfigList(key, value.c_str());
						else doc.SetConfigList(key, flatListToList(value).c_str());
						break;
					}
					default:
						break;
				}
			}
		}
	}
	auto test = doc.GetConfigInt("User_BackgroundColor", 0xFFFFFF);
	const uint16_t modeBackground = static_cast<uint16_t>(
		test == 0xFFFFFF ? doc.GetConfigInt("BackgroundColor", 0xD000) : test
	);
	tsl::defaultBackgroundColor = tsl::Color(modeBackground);
	// Every monitor mode must retain a visible backdrop, independent of an SMD's
	// historic alpha nibble or the active system theme transparency preference.
	tsl::defaultBackgroundColor.a = 0xF;
	ClampToLayerRight  = doc.GetConfigBool("ClampToLayerRight",  false);
	ClampToLayerBottom = doc.GetConfigBool("ClampToLayerBottom", false);
	if (Movable && motionControl) {
		hidStartSixAxisSensor(sixaxisHandles[Controller_ProController]);
		hidStartSixAxisSensor(sixaxisHandles[Controller_JoyConL]);
		hidStartSixAxisSensor(sixaxisHandles[Controller_JoyConR]);
	}
	int64_t overlayRefreshRate = doc.GetConfigInt("RefreshRate", 0);
	if (overlayRefreshRate == 0) overlayRefreshRate = doc.GetConfigInt("User_RefreshRate", 0);
	if (overlayRefreshRate > 0) timeout = 1'000'000'000 / overlayRefreshRate;
	if (timeout < 15'000'000) timeout = 15'000'000;
	bool EnableCPU   = doc.GetConfigBool("EnableCPU",   false);
	bool EnableGPU   = doc.GetConfigBool("EnableGPU",   false);
	bool EnableRAM   = doc.GetConfigBool("EnableRAM",   false);
	bool EnableBoard = doc.GetConfigBool("EnableBoard", false);
	bool EnableMisc  = doc.GetConfigBool("EnableMisc",  false);
	bool EnableGame  = doc.GetConfigBool("EnableGame",  false);
	if (EnableCPU)   threadCreate(&threads[threads_count++], CpuThreadLoop,   (void*)overlayRefreshRate, NULL, 0x1000, 0x3F, -2);
	if (EnableGPU)   threadCreate(&threads[threads_count++], GpuThreadLoop,   NULL, NULL, 0x1000, 0x3F, -2);
	if (EnableRAM)   threadCreate(&threads[threads_count++], RamThreadLoop,   NULL, NULL, 0x1000, 0x3F, -2);
	if (EnableBoard) threadCreate(&threads[threads_count++], BoardThreadLoop, NULL, NULL, 0x1000, 0x3F, -2);
	if (EnableMisc)  threadCreate(&threads[threads_count++], MiscThreadLoop,  NULL, NULL, 0x1000, 0x3F, -2);
	if (EnableGame)  threadCreate(&threads[threads_count++], GameThreadLoop,  (void*)overlayRefreshRate, NULL, 0x1000, 0x3F, -2);
	if (threads_count > 0) {
		threadexit2 = 0;
		leventClear(&threadexit);
	}
	for (size_t i = 0; i < threads_count; i++) {
		threadStart(&threads[i]);
	}
}

// ─── Destructor ──────────────────────────────────────────────────────────────

RenderingPipeline::~RenderingPipeline() {
	defaultButtonView = footerBackup;
	svcSignalToAddress(&threadexit2, SignalType_SignalAndIncrementIfEqual, 0, 4);
	leventSignal(&threadexit);
	persistMovablePosition();
	m_obj_offset_x_screen = 0;
	m_obj_offset_y_screen = 0;
	tsl::gfx::Renderer::get().setLayerPos(0, 0);
	if (Movable & motionControl) {
		hidStopSixAxisSensor(sixaxisHandles[Controller_ProController]);
		hidStopSixAxisSensor(sixaxisHandles[Controller_JoyConL]);
		hidStopSixAxisSensor(sixaxisHandles[Controller_JoyConR]);
	}
	tsl::defaultBackgroundColor = m_menuBackgroundColor;
	tsl::defaultBackgroundColor.a = 0xF;
	deactivateOriginalFooter = false;
	FullMode = true;
	tsl::hlp::requestForeground(true);
	for (size_t i = 0; i < threads_count; i++) {
		threadWaitForExit(&threads[i]);
		threadClose(&threads[i]);
	}
	s_rects.clear();
}

// ─── createUI ────────────────────────────────────────────────────────────────

tsl::elm::Element* RenderingPipeline::createUI() {
	if (HeaderText == true)
		rootFrame = new tsl::elm::OverlayFrame(APP_TITLE, name);
	else rootFrame = new tsl::elm::OverlayFrame("", "");
	auto Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
		if (error.length() > 0) {
			renderer->drawString(error.c_str(), false, 20, 120, 20, renderer->a(0xFFFF));
		}
		else {
			bool needsDryRun = ((ClampToLayerRight || ClampToLayerBottom) && !changingPos)
			                 || (m_saved_base_x >= 0);
			if (needsDryRun) {
				doc.Evaluate(DryRunCallback, renderer);
				if (!s_rects.empty()) {
					int64_t mnx = s_rects[0].x, mny = s_rects[0].y;
					int64_t mxx = s_rects[0].x + s_rects[0].w;
					int64_t mxy = s_rects[0].y + s_rects[0].h;
					for (const auto& r : s_rects) {
						if (r.x < mnx) mnx = r.x;
						if (r.y < mny) mny = r.y;
						if (r.x + r.w > mxx) mxx = r.x + r.w;
						if (r.y + r.h > mxy) mxy = r.y + r.h;
					}
					int64_t layer_w = (int64_t)tsl::cfg::FramebufferWidth;
					int64_t layer_h = (int64_t)tsl::cfg::FramebufferHeight;
					if (m_saved_base_x >= 0) {
						touch_pos_x    = m_saved_base_x + mnx;
						touch_pos_y    = m_saved_base_y + mny;
						m_anchor_offset_x = 0;
						m_anchor_offset_y = 0;
						int64_t T_screen_x = touch_pos_x - mnx;
						int64_t T_screen_y = touch_pos_y - mny;
						int64_t T_win_x = T_screen_x * 3 / 2;
						int64_t T_win_y = T_screen_y * 3 / 2;
						int64_t max_x = tsl::cfg::ScreenWidth  - (int64_t)tsl::cfg::LayerWidth;
						int64_t max_y = tsl::cfg::ScreenHeight - (int64_t)tsl::cfg::LayerHeight;
						if (max_x < 0) max_x = 0;
						if (max_y < 0) max_y = 0;
						m_layer_pos_x_window = std::clamp<int64_t>(T_win_x, 0, max_x);
						m_layer_pos_y_window = std::clamp<int64_t>(T_win_y, 0, max_y);
						int64_t raw_obj_x = T_screen_x - (m_layer_pos_x_window * 2 / 3);
						int64_t raw_obj_y = T_screen_y - (m_layer_pos_y_window * 2 / 3);
						int64_t obj_max_x = layer_w - mxx, obj_min_x = -mnx;
						int64_t obj_max_y = layer_h - mxy, obj_min_y = -mny;
						m_obj_offset_x_screen = std::clamp(raw_obj_x, obj_min_x, obj_max_x >= obj_min_x ? obj_max_x : obj_min_x);
						m_obj_offset_y_screen = std::clamp(raw_obj_y, obj_min_y, obj_max_y >= obj_min_y ? obj_max_y : obj_min_y);
						reachedMaxX = (m_obj_offset_x_screen + mxx >= layer_w);
						reachedMaxY = (m_obj_offset_y_screen + mxy >= layer_h);
						tsl::gfx::Renderer::get().setLayerPos((uint32_t)m_layer_pos_x_window, (uint32_t)m_layer_pos_y_window);
						touch_pos_x = -1;
						touch_pos_y = -1;
						m_saved_base_x = -1;
						m_saved_base_y = -1;
					}
					if (ClampToLayerRight  && reachedMaxX) m_obj_offset_x_screen = layer_w - mxx;
					if (ClampToLayerBottom && reachedMaxY) m_obj_offset_y_screen = layer_h - mxy;
				}
				s_rects.clear();
				doc.Reset(true);
			}
			if (doc.Evaluate(RecordCallback, renderer) == false) {
				error = doc.LastError();
			}
		}
			if (deactivateOriginalFooter == true && FullMode == true)
				renderer->drawString(ComboButtonFooter.c_str(), false, 30, 693, 23, renderer->a(tsl::defaultTextColor));
	});

	rootFrame->setContent(Status);
	return rootFrame;
}

// ─── update ──────────────────────────────────────────────────────────────────

void RenderingPipeline::update() {
	uint64_t tick = svcGetSystemTick();
	if (tick - last_tick > (systemtickfrequency / 2)) {
		smseExecuteAll();
		last_tick = tick;
	}
	SystemData.OverlayRenderingFrameTimeInNs = 0;
	SystemData.OverlayMemoryLeftInB =  getFreeHeapMemory();
	if (error.length() != 0) return;
	s_rects.clear();
	doc.Reset(changingPos);
	SystemData.IsDocked = appletGetOperationMode() != AppletOperationMode_Handheld;
	if (displayRefreshRate) {
		SystemData.DisplayRefreshRate_int = *displayRefreshRate;
	}
	uint64_t deltaTick = tick - LocalTime.relative_tick;
	int64_t seconds_passed = deltaTick / systemtickfrequency;
	time_t new_timestamp = LocalTime.timestamp + seconds_passed;
	struct tm local_time;
	gmtime_r(&new_timestamp, &local_time);
	SystemData.ClockHour     = local_time.tm_hour;
	SystemData.ClockMinute   = local_time.tm_min;
	SystemData.ClockSecond   = local_time.tm_sec;
	SystemData.CalendarDay   = local_time.tm_mday;
	SystemData.CalendarMonth = local_time.tm_mon + 1;
	SystemData.CalendarYear  = local_time.tm_year + 1900;
}

// ─── handleInput ─────────────────────────────────────────────────────────────

bool RenderingPipeline::handleInput(uint64_t keysDown, uint64_t keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) {
	if (tsl::cfg::LayerWidth != m_last_layer_w || tsl::cfg::LayerHeight != m_last_layer_h) {
		m_layer_pos_x_window = tsl::cfg::LayerPosX;
		m_layer_pos_y_window = tsl::cfg::LayerPosY;
		m_last_layer_w = tsl::cfg::LayerWidth;
		m_last_layer_h = tsl::cfg::LayerHeight;
	}

	if (error.length() != 0) {
		if (isExitComboHeld(keysHeld, mappedButtons, 100'000'000)) [[unlikely]] {
			tsl::goBack();
			if (m_double_back == true) tsl::goBack();
			return true;
		}
		return false;
	}

	int64_t mnx = 0, mny = 0;
	bool haveBounds = !s_rects.empty();
	if (haveBounds) {
		mnx = s_rects[0].x; mny = s_rects[0].y;
		for (const auto& r : s_rects) {
			if (r.x < mnx) mnx = r.x;
			if (r.y < mny) mny = r.y;
		}
		m_base_x = (uint32_t)std::max<int64_t>(0,
			mnx + (m_layer_pos_x_window * 2 / 3) + m_obj_offset_x_screen);
		m_base_y = (uint32_t)std::max<int64_t>(0,
			mny + (m_layer_pos_y_window * 2 / 3) + m_obj_offset_y_screen);
	}

	auto applyDrag = [&]() {
		if (!haveBounds) return;

		if (touch_pos_y >= 704) touch_pos_y = 720;
		else if (touch_pos_y <= 15) touch_pos_y = 0;
		if (touch_pos_x >= 1264) touch_pos_x = 1280;
		else if (touch_pos_x <= 15) touch_pos_x = 0;
		int64_t T_screen_x = touch_pos_x - m_anchor_offset_x - mnx;
		int64_t T_screen_y = touch_pos_y - m_anchor_offset_y - mny;

		int64_t mxx = s_rects[0].x + s_rects[0].w;
		int64_t mxy = s_rects[0].y + s_rects[0].h;
		for (const auto& r : s_rects) {
			if (r.x + r.w > mxx) mxx = r.x + r.w;
			if (r.y + r.h > mxy) mxy = r.y + r.h;
		}
		int64_t layer_w = (int64_t)tsl::cfg::FramebufferWidth;
		int64_t layer_h = (int64_t)tsl::cfg::FramebufferHeight;

		int64_t T_win_x = T_screen_x * 3 / 2;
		int64_t T_win_y = T_screen_y * 3 / 2;
		int64_t max_x = tsl::cfg::ScreenWidth  - (int64_t)tsl::cfg::LayerWidth;
		int64_t max_y = tsl::cfg::ScreenHeight - (int64_t)tsl::cfg::LayerHeight;
		if (max_x < 0) max_x = 0;
		if (max_y < 0) max_y = 0;
		m_layer_pos_x_window = std::clamp<int64_t>(T_win_x, 0, max_x);
		m_layer_pos_y_window = std::clamp<int64_t>(T_win_y, 0, max_y);

		int64_t raw_obj_x = T_screen_x - (m_layer_pos_x_window * 2 / 3);
		int64_t raw_obj_y = T_screen_y - (m_layer_pos_y_window * 2 / 3);

		int64_t obj_max_x = layer_w - mxx;
		int64_t obj_max_y = layer_h - mxy;
		int64_t obj_min_x = -mnx;
		int64_t obj_min_y = -mny;
		m_obj_offset_x_screen = std::clamp(raw_obj_x, obj_min_x, obj_max_x >= obj_min_x ? obj_max_x : obj_min_x);
		m_obj_offset_y_screen = std::clamp(raw_obj_y, obj_min_y, obj_max_y >= obj_min_y ? obj_max_y : obj_min_y);

		reachedMaxX = (m_obj_offset_x_screen + mxx >= layer_w);
		reachedMaxY = (m_obj_offset_y_screen + mxy >= layer_h);
		tsl::gfx::Renderer::get().setLayerPos((uint32_t)m_layer_pos_x_window, (uint32_t)m_layer_pos_y_window);
	};

	bool m_touchScreen = touchScreen;
	bool m_motionControl = motionControl;
	if (Movable == true) {
		if ((ClampToLayerRight || ClampToLayerBottom) && !changingPos && haveBounds) {
			int64_t mxx = s_rects[0].x + s_rects[0].w;
			int64_t mxy = s_rects[0].y + s_rects[0].h;
			for (const auto& r : s_rects) {
				if (r.x + r.w > mxx) mxx = r.x + r.w;
				if (r.y + r.h > mxy) mxy = r.y + r.h;
			}
			int64_t layer_w = (int64_t)tsl::cfg::FramebufferWidth;
			int64_t layer_h = (int64_t)tsl::cfg::FramebufferHeight;
			bool changed = false;
			if (ClampToLayerRight && reachedMaxX) {
				int64_t target_x = layer_w - mxx;
				if (m_obj_offset_x_screen != target_x) {
					m_obj_offset_x_screen = target_x;
					changed = true;
				}
			}
			if (ClampToLayerBottom && reachedMaxY) {
				int64_t target_y = layer_h - mxy;
				if (m_obj_offset_y_screen != target_y) {
					m_obj_offset_y_screen = target_y;
					changed = true;
				}
			}
			(void)changed;
		}
			constexpr uint64_t kDragHoldNs = 500'000'000ULL;
			constexpr int64_t kTouchDragThresholdPx = 8;
			const uint64_t nowNs = armTicksToNs(svcGetSystemTick());
			const bool touchDetected = m_touchScreen && *touchInput.delta_time != 0;

			if (touchDetected && !sixaxisChangingPos) [[unlikely]] {
				const int64_t currentTouchX = *touchInput.x;
				const int64_t currentTouchY = *touchInput.y;
				if (!m_touchContactActive) {
					m_touchContactActive = true;
					m_touchOriginValid = IsInsideTouchRange(currentTouchX, currentTouchY, 24);
					m_touchHoldStartedNs = m_touchOriginValid ? nowNs : 0;
					m_touchDragActive = false;
					m_touchDragExceededThreshold = false;
				}

				if (m_touchOriginValid && !m_touchDragActive
					&& nowNs - m_touchHoldStartedNs >= kDragHoldNs) {
					m_touchDragActive = true;
					changingPos = true;
					m_touchDragOriginX = currentTouchX;
					m_touchDragOriginY = currentTouchY;
					m_anchor_offset_x = currentTouchX - static_cast<int64_t>(m_base_x);
					m_anchor_offset_y = currentTouchY - static_cast<int64_t>(m_base_y);
					tsl::triggerOnFeedback();
				}

				if (m_touchDragActive) {
					const int64_t deltaX = currentTouchX - m_touchDragOriginX;
					const int64_t deltaY = currentTouchY - m_touchDragOriginY;
					if (!m_touchDragExceededThreshold
						&& std::abs(deltaX) + std::abs(deltaY) >= kTouchDragThresholdPx) {
						m_touchDragExceededThreshold = true;
					}
					if (m_touchDragExceededThreshold) {
						touch_pos_x = currentTouchX;
						touch_pos_y = currentTouchY;
						applyDrag();
					}
				}
			}
			else if (m_touchContactActive) {
				if (m_touchDragActive) {
					if (m_touchDragExceededThreshold) persistMovablePosition();
					tsl::triggerOffFeedback(true);
					changingPos = false;
				}
				m_touchContactActive = false;
				m_touchOriginValid = false;
				m_touchHoldStartedNs = 0;
				m_touchDragActive = false;
				m_touchDragExceededThreshold = false;
				touch_pos_x = -1;
				touch_pos_y = -1;
			}

			const bool plusHeldAlone = (keysHeld & KEY_PLUS)
				&& !(keysHeld & ~KEY_PLUS & ALL_KEYS_MASK);
			if (plusHeldAlone && !sixaxisChangingPos) {
				if (m_joystickHoldStartedNs == 0) m_joystickHoldStartedNs = nowNs;
				if (!m_joystickDragActive && nowNs - m_joystickHoldStartedNs >= kDragHoldNs) {
					m_joystickDragActive = true;
					changingPos = true;
					m_anchor_offset_x = 0;
					m_anchor_offset_y = 0;
					m_joystickDragResidualX = 0.0f;
					m_joystickDragResidualY = 0.0f;
					tsl::triggerOnFeedback();
				}

				if (m_joystickDragActive) {
					const int64_t leftMagnitude = static_cast<int64_t>(leftJoyStick.x) * leftJoyStick.x
						+ static_cast<int64_t>(leftJoyStick.y) * leftJoyStick.y;
					const int64_t rightMagnitude = static_cast<int64_t>(rightJoyStick.x) * rightJoyStick.x
						+ static_cast<int64_t>(rightJoyStick.y) * rightJoyStick.y;
					const auto& activeStick = rightMagnitude > leftMagnitude ? rightJoyStick : leftJoyStick;
					constexpr int kJoystickDeadzone = 20;
					if (std::abs(activeStick.x) > kJoystickDeadzone || std::abs(activeStick.y) > kJoystickDeadzone) {
						const float magnitude = std::sqrt(static_cast<float>(
							static_cast<int64_t>(activeStick.x) * activeStick.x
							+ static_cast<int64_t>(activeStick.y) * activeStick.y
						));
						const float normalizedMagnitude = magnitude / 32767.0f;
						const float sensitivity = 0.00008f
							+ (0.0005f - 0.00008f) * std::pow(normalizedMagnitude, 8.0f);
						m_joystickDragResidualX += static_cast<float>(activeStick.x) * sensitivity;
						m_joystickDragResidualY -= static_cast<float>(activeStick.y) * sensitivity;
						const int64_t deltaX = static_cast<int64_t>(m_joystickDragResidualX);
						const int64_t deltaY = static_cast<int64_t>(m_joystickDragResidualY);
						m_joystickDragResidualX -= static_cast<float>(deltaX);
						m_joystickDragResidualY -= static_cast<float>(deltaY);
						if (deltaX != 0 || deltaY != 0) {
							touch_pos_x = static_cast<int64_t>(m_base_x) + deltaX;
							touch_pos_y = static_cast<int64_t>(m_base_y) + deltaY;
							applyDrag();
						}
					}
				}
			}
			else if (m_joystickDragActive) {
				persistMovablePosition();
				tsl::triggerOffFeedback(true);
				m_joystickDragActive = false;
				m_joystickHoldStartedNs = 0;
				m_joystickDragResidualX = 0.0f;
				m_joystickDragResidualY = 0.0f;
				changingPos = false;
			}
			else {
				m_joystickHoldStartedNs = 0;
			}

			if (m_motionControl == true && !m_touchDragActive && !m_joystickDragActive
				&& (changingPos == false || sixaxisChangingPos == true)) [[unlikely]] {
			HidSixAxisSensorState sixaxis = {0};
			s32 id = -1;
			u64 style_set = hidGetNpadStyleSet(HidNpadIdType_No1);
			if (style_set & HidNpadStyleTag_NpadJoyDual) {
				if ((keysHeld & leftJoyconMotionMappedButtons) == leftJoyconMotionMappedButtons) id = Controller_JoyConL;
				else if ((keysHeld & rightJoyconMotionMappedButtons) == rightJoyconMotionMappedButtons) id = Controller_JoyConR;
			}
			else if (style_set & HidNpadStyleTag_NpadJoyLeft) {
				if ((keysHeld & leftJoyconMotionMappedButtons) == leftJoyconMotionMappedButtons) id = Controller_JoyConL;
			}
			else if (style_set & HidNpadStyleTag_NpadJoyRight) {
				if ((keysHeld & rightJoyconMotionMappedButtons) == rightJoyconMotionMappedButtons) id = Controller_JoyConR;
			}
			else if (style_set & HidNpadStyleTag_NpadFullKey) {
				if ((keysHeld & proControllerMotionMappedButtons) == proControllerMotionMappedButtons) id = Controller_ProController;
			}
			if (id < 0) {
				m_gyro_started = false;
				changingPos = false;
				sixaxisChangingPos = false;
			}
			else {
				static GyroCursor cursor;
				hidGetSixAxisSensorStates(sixaxisHandles[id], &sixaxis, 1);
				if (sixaxis.acceleration.x == 0.f && sixaxis.acceleration.y == 0.f && sixaxis.acceleration.z == -1.f) {
					hidsysSetAppletResourceUserId();
					hidGetSixAxisSensorStates(sixaxisHandles[id], &sixaxis, 1);
				}
				if (sixaxis.acceleration.x != 0.f || sixaxis.acceleration.y != 0.f || sixaxis.acceleration.z != -1.f) {
					if (m_gyro_started == false) {
						m_gyro_started = true;
						float sensitivity = 200;
						gyroCursor_init(cursor, (float)m_base_x, (float)m_base_y, sensitivity);
						m_anchor_offset_x = 0;
						m_anchor_offset_y = 0;
					}
					changingPos = true;
					sixaxisChangingPos = true;
					gyroCursor_update(cursor, sixaxis, &touch_pos_x, &touch_pos_y);
					applyDrag();
				}
				else {
					m_gyro_started = false;
					changingPos = false;
					sixaxisChangingPos = false;
				}
			}
		}
	}
	if (!changingPos) {
		uint64_t new_time = armTicksToNs(svcGetSystemTick());
		do {
			if (Movable == true) {
				if (m_touchScreen == true) {
					HidTouchScreenState state = {0};
					if (hidGetTouchScreenStates(&state, 1) && state.count && IsInsideTouchRange(state.touches[0].x, state.touches[0].y, 24)) {
						break;
					}
				}
				if (m_motionControl == true) {
					u64 style_set = hidGetNpadStyleSet(HidNpadIdType_No1);
					if (style_set & HidNpadStyleTag_NpadJoyDual) {
						if ((keysHeld & leftJoyconMotionMappedButtons) == leftJoyconMotionMappedButtons) break;
						else if ((keysHeld & rightJoyconMotionMappedButtons) == rightJoyconMotionMappedButtons) break;
					}
					else if (style_set & HidNpadStyleTag_NpadJoyLeft) {
						if ((keysHeld & leftJoyconMotionMappedButtons) == leftJoyconMotionMappedButtons) break;
					}
					else if (style_set & HidNpadStyleTag_NpadJoyRight) {
						if ((keysHeld & rightJoyconMotionMappedButtons) == rightJoyconMotionMappedButtons) break;
					}
					else if (style_set & HidNpadStyleTag_NpadFullKey) {
						if ((keysHeld & proControllerMotionMappedButtons) == proControllerMotionMappedButtons) break;
					}
				}
			}
			if (isExitComboHeld(
				keysHeld, mappedButtons,
				UseCustomExitCombo ? static_cast<uint64_t>(keyComboTimeDelay) : 100'000'000
			)) [[unlikely]] {
				tsl::goBack();
				if (m_double_back == true) tsl::goBack();
				return true;
			}
			svcSleepThread(1000000);
			new_time = armTicksToNs(svcGetSystemTick());
		} while (new_time - m_last_time < timeout);
		m_last_time = new_time;
	}
	SystemData.KeysHeld_int = keysHeld;
	SystemData.KeysDown_int = keysDown;
	return false;
}
