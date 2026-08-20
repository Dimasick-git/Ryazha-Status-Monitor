#pragma once
#include <charconv>
#include <vector>
#include <string>
#include <tesla.hpp>
#include "smd_parser.hpp"

inline int64_t COMMON_MARGIN = 0;

class RenderingPipeline : public tsl::Gui {
private:
	bool m_double_back = false;
	uint64_t mappedButtons = HidNpadButton_B;
	uint64_t leftJoyconMotionMappedButtons  = 0;
	uint64_t rightJoyconMotionMappedButtons = 0;
	uint64_t proControllerMotionMappedButtons = 0;
	uint8_t* displayRefreshRate = 0;
	bool UseCustomExitCombo = false;
	std::string name;
	std::string rel_filepath;
	std::string error;
	std::string footerBackup;
	smd::Document doc;
	bool HeaderText = true;
	bool FooterText = true;
	bool Movable = false;
	bool ClampToLayerRight = false;
	bool ClampToLayerBottom = false;
	size_t threads_count = 0;
	Thread threads[6];
	uint64_t timeout = 15'000'000;
	std::string ComboButtonFooter;
	bool changingPos = false;
	bool sixaxisChangingPos = false;
	bool changedPos = false;
	bool reachedMaxX = false;
	bool reachedMaxY = false;
	int64_t touch_pos_x = -1;
	int64_t touch_pos_y = -1;
	uint32_t m_base_x = 0;
	uint32_t m_base_y = 0;
	int64_t m_anchor_offset_x = 0;
	int64_t m_anchor_offset_y = 0;
	int64_t m_saved_base_x = -1;
	int64_t m_saved_base_y = -1;
	uint64_t m_last_time = armTicksToNs(svcGetSystemTick());
	bool m_gyro_started = false;
	u32 m_last_layer_w = tsl::cfg::LayerWidth;
	u32 m_last_layer_h = tsl::cfg::LayerHeight;
	u64 last_tick = 0;
	u32 smd_hash = 0;

	struct TouchRect { int64_t x, y, w, h; };
	static inline std::vector<TouchRect> s_rects{};

	int64_t m_layer_pos_x_window = 0;
	int64_t m_layer_pos_y_window = 0;

	static inline int64_t m_obj_offset_x_screen = 0;
	static inline int64_t m_obj_offset_y_screen = 0;

	static void TrackRect(int64_t x, int64_t y, int64_t w, int64_t h);
	static void RenderGraphLineChart(smd::RenderCommand& cmd, tsl::gfx::Renderer* m_renderer);
	static void PreRecordCallback(std::string& outLocaleCode, void* user);
	static void RecordCallback(smd::RenderCommand& cmd, void* user);
	static void DryRunCallback(smd::RenderCommand& cmd, void* user);
	size_t getFreeHeapMemory() const;

	bool IsInsideTouchRange(int64_t screen_x, int64_t screen_y) const;

public:
	RenderingPipeline(std::string filepath, bool double_back = false);
	~RenderingPipeline();
	virtual tsl::elm::Element* createUI() override;
	virtual void update() override;
	virtual bool handleInput(uint64_t keysDown, uint64_t keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override;
};
