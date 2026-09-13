class MainMenu;

// Lightweight, allocation-free EMA + slew channel used by Full mode. Raw
// telemetry is sampled at a modest rate while the visible value advances every
// frame, matching the fluid counter behaviour used by Ryazha-clk.
class FullSmoothChannel {
public:
    constexpr FullSmoothChannel(float alphaUp, float alphaDown,
                                float slewUp, float slewDown)
        : m_alphaUp(alphaUp), m_alphaDown(alphaDown),
          m_slewUp(slewUp), m_slewDown(slewDown) {}

    void feed(float value) {
        if (!std::isfinite(value)) return;
        m_raw = value;
        m_hasRaw = true;
    }

    void step(float dt) {
        if (!m_hasRaw) return;
        if (!m_initialized) {
            m_ema = m_value = m_raw;
            m_initialized = true;
            return;
        }

        const float alpha = m_raw >= m_ema ? m_alphaUp : m_alphaDown;
        m_ema += alpha * (m_raw - m_ema);
        const float delta = m_ema - m_value;
        const float limit = (delta >= 0.0f ? m_slewUp : m_slewDown) * dt;
        m_value += std::clamp(delta, -limit, limit);
    }

    float value(float fallback = 0.0f) const {
        return m_initialized ? m_value : fallback;
    }

private:
    float m_alphaUp;
    float m_alphaDown;
    float m_slewUp;
    float m_slewDown;
    float m_raw = 0.0f;
    float m_ema = 0.0f;
    float m_value = 0.0f;
    bool m_hasRaw = false;
    bool m_initialized = false;
};

struct FullSmoothBank {
    static constexpr float FREQ_SLEW = 300'000'000.0f;
    FullSmoothChannel cpuTarget{0.55f, 0.45f, FREQ_SLEW, FREQ_SLEW};
    FullSmoothChannel gpuTarget{0.55f, 0.45f, FREQ_SLEW, FREQ_SLEW};
    FullSmoothChannel ramTarget{0.55f, 0.45f, FREQ_SLEW, FREQ_SLEW};
    FullSmoothChannel cpuReal  {0.55f, 0.45f, FREQ_SLEW, FREQ_SLEW};
    FullSmoothChannel gpuReal  {0.55f, 0.45f, FREQ_SLEW, FREQ_SLEW};
    FullSmoothChannel ramReal  {0.55f, 0.45f, FREQ_SLEW, FREQ_SLEW};
    FullSmoothChannel core0{0.30f, 0.20f, 1'000.0f, 1'000.0f};
    FullSmoothChannel core1{0.30f, 0.20f, 1'000.0f, 1'000.0f};
    FullSmoothChannel core2{0.30f, 0.20f, 1'000.0f, 1'000.0f};
    FullSmoothChannel core3{0.30f, 0.20f, 1'000.0f, 1'000.0f};
    FullSmoothChannel gpuLoad {0.30f, 0.20f, 1'000.0f, 1'000.0f};
    FullSmoothChannel ramLoad {0.30f, 0.20f, 1'000.0f, 1'000.0f};
    FullSmoothChannel ramCpu  {0.30f, 0.20f, 1'000.0f, 1'000.0f};
    FullSmoothChannel ramGpu  {0.30f, 0.20f, 1'000.0f, 1'000.0f};
    FullSmoothChannel fan     {0.20f, 0.15f, 1'000.0f, 1'000.0f};
    FullSmoothChannel power   {0.25f, 0.20f, 1'000.0f, 1'000.0f};
    FullSmoothChannel battery {0.10f, 0.10f, 1'000.0f, 1'000.0f};
    FullSmoothChannel pfps    {0.40f, 0.25f, 240.0f, 120.0f};
    FullSmoothChannel fps     {0.40f, 0.25f, 240.0f, 120.0f};
    FullSmoothChannel cpuTemp {0.10f, 0.08f, 12.0f, 6.0f};
    FullSmoothChannel gpuTemp {0.10f, 0.08f, 12.0f, 6.0f};
    FullSmoothChannel ramTemp {0.10f, 0.08f, 12.0f, 6.0f};
    FullSmoothChannel socTemp {0.10f, 0.08f, 12.0f, 6.0f};
    FullSmoothChannel pcbTemp {0.10f, 0.08f, 12.0f, 6.0f};
    FullSmoothChannel skinTemp{0.10f, 0.08f, 12.0f, 6.0f};

    void step(float dt) {
        cpuTarget.step(dt); gpuTarget.step(dt); ramTarget.step(dt);
        cpuReal.step(dt); gpuReal.step(dt); ramReal.step(dt);
        core0.step(dt); core1.step(dt); core2.step(dt); core3.step(dt);
        gpuLoad.step(dt); ramLoad.step(dt); ramCpu.step(dt); ramGpu.step(dt);
        fan.step(dt); power.step(dt); battery.step(dt); pfps.step(dt); fps.step(dt);
        cpuTemp.step(dt); gpuTemp.step(dt); ramTemp.step(dt);
        socTemp.step(dt); pcbTemp.step(dt); skinTemp.step(dt);
    }
};

class FullOverlay : public tsl::Gui {
private:
    char DeltaCPU_c[12] = "";
    char DeltaGPU_c[12] = "";
    char DeltaRAM_c[12] = "";
    char RealCPU_Hz_c[64] = "";
    char RealGPU_Hz_c[64] = "";
    char RealRAM_Hz_c[64] = "";
    char GPU_Load_c[32] = "";
    char Rotation_SpeedLevel_c[64] = "";
    char RAM_compressed_c[64] = "";
    char RAM_var_compressed_c[128] = "";
    char RAM_percentage_var_compressed_c[128] = "";
    char CPU_Hz_c[64] = "";
    char GPU_Hz_c[64] = "";
    char RAM_Hz_c[64] = "";
    char CPU_compressed_c[160] = "";
    char SOC_temperature_c[32] = "";
    char PCB_temperature_c[32] = "";
    char skin_temperature_c[32] = "";
    char CPU_temp_c[16] = "";
    char GPU_temp_c[16] = "";
    char MEM_temp_c[16] = "";
    char BatteryDraw_c[64] = "";
    char FPS_var_compressed_c[64] = "";
    char RAM_load_c[64] = "";
    char RAM_breakdown_c[48] = "";
    char Resolutions_c[64] = "";
    char readSpeed_c[32] = "";
    char BatteryTime_c[8] = "--:--";
    
    // New separated value buffers for CPU cores
    char CPU_Core0_c[16] = "";
    char CPU_Core1_c[16] = "";
    char CPU_Core2_c[16] = "";
    char CPU_Core3_c[16] = "";
    
    // New separated value buffers for FPS
    char PFPS_value_c[16] = "";
    char FPS_value_c[16] = "";

    static constexpr uint8_t COMMON_MARGIN = 20;
    FullSettings settings;
    uint64_t systemtickfrequency_impl = systemtickfrequency;
    std::string formattedKeyCombo = keyCombo;
    std::string message;
    const std::vector<std::string> KEY_SYMBOLS = {
        "\uE0E4", "\uE0E5", "\uE0E6", "\uE0E7",
        "\uE0E8", "\uE0E9", "\uE0ED", "\uE0EB",
        "\uE0EE", "\uE0EC", "\uE0E0", "\uE0E1",
        "\uE0E2", "\uE0E3", "\uE08A", "\uE08B",
        "\uE0B6", "\uE0B5"
    };

    bool skipOnce = true;
    bool runOnce = true;
    u64  lastDataUpdateTick = 0;  // tick of last sensor data format; used to gate updates when frame limiter is off
    u64  lastSmoothFrameTick = 0;
    FullSmoothBank smooth;

    enum class FocusCard : uint8_t { None, CPU, GPU, RAM, Game, Temperatures, System };
    FocusCard focusCard = FocusCard::None;
    bool focusCardOpening = false;
    float focusCardProgress = 0.0f;
    bool focusTouchWasDown = false;
    int focusTouchStartX = 0;
    int focusTouchStartY = 0;
    u64 focusTouchStartNs = 0;
    bool focusTouchMoved = false;
    bool focusSwipeConsumed = false;

    static constexpr int WIDGET_COUNT = 6;
    static constexpr int WIDGET_W = 196;
    static constexpr int WIDGET_H = 82;
    struct WidgetPlacement {
        bool pinned = false;
        int x = 18;
        int y = 112;
        float scale = 1.0f;
    };
    std::array<WidgetPlacement, WIDGET_COUNT> widgets{};
    int layerScreenX = 0;
    int focusCardX = 24;
    int focusCardY = 138;
    float focusCardScale = 1.0f;
    bool focusPinching = false;
    float pinchStartDistance = 0.0f;
    float pinchStartScale = 1.0f;
    float pinchAnchorX = 0.5f;
    float pinchAnchorY = 0.5f;
    FocusCard touchTargetCard = FocusCard::None;
    bool touchTargetsFocusedCard = false;
    bool touchTargetsPinnedCard = false;
    bool touchTargetsPinButton = false;
    int touchStartScreenX = 0;
    int touchStartScreenY = 0;
    int dragStartLayerX = 0;
    int dragStartFocusX = 24;
    int dragStartFocusY = 138;
    int dragStartWidgetX = 0;
    int dragStartWidgetY = 0;

    int widgetCanvasWidth() const {
        return std::max(448, static_cast<int>(tsl::cfg::FramebufferWidth));
    }

    float maxFocusScale() const {
        const float byWidth = static_cast<float>(widgetCanvasWidth() - 16) / 400.0f;
        const float byHeight = static_cast<float>(720 - 16) / 438.0f;
        return std::min({1.55f, byWidth, byHeight});
    }

    float maxWidgetScale() const {
        const float byWidth = static_cast<float>(widgetCanvasWidth() - 16) / static_cast<float>(WIDGET_W);
        const float byHeight = static_cast<float>(720 - 16) / static_cast<float>(WIDGET_H);
        return std::min({2.25f, byWidth, byHeight});
    }

    int scaledWidgetWidth(const WidgetPlacement& widget) const {
        return std::max(1, static_cast<int>(std::lround(WIDGET_W * widget.scale)));
    }

    int scaledWidgetHeight(const WidgetPlacement& widget) const {
        return std::max(1, static_cast<int>(std::lround(WIDGET_H * widget.scale)));
    }

    void clampWidgetPlacement(WidgetPlacement& widget) const {
        const int width = scaledWidgetWidth(widget);
        const int height = scaledWidgetHeight(widget);
        widget.x = std::clamp(widget.x, 8, widgetCanvasWidth() - width - 8);
        widget.y = std::clamp(widget.y, 8, 720 - height - 8);
    }

    static constexpr int cardIndex(FocusCard card) {
        return static_cast<int>(card) - static_cast<int>(FocusCard::CPU);
    }

    static FocusCard cardFromIndex(int index) {
        return static_cast<FocusCard>(index + static_cast<int>(FocusCard::CPU));
    }

    static const char* cardKey(FocusCard card) {
        switch (card) {
            case FocusCard::CPU: return "cpu";
            case FocusCard::GPU: return "gpu";
            case FocusCard::RAM: return "ram";
            case FocusCard::Game: return "game";
            case FocusCard::Temperatures: return "temperatures";
            case FocusCard::System: return "system";
            default: return "none";
        }
    }

    static const char* cardTitle(FocusCard card) {
        switch (card) {
            case FocusCard::CPU: return "CPU";
            case FocusCard::GPU: return "GPU";
            case FocusCard::RAM: return "RAM";
            case FocusCard::Game: return "Game";
            case FocusCard::Temperatures: return "Temperatures";
            case FocusCard::System: return "System";
            default: return "";
        }
    }

    int readWidgetInt(const std::string& key, int fallback) const {
        const std::string value = ult::parseValueFromIniSection(configIniPath, "full-widgets", key);
        if (value.empty()) return fallback;
        char* end = nullptr;
        const long parsed = std::strtol(value.c_str(), &end, 10);
        return (end && end != value.c_str()) ? static_cast<int>(parsed) : fallback;
    }

    void loadWidgetLayout() {
        const int canvasWidth = widgetCanvasWidth();
        for (int i = 0; i < WIDGET_COUNT; ++i) {
            const FocusCard card = cardFromIndex(i);
            const std::string prefix = cardKey(card);
            widgets[i].scale = std::clamp(readWidgetInt(prefix + "_scale", 100) / 100.0f,
                                          0.60f, maxWidgetScale());
            widgets[i].x = readWidgetInt(prefix + "_x", 18 + (i % 2) * 214);
            widgets[i].y = readWidgetInt(prefix + "_y", 108 + (i / 2) * 94);
            clampWidgetPlacement(widgets[i]);
            widgets[i].pinned = ult::parseValueFromIniSection(
                configIniPath, "full-widgets", prefix + "_pinned") == "true";
        }
        // A 1280-wide Full framebuffer is itself fixed at screen X=0. The
        // original 448-wide fallback can still be edge-aligned as before.
        layerScreenX = canvasWidth > 448 ? 0 : (settings.setPosRight ? 832 : 0);
        focusCardScale = std::clamp(readWidgetInt("focus_scale", 100) / 100.0f, 0.60f, maxFocusScale());
        const int focusW = static_cast<int>(std::lround(400.0f * focusCardScale));
        const int focusH = static_cast<int>(std::lround(438.0f * focusCardScale));
        focusCardX = std::clamp(readWidgetInt("focus_x", 24), 8, canvasWidth - focusW - 8);
        focusCardY = std::clamp(readWidgetInt("focus_y", 138), 8, 720 - focusH - 8);
    }

    void saveWidget(FocusCard card) {
        const int index = cardIndex(card);
        if (index < 0 || index >= WIDGET_COUNT) return;
        const std::string prefix = cardKey(card);
        ult::setIniFileValue(configIniPath, "full-widgets", prefix + "_pinned", widgets[index].pinned ? "true" : "false");
        ult::setIniFileValue(configIniPath, "full-widgets", prefix + "_x", std::to_string(widgets[index].x));
        ult::setIniFileValue(configIniPath, "full-widgets", prefix + "_y", std::to_string(widgets[index].y));
        ult::setIniFileValue(configIniPath, "full-widgets", prefix + "_scale",
                             std::to_string(static_cast<int>(std::lround(widgets[index].scale * 100.0f))));
    }

    void saveFreePosition() {
        ult::setIniFileValue(configIniPath, "full-widgets", "focus_x", std::to_string(focusCardX));
        ult::setIniFileValue(configIniPath, "full-widgets", "focus_y", std::to_string(focusCardY));
        ult::setIniFileValue(configIniPath, "full-widgets", "focus_scale",
                             std::to_string(static_cast<int>(std::lround(focusCardScale * 100.0f))));
    }

    bool isPinned(FocusCard card) const {
        const int index = cardIndex(card);
        return index >= 0 && index < WIDGET_COUNT && widgets[index].pinned;
    }

    void togglePinned(FocusCard card) {
        const int index = cardIndex(card);
        if (index < 0 || index >= WIDGET_COUNT) return;
        widgets[index].pinned = !widgets[index].pinned;
        saveWidget(card);
        widgets[index].pinned ? triggerOnFeedback() : triggerOffFeedback(true);
    }

    FocusCard hitPinnedWidget(int x, int y) const {
        for (int i = WIDGET_COUNT - 1; i >= 0; --i) {
            const auto& widget = widgets[i];
            if (widget.pinned && x >= widget.x && x < widget.x + scaledWidgetWidth(widget) &&
                y >= widget.y && y < widget.y + scaledWidgetHeight(widget))
                return cardFromIndex(i);
        }
        return FocusCard::None;
    }

    void drawPinnedWidget(tsl::gfx::Renderer* renderer, FocusCard card,
                          const WidgetPlacement& widget) const {
        const tsl::Color panelFill(settings.focusBackgroundColor);
        const tsl::Color titleColor(settings.catColor1);
        const tsl::Color labelColor(settings.catColor2);
        const tsl::Color textColor(settings.textColor);
        const tsl::Color accentColor(settings.separatorColor);
        char primary[64] = "--";
        char secondary[80] = "";

        switch (card) {
            case FocusCard::CPU:
                std::snprintf(primary, sizeof(primary), "%s", RealCPU_Hz_c[0] ? RealCPU_Hz_c : CPU_Hz_c);
                std::snprintf(secondary, sizeof(secondary), "0 %s  1 %s", CPU_Core0_c, CPU_Core1_c);
                break;
            case FocusCard::GPU:
                std::snprintf(primary, sizeof(primary), "%s", RealGPU_Hz_c[0] ? RealGPU_Hz_c : GPU_Hz_c);
                std::snprintf(secondary, sizeof(secondary), "Load %s", GPU_Load_c);
                break;
            case FocusCard::RAM:
                std::snprintf(primary, sizeof(primary), "%s", RealRAM_Hz_c[0] ? RealRAM_Hz_c : RAM_Hz_c);
                std::snprintf(secondary, sizeof(secondary), "Load %s", RAM_load_c);
                break;
            case FocusCard::Game:
                if (GameRunning) {
                    std::snprintf(primary, sizeof(primary), "%s FPS", FPS_value_c);
                    std::snprintf(secondary, sizeof(secondary), "PFPS %s  %s", PFPS_value_c, Resolutions_c);
                } else {
                    std::snprintf(primary, sizeof(primary), "%s", "No game");
                }
                break;
            case FocusCard::Temperatures:
                std::snprintf(primary, sizeof(primary), "CPU %s", CPU_temp_c);
                std::snprintf(secondary, sizeof(secondary), "GPU %s  RAM %s", GPU_temp_c, MEM_temp_c);
                break;
            case FocusCard::System:
                std::snprintf(primary, sizeof(primary), "%.1f W  %.0f%%",
                              smooth.power.value(), smooth.battery.value());
                std::snprintf(secondary, sizeof(secondary), "Fan %s", Rotation_SpeedLevel_c);
                break;
            default: break;
        }

        const auto sp = [&widget](int value) {
            return std::max(1, static_cast<int>(std::lround(value * widget.scale)));
        };
        const auto sf = [&widget](int value) {
            return std::max(7, static_cast<int>(std::lround(value * widget.scale)));
        };
        const int width = scaledWidgetWidth(widget);
        const int height = scaledWidgetHeight(widget);

        renderer->drawRoundedRectSingleThreaded(widget.x, widget.y, width, height, sp(12), accentColor);
        renderer->drawRoundedRectSingleThreaded(widget.x + sp(1), widget.y + sp(1),
                                                width - sp(2), height - sp(2), sp(11), panelFill);
        renderer->drawRoundedRectSingleThreaded(widget.x + sp(11), widget.y + sp(10),
                                                sp(3), sp(15), sp(2), accentColor);
        renderer->drawString(cardTitle(card), false, widget.x + sp(21), widget.y + sp(22), sf(12), titleColor);
        renderer->drawRoundedRectSingleThreaded(widget.x + width - sp(17), widget.y + sp(11),
                                                sp(6), sp(6), sp(3), accentColor);
        renderer->drawString(primary, false, widget.x + sp(12), widget.y + sp(49), sf(16), textColor);
        if (secondary[0])
            renderer->drawString(secondary, false, widget.x + sp(12), widget.y + sp(69), sf(9), labelColor);
        renderer->drawRoundedRectSingleThreaded(widget.x + width - sp(28),
                                                widget.y + height - sp(9), sp(18), sp(2), sp(1), labelColor);
    }

    void setFreeLayerX(int x) {
        if (widgetCanvasWidth() > 448) {
            layerScreenX = 0;
            ult::useRightAlignment = false;
            ult::layerEdge = 0;
            tsl::gfx::Renderer::get().setLayerPos(0, 0);
            settings.setPosRight = false;
            return;
        }
        layerScreenX = std::clamp(x, 0, 832);
        ult::useRightAlignment = false;
        ult::layerEdge = layerScreenX;
        tsl::gfx::Renderer::get().setLayerPos(static_cast<u32>(layerScreenX), 0);
        settings.setPosRight = layerScreenX >= 416;
    }

    void clampFocusedCard() {
        const int width = static_cast<int>(std::lround(400.0f * focusCardScale));
        const int height = static_cast<int>(std::lround(438.0f * focusCardScale));
        focusCardX = std::clamp(focusCardX, 8, widgetCanvasWidth() - width - 8);
        focusCardY = std::clamp(focusCardY, 8, 720 - height - 8);
    }

    // Full's Plus-hold positioning poller. fullSwipeExitEvent is a global
    // (zero-initialized
    // like threadexit) - using a class member LEvent without leventCreate
    // causes uninitialized handle corruption on the 2nd/3rd open when heap
    // memory is reused with stale content.
    // plusFocusActive is set by the poll thread while Plus is held long enough
    // to activate focus/reposition mode; cleared by the poll thread on release.
    std::atomic<bool> plusFocusActive{false};   // true while Plus-hold focus mode is live
    bool plusFocusWasActive = false;             // edge-detect: was active last handleInput frame
    bool focusClearOnRelease = false;            // deferred frame-limiter re-enable after focus ends
    Thread swipePollThread;

    bool originalUseRightAlignment = ult::useRightAlignment;
    tsl::Color originalBackgroundColor = tsl::defaultBackgroundColor;

    // Poll thread: wakes every ~32 ms via leventWait. leventWait
    // returns true when fullSwipeExitEvent is signalled -> thread exits immediately.
    // Plus-hold focus mode: sets plusFocusActive after a configurable hold
    // ("Button Move Delay", default 1000 ms); cleared on release.
    // While active, handleInput switches to focusBackgroundColor and handles joystick flips.
    static void swipePollFunc(void* arg) {
        auto* self = static_cast<FullOverlay*>(arg);

        static constexpr u64 POLL_NS          = 32'000'000ULL;   // 32 ms sleep / exit check
        // Plus-hold threshold is user-configurable ("Button Move Delay").
        // settings is fully populated by GetConfigSettings() in the constructor,
        // which runs before this thread is created, so reading it here is safe.
        // Note: there is no touch_move_delay for this mode — touch repositioning
        // is a swipe gesture, not a press-and-hold.
        const u64 PLUS_HOLD_NS = (u64)self->settings.buttonMoveDelayMs * 1'000'000ULL;

        // HID setup - same as Micro's touch poll thread: allow P1 + Handheld.
        const HidNpadIdType id_list[2] = { HidNpadIdType_No1, HidNpadIdType_Handheld };
        hidSetSupportedNpadIdType(id_list, 2);
        padConfigureInput(2, HidNpadStyleSet_NpadStandard | HidNpadStyleTag_NpadSystemExt);
        PadState pad_p1;
        PadState pad_handheld;
        padInitialize(&pad_p1,       HidNpadIdType_No1);
        padInitialize(&pad_handheld, HidNpadIdType_Handheld);

        u64  plusHoldStart        = 0;

        do {
            // Sleep gate: swipe and button input are impossible while the
            // system is sleeping and the display is off. Block here until
            // wake; the while condition checks fullSwipeExitEvent on resume
            // so shutdown during sleep still exits cleanly within one POLL_NS.
            if (tsl::hlp::waitWhileSleeping(POLL_NS)) continue;

            const u64 nowNs = armTicksToNs(armGetSystemTick());

            // -- Plus-hold focus mode ------------------------------------------
            padUpdate(&pad_p1);
            padUpdate(&pad_handheld);
            const u64 keysHeld = padGetButtons(&pad_p1) | padGetButtons(&pad_handheld);
            const bool plusOnly = (keysHeld & KEY_PLUS) && !(keysHeld & ~KEY_PLUS & ALL_KEYS_MASK);

            if (plusOnly) {
                if (plusHoldStart == 0) plusHoldStart = nowNs;
                if (!self->plusFocusActive.load(std::memory_order_acquire) &&
                    (nowNs - plusHoldStart) >= PLUS_HOLD_NS) {
                    // Hold threshold reached - activate focus mode and stop the
                    // frame limiter so handleInput can render at full speed.
                    self->plusFocusActive.store(true, std::memory_order_release);
                    if (isRendering) {
                        isRendering = false;
                        leventSignal(&renderingStopEvent);
                    }
                    triggerOnFeedback();
                }
            } else {
                if (self->plusFocusActive.load(std::memory_order_acquire)) {
                    // Plus released while focus mode was active - deactivate.
                    self->plusFocusActive.store(false, std::memory_order_release);
                    triggerOffFeedback(true);
                }
                plusHoldStart = 0;
            }

        } while (!leventWait(&fullSwipeExitEvent, POLL_NS));
    }

    // Applies a left/right reposition: updates settings, persists to the ini,
    // flips alignment, and moves the VI layer. No-op if already at the
    // requested side. Shared by both the joystick-flip and swipe-flip paths
    // in handleInput (always called from the main thread).
    void applyPositionFlip(bool wantRight) {
        // The wide Full canvas must stay at screen X=0; only its cards move.
        // Moving this layer would move and clip the complete dashboard again.
        if (widgetCanvasWidth() > 448) return;
        const int targetX = wantRight ? 832 : 0;
        if (layerScreenX == targetX) return;
        settings.setPosRight = wantRight;
        ult::setIniFileValue(configIniPath, "full", "layer_width_align", wantRight ? "right" : "left");
        setFreeLayerX(targetX);
        saveFreePosition();
        // Note: callers are responsible for triggering move feedback.
        // The swipe path already fires it in the poll thread; the joystick
        // path calls triggerMoveFeedback() explicitly after applyPositionFlip.
    }

public:
    FullOverlay() { 
        fullPanelHidden = false;
        disableJumpTo = true;
        GetConfigSettings(&settings);
        loadWidgetLayout();
        if (settings.useRyazhaTheme) {
            // Theme resources are still initialized for Ryazha styling, but the
            // Full-mode colour settings remain authoritative.  Previously this
            // block overwrote every selected colour after config loading.
            tsl::initializeTheme();
            tsl::initializeThemeVars();
        }
        tsl::defaultBackgroundColor = tsl::Color(settings.backgroundColor); // apply Full's bg color to the tesla draw path
        mutexInit(&mutex_BatteryChecker);
        mutexInit(&mutex_Misc);
        tsl::hlp::requestForeground(false);
        TeslaFPS = settings.refreshRate;
        systemtickfrequency_impl /= settings.refreshRate;
        idletick0.store(systemtickfrequency_impl, std::memory_order_relaxed);
        idletick1.store(systemtickfrequency_impl, std::memory_order_relaxed);
        idletick2.store(systemtickfrequency_impl, std::memory_order_relaxed);
        idletick3.store(systemtickfrequency_impl, std::memory_order_relaxed);
        setFreeLayerX(layerScreenX);
        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().removeScreenshotStacks();
        }
        deactivateOriginalFooter = true;
        formatButtonCombination(formattedKeyCombo);
        //message = "Press " + formattedKeyCombo + " to Exit";

        realVoltsPolling = false;
        StartThreads();

        // Start swipe-to-flip poll thread.
        // leventClear ensures the exit event starts non-signalled before threadStart.
        leventClear(&fullSwipeExitEvent);
        threadCreate(&swipePollThread, swipePollFunc, this, nullptr, 0x1000, 0x2c, -2);
        threadStart(&swipePollThread);
    }
    ~FullOverlay() {
        fullPanelHidden = false;
        leventSignal(&fullSwipeExitEvent);
        threadWaitForExit(&swipePollThread);
        threadClose(&swipePollThread);
        CloseThreads();
        fixForeground = true;
        ult::useRightAlignment = originalUseRightAlignment;
        tsl::defaultBackgroundColor = originalBackgroundColor; // restore for non-full modes
        if (settings.disableScreenshots) {
            tsl::gfx::Renderer::get().addScreenshotStacks();
        }
        deactivateOriginalFooter = false;
    }

    resolutionCalls m_resolutionRenderCalls[8] = {0};
    resolutionCalls m_resolutionViewportCalls[8] = {0};
    resolutionCalls m_resolutionOutput[8] = {0};
    uint8_t resolutionLookup = 0;

    virtual tsl::elm::Element* createUI() override {
        auto Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
            static const std::vector<std::string> specialChars = {""};
            constexpr int outerX = 16;
            constexpr int outerW = 416;
            constexpr int gap = 8;
            constexpr int cardW = (outerW - gap * 2) / 3;
            constexpr int topY = 104;
            constexpr int topH = 174;
            constexpr int gameY = 286;
            constexpr int gameH = 82;
            constexpr int thermalY = 376;
            constexpr int thermalH = 102;
            constexpr int systemY = 486;
            constexpr int systemH = 132;

            const tsl::Color panelFill(settings.focusBackgroundColor);
            const tsl::Color titleColor(settings.catColor1);
            const tsl::Color labelColor(settings.catColor2);
            const tsl::Color textColor(settings.textColor);
            const tsl::Color accentColor(settings.separatorColor);
            const tsl::Color meterTrack(settings.backgroundColor);

            if (fullPanelHidden) {
                for (int i = 0; i < WIDGET_COUNT; ++i) {
                    if (widgets[i].pinned)
                        drawPinnedWidget(renderer, cardFromIndex(i), widgets[i]);
                }
                return;
            }

            const auto drawPanel = [&](int px, int py, int pw, int ph, const char* title) {
                renderer->drawRoundedRectSingleThreaded(px, py, pw, ph, 13, panelFill);
                renderer->drawRoundedRectSingleThreaded(px + 10, py + 10, 3, 17, 2, accentColor);
                renderer->drawString(title, false, px + 20, py + 24, 14, titleColor);
            };
            const auto drawMeter = [&](int mx, int my, int mw, float percent, tsl::Color color) {
                renderer->drawRoundedRectSingleThreaded(mx, my, mw, 5, 3, meterTrack);
                const int fill = std::clamp(static_cast<int>(std::lround(mw * percent / 100.0f)), 0, mw);
                if (fill > 0)
                    renderer->drawRoundedRectSingleThreaded(mx, my, fill, 5, 3, color);
            };
            const auto firstLine = [](const char* value) {
                std::string compact(value ? value : "--");
                const auto newline = compact.find('\n');
                if (newline != std::string::npos) compact.resize(newline);
                return compact;
            };
            const auto primaryFrequency = [](bool showReal, bool realAvailable, bool showTarget,
                                             const char* realValue, const char* targetValue) {
                if (showReal && realAvailable && realValue && realValue[0]) return realValue;
                return showTarget ? targetValue : "--";
            };

            const int cpuX = outerX;
            const int gpuX = outerX + cardW + gap;
            const int ramX = outerX + (cardW + gap) * 2;

            // Performance overview: three quiet cards, one clear primary value
            // per component, and secondary information below it.
            drawPanel(cpuX, topY, cardW, topH, "CPU");
            if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck)) {
                renderer->drawString(primaryFrequency(settings.showRealFreqs, realCPU_Hz != 0,
                                                       settings.showTargetFreqs, RealCPU_Hz_c, CPU_Hz_c),
                                     false, cpuX + 10, topY + 53, 16, textColor);
                if (settings.showTargetFreqs) {
                    renderer->drawString("Target", false, cpuX + 10, topY + 73, 10, labelColor);
                    renderer->drawString(CPU_Hz_c, false, cpuX + 50, topY + 73, 10, textColor);
                }
                if (settings.showDeltas && settings.showRealFreqs && settings.showTargetFreqs && realCPU_Hz)
                    renderer->drawString(DeltaCPU_c, false, cpuX + 10, topY + 91, 10, labelColor);

                const char* coreLabels[4] = {"0", "1", "2", "3"};
                const char* coreValues[4] = {CPU_Core0_c, CPU_Core1_c, CPU_Core2_c, CPU_Core3_c};
                const float coreLoads[4] = {smooth.core0.value(), smooth.core1.value(), smooth.core2.value(), smooth.core3.value()};
                for (int i = 0; i < 4; ++i) {
                    const int rowY = topY + 112 + i * 14;
                    renderer->drawString(coreLabels[i], false, cpuX + 10, rowY, 9, labelColor);
                    drawMeter(cpuX + 23, rowY - 5, 55, coreLoads[i], accentColor);
                    renderer->drawString(coreValues[i], false, cpuX + 84, rowY, 9, textColor);
                }
            } else {
                renderer->drawString("--", false, cpuX + 10, topY + 53, 16, textColor);
            }

            drawPanel(gpuX, topY, cardW, topH, "GPU");
            if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck) || R_SUCCEEDED(nvCheck)) {
                renderer->drawString(primaryFrequency(settings.showRealFreqs, realGPU_Hz != 0,
                                                       settings.showTargetFreqs, RealGPU_Hz_c, GPU_Hz_c),
                                     false, gpuX + 10, topY + 53, 16, textColor);
                if (settings.showTargetFreqs) {
                    renderer->drawString("Target", false, gpuX + 10, topY + 73, 10, labelColor);
                    renderer->drawString(GPU_Hz_c, false, gpuX + 50, topY + 73, 10, textColor);
                }
                if (settings.showDeltas && settings.showRealFreqs && settings.showTargetFreqs && realGPU_Hz)
                    renderer->drawString(DeltaGPU_c, false, gpuX + 10, topY + 91, 10, labelColor);
                renderer->drawString("Load", false, gpuX + 10, topY + 120, 11, labelColor);
                renderer->drawString(GPU_Load_c, false, gpuX + 86, topY + 120, 11, textColor);
                drawMeter(gpuX + 10, topY + 132, cardW - 20, smooth.gpuLoad.value(), accentColor);
            } else {
                renderer->drawString("--", false, gpuX + 10, topY + 53, 16, textColor);
            }

            drawPanel(ramX, topY, cardW, topH, "RAM");
            if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck) || R_SUCCEEDED(Hinted)) {
                renderer->drawString(primaryFrequency(settings.showRealFreqs, realRAM_Hz != 0,
                                                       settings.showTargetFreqs, RealRAM_Hz_c, RAM_Hz_c),
                                     false, ramX + 10, topY + 53, 16, textColor);
                if (settings.showTargetFreqs) {
                    renderer->drawString("Target", false, ramX + 10, topY + 73, 10, labelColor);
                    renderer->drawString(RAM_Hz_c, false, ramX + 50, topY + 73, 10, textColor);
                }
                renderer->drawString("Load", false, ramX + 10, topY + 101, 11, labelColor);
                renderer->drawString(RAM_load_c, false, ramX + 10, topY + 119, 10, textColor);
                drawMeter(ramX + 10, topY + 131, cardW - 20, smooth.ramLoad.value(), accentColor);
                const std::string totalRam = firstLine(RAM_var_compressed_c);
                if (!totalRam.empty())
                    renderer->drawString(totalRam, false, ramX + 10, topY + 155, 9, labelColor);
            } else {
                renderer->drawString("--", false, ramX + 10, topY + 53, 16, textColor);
            }

            drawPanel(outerX, gameY, outerW, gameH, "Game");
            if (GameRunning) {
                if (settings.showFPS) {
                    renderer->drawString("PFPS", false, outerX + 18, gameY + 50, 10, labelColor);
                    renderer->drawString(PFPS_value_c, false, outerX + 55, gameY + 53, 22, textColor);
                    renderer->drawString("FPS", false, outerX + 114, gameY + 50, 10, labelColor);
                    renderer->drawString(FPS_value_c, false, outerX + 145, gameY + 53, 22, textColor);
                }
                if (settings.showRES && NxFps && SharedMemoryUsed && (NxFps->API >= 1)) {
                    renderer->drawString("Resolution", false, outerX + 220, gameY + 43, 10, labelColor);
                    renderer->drawString(Resolutions_c, false, outerX + 220, gameY + 61, 12, textColor);
                }
                if (settings.showRDSD) {
                    renderer->drawString("Read", false, outerX + 332, gameY + 43, 10, labelColor);
                    renderer->drawString(readSpeed_c, false, outerX + 332, gameY + 61, 11, textColor);
                }
            } else {
                renderer->drawString("Game is not running\nor it's incompatible.", false,
                                     outerX + 20, gameY + 53, 13, labelColor);
            }

            drawPanel(outerX, thermalY, outerW, thermalH, "Temperatures");
            const char* tempLabels[6] = {"CPU", "GPU", "RAM", "SoC", "PCB", "Skin"};
            const char* tempValues[6] = {CPU_temp_c, GPU_temp_c, MEM_temp_c,
                                         SOC_temperature_c, PCB_temperature_c, skin_temperature_c};
            const float tempNumbers[6] = {smooth.cpuTemp.value(), smooth.gpuTemp.value(), smooth.ramTemp.value(),
                                          smooth.socTemp.value(), smooth.pcbTemp.value(), smooth.skinTemp.value()};
            constexpr int tempColW = 66;
            for (int i = 0; i < 6; ++i) {
                const int tx = outerX + 13 + i * tempColW;
                renderer->drawString(tempLabels[i], false, tx, thermalY + 55, 10, labelColor);
                const tsl::Color valueColor = settings.useDynamicColors
                    ? tsl::GradientColor(tempNumbers[i], tsl::DEFAULT_TEMP_RANGE_HIGH)
                    : textColor;
                renderer->drawString(tempValues[i], false, tx, thermalY + 78, 12, valueColor);
            }

            drawPanel(outerX, systemY, outerW, systemH, "System");
            renderer->drawString("Battery", false, outerX + 16, systemY + 52, 11, labelColor);
            renderer->drawString(BatteryDraw_c, false, outerX + 16, systemY + 75, 14, textColor);
            drawMeter(outerX + 16, systemY + 86, 174, smooth.battery.value(), accentColor);

            renderer->drawString("Fan", false, outerX + 222, systemY + 52, 11, labelColor);
            renderer->drawString(Rotation_SpeedLevel_c, false, outerX + 222, systemY + 75, 14, textColor);
            drawMeter(outerX + 222, systemY + 86, 174, smooth.fan.value(), accentColor);

            renderer->drawString("Total", false, outerX + 16, systemY + 112, 10, labelColor);
            renderer->drawString(firstLine(RAM_var_compressed_c), false, outerX + 54, systemY + 112, 10, textColor);
            renderer->drawString("Load", false, outerX + 250, systemY + 112, 10, labelColor);
            renderer->drawString(RAM_breakdown_c, false, outerX + 285, systemY + 112, 10, textColor);

            // Pinned telemetry cards share the Full process and its smoothed
            // samples, so adding widgets costs only a handful of draw calls and
            // never starts duplicate polling threads.
            for (int i = 0; i < WIDGET_COUNT; ++i) {
                if (widgets[i].pinned)
                    drawPinnedWidget(renderer, cardFromIndex(i), widgets[i]);
            }

            // Tapping any dashboard block opens a lightweight detail card. Its
            // geometry interpolates from the source block, so it feels attached
            // to the item rather than appearing as a separate page.
            if (focusCard != FocusCard::None && focusCardProgress > 0.001f) {
                int sourceX = outerX, sourceY = topY, sourceW = cardW, sourceH = topH;
                const char* focusTitle = "CPU";
                switch (focusCard) {
                    case FocusCard::CPU: focusTitle = "CPU"; break;
                    case FocusCard::GPU: sourceX = gpuX; focusTitle = "GPU"; break;
                    case FocusCard::RAM: sourceX = ramX; focusTitle = "RAM"; break;
                    case FocusCard::Game: sourceY = gameY; sourceW = outerW; sourceH = gameH; focusTitle = "Game"; break;
                    case FocusCard::Temperatures: sourceY = thermalY; sourceW = outerW; sourceH = thermalH; focusTitle = "Temperatures"; break;
                    case FocusCard::System: sourceY = systemY; sourceW = outerW; sourceH = systemH; focusTitle = "System"; break;
                    default: break;
                }

                const float t = focusCardProgress * focusCardProgress * (3.0f - 2.0f * focusCardProgress);
                const int targetW = static_cast<int>(std::lround(400.0f * focusCardScale));
                const int targetH = static_cast<int>(std::lround(438.0f * focusCardScale));
                const int targetX = std::clamp(focusCardX, 8, widgetCanvasWidth() - targetW - 8);
                const int targetY = std::clamp(focusCardY, 8, 720 - targetH - 8);
                const auto mixInt = [t](int from, int to) {
                    return static_cast<int>(std::lround(from + (to - from) * t));
                };
                const int fx = mixInt(sourceX, targetX);
                const int fy = mixInt(sourceY, targetY);
                const int fw = mixInt(sourceW, targetW);
                const int fh = mixInt(sourceH, targetH);

                renderer->drawRect(0, 97, 448, 548,
                    tsl::Color(0, 0, 0, static_cast<u8>(std::clamp<int>(std::lround(11.0f * t), 0, 11))));
                renderer->drawRoundedRectSingleThreaded(fx, fy, fw, fh, 18, panelFill);
                renderer->drawRoundedRectSingleThreaded(fx + 18, fy + 17, mixInt(3, 5), mixInt(17, 28), 3, accentColor);

                if (focusCardProgress > 0.42f) {
                    const u8 contentAlpha = static_cast<u8>(std::clamp<int>(
                        std::lround((focusCardProgress - 0.42f) / 0.58f * 15.0f), 0, 15));
                    const auto fade = [contentAlpha](tsl::Color color) {
                        color.a = static_cast<u8>(std::min<unsigned int>(color.a, contentAlpha));
                        return color;
                    };
                    const tsl::Color fTitle = fade(titleColor);
                    const tsl::Color fLabel = fade(labelColor);
                    const tsl::Color fText = fade(textColor);
                    const tsl::Color fAccent = fade(accentColor);
                    const auto sx = [fx, this](int offset) { return fx + static_cast<int>(std::lround(offset * focusCardScale)); };
                    const auto sy = [fy, this](int offset) { return fy + static_cast<int>(std::lround(offset * focusCardScale)); };
                    const auto sw = [this](int value) { return std::max(1, static_cast<int>(std::lround(value * focusCardScale))); };
                    const auto sf = [this](int value) { return std::max(8, static_cast<int>(std::lround(value * focusCardScale))); };
                    renderer->drawString(focusTitle, false, sx(34), sy(43), sf(24), fTitle);
                    renderer->drawRoundedRectSingleThreaded(fx + fw - sw(54), sy(15), sw(36), sw(26), sw(8),
                                                            isPinned(focusCard) ? fAccent : fade(meterTrack));
                    renderer->drawString(isPinned(focusCard) ? "ON" : "X", false,
                                         fx + fw - sw(43), sy(34), sf(11), fText);

                    if (focusCard == FocusCard::CPU) {
                        renderer->drawString("Real", false, sx(28), sy(88), sf(13), fLabel);
                        renderer->drawString(RealCPU_Hz_c, false, sx(122), sy(90), sf(23), fText);
                        renderer->drawString("Target", false, sx(28), sy(126), sf(13), fLabel);
                        renderer->drawString(CPU_Hz_c, false, sx(122), sy(128), sf(18), fText);
                        renderer->drawString(DeltaCPU_c, false, sx(28), sy(160), sf(13), fLabel);
                        const char* labels[4] = {"Core 0", "Core 1", "Core 2", "Core 3"};
                        const char* values[4] = {CPU_Core0_c, CPU_Core1_c, CPU_Core2_c, CPU_Core3_c};
                        const float loads[4] = {smooth.core0.value(), smooth.core1.value(), smooth.core2.value(), smooth.core3.value()};
                        for (int i = 0; i < 4; ++i) {
                            const int rowOffset = 211 + i * 49;
                            renderer->drawString(labels[i], false, sx(28), sy(rowOffset), sf(13), fLabel);
                            drawMeter(sx(96), sy(rowOffset - 7), sw(220), loads[i], fAccent);
                            renderer->drawString(values[i], false, sx(329), sy(rowOffset), sf(12), fText);
                        }
                    } else if (focusCard == FocusCard::GPU) {
                        renderer->drawString("Real", false, sx(28), sy(100), sf(13), fLabel);
                        renderer->drawString(RealGPU_Hz_c, false, sx(122), sy(102), sf(25), fText);
                        renderer->drawString("Target", false, sx(28), sy(145), sf(13), fLabel);
                        renderer->drawString(GPU_Hz_c, false, sx(122), sy(147), sf(19), fText);
                        renderer->drawString(DeltaGPU_c, false, sx(28), sy(184), sf(13), fLabel);
                        renderer->drawString("Load", false, sx(28), sy(246), sf(14), fLabel);
                        renderer->drawString(GPU_Load_c, false, sx(302), sy(246), sf(18), fText);
                        drawMeter(sx(28), sy(265), sw(344), smooth.gpuLoad.value(), fAccent);
                    } else if (focusCard == FocusCard::RAM) {
                        renderer->drawString("Real", false, sx(28), sy(100), sf(13), fLabel);
                        renderer->drawString(RealRAM_Hz_c, false, sx(122), sy(102), sf(25), fText);
                        renderer->drawString("Target", false, sx(28), sy(145), sf(13), fLabel);
                        renderer->drawString(RAM_Hz_c, false, sx(122), sy(147), sf(19), fText);
                        renderer->drawString("Load", false, sx(28), sy(212), sf(14), fLabel);
                        renderer->drawString(RAM_load_c, false, sx(302), sy(212), sf(18), fText);
                        drawMeter(sx(28), sy(231), sw(344), smooth.ramLoad.value(), fAccent);
                        renderer->drawString("Total", false, sx(28), sy(285), sf(13), fLabel);
                        renderer->drawString(firstLine(RAM_var_compressed_c), false, sx(122), sy(285), sf(16), fText);
                        renderer->drawString(RAM_breakdown_c, false, sx(28), sy(333), sf(15), fText);
                    } else if (focusCard == FocusCard::Game) {
                        if (GameRunning) {
                            renderer->drawString("PFPS", false, sx(30), sy(102), sf(13), fLabel);
                            renderer->drawString(PFPS_value_c, false, sx(30), sy(157), sf(42), fText);
                            renderer->drawString("FPS", false, sx(168), sy(102), sf(13), fLabel);
                            renderer->drawString(FPS_value_c, false, sx(168), sy(157), sf(42), fText);
                            renderer->drawString("Resolution", false, sx(30), sy(225), sf(13), fLabel);
                            renderer->drawString(Resolutions_c, false, sx(30), sy(254), sf(19), fText);
                            renderer->drawString("Read", false, sx(30), sy(315), sf(13), fLabel);
                            renderer->drawString(readSpeed_c, false, sx(30), sy(344), sf(19), fText);
                        } else {
                            renderer->drawString("Game is not running\nor it's incompatible.", false,
                                                 sx(30), sy(135), sf(18), fLabel);
                        }
                    } else if (focusCard == FocusCard::Temperatures) {
                        const char* labels[6] = {"CPU", "GPU", "RAM", "SoC", "PCB", "Skin"};
                        const char* values[6] = {CPU_temp_c, GPU_temp_c, MEM_temp_c,
                                                 SOC_temperature_c, PCB_temperature_c, skin_temperature_c};
                        const float numbers[6] = {smooth.cpuTemp.value(), smooth.gpuTemp.value(), smooth.ramTemp.value(),
                                                  smooth.socTemp.value(), smooth.pcbTemp.value(), smooth.skinTemp.value()};
                        for (int i = 0; i < 6; ++i) {
                            const int col = i % 3;
                            const int row = i / 3;
                            const int tx = sx(28 + col * 123);
                            const int ty = sy(112 + row * 130);
                            renderer->drawString(labels[i], false, tx, ty, sf(14), fLabel);
                            const tsl::Color tempColor = settings.useDynamicColors
                                ? tsl::GradientColor(numbers[i], tsl::DEFAULT_TEMP_RANGE_HIGH)
                                : fText;
                            renderer->drawString(values[i], false, tx, ty + sw(38), sf(23), fade(tempColor));
                        }
                    } else if (focusCard == FocusCard::System) {
                        renderer->drawString("Battery", false, sx(28), sy(96), sf(14), fLabel);
                        renderer->drawString(BatteryDraw_c, false, sx(28), sy(128), sf(20), fText);
                        drawMeter(sx(28), sy(147), sw(344), smooth.battery.value(), fAccent);
                        renderer->drawString("Fan", false, sx(28), sy(205), sf(14), fLabel);
                        renderer->drawString(Rotation_SpeedLevel_c, false, sx(302), sy(205), sf(18), fText);
                        drawMeter(sx(28), sy(224), sw(344), smooth.fan.value(), fAccent);
                        renderer->drawString("Total", false, sx(28), sy(286), sf(14), fLabel);
                        renderer->drawString(firstLine(RAM_var_compressed_c), false, sx(122), sy(286), sf(17), fText);
                        renderer->drawString("Load", false, sx(28), sy(335), sf(14), fLabel);
                        renderer->drawString(RAM_breakdown_c, false, sx(122), sy(335), sf(16), fText);
                    }
                }
            }

            if (focusCard != FocusCard::None && focusCardProgress > 0.75f)
                renderer->drawString("X Pin · Drag Move · Pinch Resize", false, 24, 645, 11, labelColor);

            renderer->drawString("\uE0B6", false, 24, 665, 14, tsl::buttonColor);
            renderer->drawString("Hide panel", false, 45, 665, 12, labelColor);

            const auto pressWidth = renderer->getTextDimensions("Press ", false, 20).first;
            static const auto keyComboWidth = renderer->getTextDimensions(formattedKeyCombo.c_str(), false, 20).first;
            renderer->drawString("Press ", false, 24, 690, 20, tsl::bottomTextColor);
            renderer->drawStringWithColoredSections(formattedKeyCombo, false, KEY_SYMBOLS, 24 + pressWidth, 690, 20, tsl::bottomTextColor, tsl::buttonColor);
            renderer->drawString(" to Exit", false, 24 + pressWidth + keyComboWidth, 690, 20, tsl::bottomTextColor);
        });

        auto rootFrame = new tsl::elm::HeaderOverlayFrame("Status Monitor", APP_VERSION);
        rootFrame->setContent(Status);
        return rootFrame;
    }

    virtual void update() override {
        // While Plus-hold focus mode is active, swap to the focus background
        // color so the user gets immediate visual feedback even though data
        // formatting below is still throttled to sampleRate. This runs every
        // frame (not gated) since the frame limiter may be unlocked here.
        tsl::defaultBackgroundColor = tsl::Color(
            plusFocusActive.load(std::memory_order_acquire) ? settings.focusBackgroundColor
                                                              : settings.backgroundColor);

        // Throttle data formatting to the user-specified sample rate even when
        // the frame limiter is off (e.g. during Plus-hold focus mode). The render
        // loop may call update() at vsync speed (~60 fps) while focus mode is
        // active, but we only rebuild the display strings at 1/sampleRate intervals.
        const u64 nowTick = armGetSystemTick();
        const u64 frameDeltaTicks = lastSmoothFrameTick ? (nowTick - lastSmoothFrameTick) : 0;
        lastSmoothFrameTick = nowTick;
        float smoothDt = frameDeltaTicks
            ? static_cast<float>(armTicksToNs(frameDeltaTicks)) / 1'000'000'000.0f
            : (1.0f / static_cast<float>(settings.refreshRate));
        smoothDt = std::clamp(smoothDt, 0.0f, 0.25f);
        constexpr float focusAnimationSpeed = 6.5f;
        if (focusCard != FocusCard::None) {
            const float direction = focusCardOpening ? 1.0f : -1.0f;
            focusCardProgress = std::clamp(focusCardProgress + direction * focusAnimationSpeed * smoothDt,
                                           0.0f, 1.0f);
            if (!focusCardOpening && focusCardProgress <= 0.0f)
                focusCard = FocusCard::None;
        }
        const u64 pollIntervalTicks = systemtickfrequency / settings.sampleRate;
        const bool shouldUpdateData = (nowTick - lastDataUpdateTick) >= pollIntervalTicks;
        if (shouldUpdateData) lastDataUpdateTick = nowTick;

        if (shouldUpdateData) {
        //Make stuff ready to print
        ///CPU
        if (systemtickfrequency_impl > 0) {
           const uint64_t idle0_val = std::min(idletick0.load(std::memory_order_acquire), systemtickfrequency_impl);
           const uint64_t idle1_val = std::min(idletick1.load(std::memory_order_acquire), systemtickfrequency_impl);
           const uint64_t idle2_val = std::min(idletick2.load(std::memory_order_acquire), systemtickfrequency_impl);
           const uint64_t idle3_val = std::min(idletick3.load(std::memory_order_acquire), systemtickfrequency_impl);
           
           const float usage0 = std::clamp(100.0f * (1.0f - float(idle0_val) / systemtickfrequency_impl), 0.0f, 100.0f);
           const float usage1 = std::clamp(100.0f * (1.0f - float(idle1_val) / systemtickfrequency_impl), 0.0f, 100.0f);
           const float usage2 = std::clamp(100.0f * (1.0f - float(idle2_val) / systemtickfrequency_impl), 0.0f, 100.0f);
           const float usage3 = std::clamp(100.0f * (1.0f - float(idle3_val) / systemtickfrequency_impl), 0.0f, 100.0f);
           
           // Format individual core values
           snprintf(CPU_Core0_c, sizeof(CPU_Core0_c), "%.2f%%", usage0);
           snprintf(CPU_Core1_c, sizeof(CPU_Core1_c), "%.2f%%", usage1);
           snprintf(CPU_Core2_c, sizeof(CPU_Core2_c), "%.2f%%", usage2);
           snprintf(CPU_Core3_c, sizeof(CPU_Core3_c), "%.2f%%", usage3);
           smooth.core0.feed(usage0);
           smooth.core1.feed(usage1);
           smooth.core2.feed(usage2);
           smooth.core3.feed(usage3);
         }

        mutexLock(&mutex_Misc);
        snprintf(CPU_Hz_c, sizeof(CPU_Hz_c), "%u.%u MHz", CPU_Hz / 1000000, (CPU_Hz / 100000) % 10);
        smooth.cpuTarget.feed(static_cast<float>(CPU_Hz));
        if (realCPU_Hz) {
            smooth.cpuReal.feed(static_cast<float>(realCPU_Hz));
            snprintf(RealCPU_Hz_c, sizeof(RealCPU_Hz_c), "%u.%u MHz", realCPU_Hz / 1000000, (realCPU_Hz / 100000) % 10);
            const int32_t deltaCPU = (int32_t)(realCPU_Hz / 1000) - (CPU_Hz / 1000);
            snprintf(DeltaCPU_c, sizeof(DeltaCPU_c), "Δ %d.%u", deltaCPU / 1000, abs(deltaCPU / 100) % 10);
        }
        
        ///GPU
        snprintf(GPU_Hz_c, sizeof GPU_Hz_c, "%u.%u MHz", GPU_Hz / 1000000, (GPU_Hz / 100000) % 10);
        smooth.gpuTarget.feed(static_cast<float>(GPU_Hz));
        if (realGPU_Hz) {
            smooth.gpuReal.feed(static_cast<float>(realGPU_Hz));
            snprintf(RealGPU_Hz_c, sizeof(RealGPU_Hz_c), "%u.%u MHz", realGPU_Hz / 1000000, (realGPU_Hz / 100000) % 10);
            const int32_t deltaGPU = (int32_t)(realGPU_Hz / 1000) - (GPU_Hz / 1000);
            snprintf(DeltaGPU_c, sizeof(DeltaGPU_c), "Δ %d.%u", deltaGPU / 1000, abs(deltaGPU / 100) % 10);
        }
        snprintf(GPU_Load_c, sizeof GPU_Load_c, "%u.%u%%", GPU_Load_u / 10, GPU_Load_u % 10);
        smooth.gpuLoad.feed(std::clamp(GPU_Load_u / 10.0f, 0.0f, 100.0f));
        
        ///RAM
        snprintf(RAM_Hz_c, sizeof RAM_Hz_c, "%u.%u MHz", RAM_Hz / 1000000, (RAM_Hz / 100000) % 10);
        smooth.ramTarget.feed(static_cast<float>(RAM_Hz));
        if (realRAM_Hz) {
            smooth.ramReal.feed(static_cast<float>(realRAM_Hz));
            snprintf(RealRAM_Hz_c, sizeof(RealRAM_Hz_c), "%u.%u MHz", realRAM_Hz / 1000000, (realRAM_Hz / 100000) % 10);
            const int32_t deltaRAM = (int32_t)(realRAM_Hz / 1000) - (RAM_Hz / 1000);
            snprintf(DeltaRAM_c, sizeof(DeltaRAM_c), "Δ %d.%u", deltaRAM / 1000, abs(deltaRAM / 100) % 10);
        }

        const float RAM_Total_application_f    = (float)RAM_Total_application_u / 1024 / 1024;
        const float RAM_Total_applet_f         = (float)RAM_Total_applet_u / 1024 / 1024;
        const float RAM_Total_system_f         = (float)RAM_Total_system_u / 1024 / 1024;
        const float RAM_Total_systemunsafe_f   = (float)RAM_Total_systemunsafe_u / 1024 / 1024;
        const float RAM_Total_all_f            = RAM_Total_application_f + RAM_Total_applet_f + RAM_Total_system_f + RAM_Total_systemunsafe_f;
        
        const float RAM_Used_application_f     = (float)RAM_Used_application_u / 1024 / 1024;
        const float RAM_Used_applet_f          = (float)RAM_Used_applet_u / 1024 / 1024;
        const float RAM_Used_system_f          = (float)RAM_Used_system_u / 1024 / 1024;
        const float RAM_Used_systemunsafe_f    = (float)RAM_Used_systemunsafe_u / 1024 / 1024;
        const float RAM_Used_all_f             = RAM_Used_application_f + RAM_Used_applet_f + RAM_Used_system_f + RAM_Used_systemunsafe_f;
        
        // Compute percentages
        const auto safePercent = [](float used, float total) -> int {
            return total > 0.0f ? std::clamp(static_cast<int>((used / total) * 100.0f), 0, 100) : 0;
        };
        const int RAMPct_all          = safePercent(RAM_Used_all_f, RAM_Total_all_f);
        const int RAMPct_app          = safePercent(RAM_Used_application_f, RAM_Total_application_f);
        const int RAMPct_applet       = safePercent(RAM_Used_applet_f, RAM_Total_applet_f);
        const int RAMPct_system       = safePercent(RAM_Used_system_f, RAM_Total_system_f);
        const int RAMPct_systemunsafe = safePercent(RAM_Used_systemunsafe_f, RAM_Total_systemunsafe_f);
        
        snprintf(RAM_var_compressed_c, sizeof(RAM_var_compressed_c),
            "%.1f MB / %.1f MB\n"
            "%.1f MB / %.1f MB\n"
            "%.1f MB / %.1f MB\n"
            "%.1f MB / %.1f MB\n"
            "%.1f MB / %.1f MB",
            RAM_Used_all_f,          RAM_Total_all_f,
            RAM_Used_application_f,  RAM_Total_application_f,
            RAM_Used_applet_f,       RAM_Total_applet_f,
            RAM_Used_system_f,       RAM_Total_system_f,
            RAM_Used_systemunsafe_f, RAM_Total_systemunsafe_f
        );
        
        // 2. Percentages only (newlines preserved)
        snprintf(RAM_percentage_var_compressed_c, sizeof(RAM_percentage_var_compressed_c),
            "(%d%%)\n"
            "(%d%%)\n"
            "(%d%%)\n"
            "(%d%%)\n"
            "(%d%%)",
            RAMPct_all,
            RAMPct_app,
            RAMPct_applet,
            RAMPct_system,
            RAMPct_systemunsafe
        );
        
        {
            const int RAM_GPU_Load = (int)ramLoad[SysClkRamLoad_All] - (int)ramLoad[SysClkRamLoad_Cpu];
            const unsigned gpuLoad = (unsigned)(RAM_GPU_Load > 0 ? RAM_GPU_Load : 0);
            smooth.ramLoad.feed(std::clamp(ramLoad[SysClkRamLoad_All] / 10.0f, 0.0f, 100.0f));
            smooth.ramCpu.feed(std::clamp(ramLoad[SysClkRamLoad_Cpu] / 10.0f, 0.0f, 100.0f));
            smooth.ramGpu.feed(std::clamp(gpuLoad / 10.0f, 0.0f, 100.0f));
            snprintf(RAM_load_c, sizeof RAM_load_c, 
                "%u.%u%%    CPU  %u.%u%%   GPU  %u.%u%%",
                ramLoad[SysClkRamLoad_All] / 10, ramLoad[SysClkRamLoad_All] % 10,
                ramLoad[SysClkRamLoad_Cpu] / 10, ramLoad[SysClkRamLoad_Cpu] % 10,
                gpuLoad / 10, gpuLoad % 10);
        }
        ///Thermal
        snprintf(SOC_temperature_c, sizeof SOC_temperature_c, "%.1f\u00B0C", SOC_temperatureF);
        snprintf(PCB_temperature_c, sizeof PCB_temperature_c, "%.1f\u00B0C", PCB_temperatureF);
        snprintf(skin_temperature_c, sizeof skin_temperature_c, "%d.%d\u00B0C", skin_temperaturemiliC / 1000, (skin_temperaturemiliC / 100) % 10);
        // HOC component die temps (always populate; displayed when available)
        snprintf(CPU_temp_c, sizeof CPU_temp_c, "%.1f\u00B0C", componentCPU_mC / 1000.0f);
        snprintf(GPU_temp_c, sizeof GPU_temp_c, "%.1f\u00B0C", componentGPU_mC / 1000.0f);
        snprintf(MEM_temp_c, sizeof MEM_temp_c, "%.1f\u00B0C", componentRAM_mC / 1000.0f);

        const auto feedTemperature = [](FullSmoothChannel& channel, float value) {
            if (std::isfinite(value) && value >= -20.0f && value <= 150.0f)
                channel.feed(value);
        };
        feedTemperature(smooth.cpuTemp, componentCPU_mC / 1000.0f);
        feedTemperature(smooth.gpuTemp, componentGPU_mC / 1000.0f);
        feedTemperature(smooth.ramTemp, componentRAM_mC / 1000.0f);
        feedTemperature(smooth.socTemp, SOC_temperatureF);
        feedTemperature(smooth.pcbTemp, PCB_temperatureF);
        feedTemperature(smooth.skinTemp, skin_temperaturemiliC / 1000.0f);

        snprintf(Rotation_SpeedLevel_c, sizeof Rotation_SpeedLevel_c, "%.1f%%", Rotation_Duty);
        smooth.fan.feed(static_cast<float>(std::clamp(Rotation_Duty, 0.0, 100.0)));
        
        ///FPS
        if (settings.showFPS == true) {
            snprintf(PFPS_value_c, sizeof PFPS_value_c, "%1u", FPS);
            snprintf(FPS_value_c, sizeof FPS_value_c, "%.1f", useOldFPSavg ? FPSavg_old : FPSavg);
            if (GameRunning && FPS < 240) smooth.pfps.feed(static_cast<float>(FPS));
            const float averageFps = useOldFPSavg ? FPSavg_old : FPSavg;
            if (GameRunning && std::isfinite(averageFps) && averageFps >= 0.0f && averageFps < 240.0f)
                smooth.fps.feed(averageFps);
        }

        //Resolutions
        if ((settings.showRES == true) && GameRunning && NxFps) {
            if (!resolutionLookup) {
                (NxFps -> renderCalls[0].calls) = 0xFFFF;
                resolutionLookup = 1;
            }
            else if (resolutionLookup == 1) {
                if ((NxFps -> renderCalls[0].calls) != 0xFFFF) resolutionLookup = 2;
            }
            else {
                if (NxFps && SharedMemoryUsed) {
                    memcpy(&m_resolutionRenderCalls, &(NxFps -> renderCalls), sizeof(m_resolutionRenderCalls));
                    memcpy(&m_resolutionViewportCalls, &(NxFps -> viewportCalls), sizeof(m_resolutionViewportCalls));
                } else {
                    memset(&m_resolutionRenderCalls, 0, sizeof(m_resolutionRenderCalls));
                    memset(&m_resolutionViewportCalls, 0, sizeof(m_resolutionViewportCalls));
                }
                qsort(m_resolutionRenderCalls, 8, sizeof(resolutionCalls), compare);
                qsort(m_resolutionViewportCalls, 8, sizeof(resolutionCalls), compare);
                memset(&m_resolutionOutput, 0, sizeof(m_resolutionOutput));
                size_t out_iter = 0;
                bool found = false;
                for (size_t i = 0; i < 8; i++) {
                    for (size_t x = 0; x < 8; x++) {
                        if (m_resolutionRenderCalls[i].width == 0) {
                            break;
                        }
                        if ((m_resolutionRenderCalls[i].width == m_resolutionViewportCalls[x].width) && (m_resolutionRenderCalls[i].height == m_resolutionViewportCalls[x].height)) {
                            m_resolutionOutput[out_iter].width = m_resolutionRenderCalls[i].width;
                            m_resolutionOutput[out_iter].height = m_resolutionRenderCalls[i].height;
                            m_resolutionOutput[out_iter].calls = (m_resolutionRenderCalls[i].calls > m_resolutionViewportCalls[x].calls) ? m_resolutionRenderCalls[i].calls : m_resolutionViewportCalls[x].calls;
                            out_iter++;
                            found = true;
                            break;
                        }
                    }
                    if (!found && m_resolutionRenderCalls[i].width != 0) {
                        m_resolutionOutput[out_iter].width = m_resolutionRenderCalls[i].width;
                        m_resolutionOutput[out_iter].height = m_resolutionRenderCalls[i].height;
                        m_resolutionOutput[out_iter].calls = m_resolutionRenderCalls[i].calls;
                        out_iter++;
                    }
                    found = false;
                    if (out_iter == 8) break;
                }
                if (out_iter < 8) {
                    const size_t out_iter_s = out_iter;
                    for (size_t x = 0; x < 8; x++) {
                        for (size_t y = 0; y < out_iter_s; y++) {
                            if (m_resolutionViewportCalls[x].width == 0) {
                                break;
                            }
                            if ((m_resolutionViewportCalls[x].width == m_resolutionOutput[y].width) && (m_resolutionViewportCalls[x].height == m_resolutionOutput[y].height)) {
                                found = true;
                                break;
                            }
                        }
                        if (!found && m_resolutionViewportCalls[x].width != 0) {
                            m_resolutionOutput[out_iter].width = m_resolutionViewportCalls[x].width;
                            m_resolutionOutput[out_iter].height = m_resolutionViewportCalls[x].height;
                            m_resolutionOutput[out_iter].calls = m_resolutionViewportCalls[x].calls;
                            out_iter++;         
                        }
                        found = false;
                        if (out_iter == 8) break;
                    }
                }
                qsort(m_resolutionOutput, 8, sizeof(resolutionCalls), compare);
                static std::pair<uint16_t, uint16_t> old_res[2];
                
                // Only swap if BOTH resolutions exist (prevent swapping with empty slot)
                if (m_resolutionOutput[0].width && m_resolutionOutput[1].width) {
                    if ((m_resolutionOutput[0].width == old_res[1].first && m_resolutionOutput[0].height == old_res[1].second) || 
                        (m_resolutionOutput[1].width == old_res[0].first && m_resolutionOutput[1].height == old_res[0].second)) {
                        const uint16_t swap_width = m_resolutionOutput[0].width;
                        const uint16_t swap_height = m_resolutionOutput[0].height;
                        m_resolutionOutput[0].width = m_resolutionOutput[1].width;
                        m_resolutionOutput[0].height = m_resolutionOutput[1].height;
                        m_resolutionOutput[1].width = swap_width;
                        m_resolutionOutput[1].height = swap_height;
                    }
                }
                
                //if (!m_resolutionOutput[1].width) {
                //    snprintf(Resolutions_c, sizeof(Resolutions_c), "%dx%d", m_resolutionOutput[0].width, m_resolutionOutput[0].height);
                //}
                //else {
                //    snprintf(Resolutions_c, sizeof(Resolutions_c), "%dx%d%dx%d", m_resolutionOutput[0].width, m_resolutionOutput[0].height, m_resolutionOutput[1].width, m_resolutionOutput[1].height);
                //}

                if (!m_resolutionOutput[1].width || !m_resolutionOutput[0].width) {
                    if (!m_resolutionOutput[1].width)
                        snprintf(Resolutions_c, sizeof(Resolutions_c), "%dx%d", m_resolutionOutput[0].width, m_resolutionOutput[0].height);
                    else snprintf(Resolutions_c, sizeof(Resolutions_c), "%dx%d", m_resolutionOutput[1].width, m_resolutionOutput[1].height);
                }
                else snprintf(Resolutions_c, sizeof(Resolutions_c),"%dx%d%dx%d", m_resolutionOutput[0].width, m_resolutionOutput[0].height, m_resolutionOutput[1].width, m_resolutionOutput[1].height);
                
                old_res[0] = std::make_pair(m_resolutionOutput[0].width, m_resolutionOutput[0].height);
                old_res[1] = std::make_pair(m_resolutionOutput[1].width, m_resolutionOutput[1].height);
            }
            if (settings.showRDSD == true && GameRunning && NxFps) {
                if ((NxFps -> readSpeedPerSecond) != 0.f) snprintf(readSpeed_c, sizeof(readSpeed_c), "%.2f MiB/s", (NxFps -> readSpeedPerSecond) / 1048576.f);
                else snprintf(readSpeed_c, sizeof(readSpeed_c), "n/d");
            }
        }
        else if (!GameRunning && resolutionLookup != 0) {
            resolutionLookup = 0;
        }

        mutexUnlock(&mutex_Misc);

        /* ── Battery / power draw ───────────────────────────────────── */
        /* Normalise "-0.00" → "0.00" W */
        const float drawW = (fabsf(PowerConsumption) < 0.01f) ? 0.0f
                                                         : PowerConsumption;
        
        mutexLock(&mutex_BatteryChecker);
        
        /* keep "--:--" whenever estimate is negative */
        if (batTimeEstimate >= 0 && !(drawW <= 0.01f && drawW >= -0.01f)) {
            snprintf(BatteryTime_c, sizeof(BatteryTime_c),
                     "%d:%02d", batTimeEstimate / 60, batTimeEstimate % 60);
        } else {
            strcpy(BatteryTime_c, "--:--");
        }
        
        const float batteryPercent = (float)_batteryChargeInfoFields.RawBatteryCharge / 1000.0f;
        
        smooth.power.feed(drawW);
        smooth.battery.feed(std::clamp(batteryPercent, 0.0f, 100.0f));
        
        mutexUnlock(&mutex_BatteryChecker);
        } // end shouldUpdateData

        // Advance the display values at the draw rate and rebuild only the small
        // fixed buffers. This keeps 60 Hz motion without 60 Hz service/IPC reads.
        smooth.step(smoothDt);
        const auto formatFrequency = [](char* dst, size_t size, float hz) {
            const uint32_t value = static_cast<uint32_t>(std::max(0.0f, hz));
            snprintf(dst, size, "%u.%u MHz", value / 1'000'000, (value / 100'000) % 10);
        };
        formatFrequency(CPU_Hz_c, sizeof(CPU_Hz_c), smooth.cpuTarget.value(static_cast<float>(CPU_Hz)));
        formatFrequency(GPU_Hz_c, sizeof(GPU_Hz_c), smooth.gpuTarget.value(static_cast<float>(GPU_Hz)));
        formatFrequency(RAM_Hz_c, sizeof(RAM_Hz_c), smooth.ramTarget.value(static_cast<float>(RAM_Hz)));
        formatFrequency(RealCPU_Hz_c, sizeof(RealCPU_Hz_c), smooth.cpuReal.value(static_cast<float>(realCPU_Hz)));
        formatFrequency(RealGPU_Hz_c, sizeof(RealGPU_Hz_c), smooth.gpuReal.value(static_cast<float>(realGPU_Hz)));
        formatFrequency(RealRAM_Hz_c, sizeof(RealRAM_Hz_c), smooth.ramReal.value(static_cast<float>(realRAM_Hz)));

        const float cpuDelta = (smooth.cpuReal.value(static_cast<float>(realCPU_Hz)) -
                                smooth.cpuTarget.value(static_cast<float>(CPU_Hz))) / 1'000'000.0f;
        const float gpuDelta = (smooth.gpuReal.value(static_cast<float>(realGPU_Hz)) -
                                smooth.gpuTarget.value(static_cast<float>(GPU_Hz))) / 1'000'000.0f;
        snprintf(DeltaCPU_c, sizeof(DeltaCPU_c), "Δ %+.1f", cpuDelta);
        snprintf(DeltaGPU_c, sizeof(DeltaGPU_c), "Δ %+.1f", gpuDelta);
        snprintf(CPU_Core0_c, sizeof(CPU_Core0_c), "%.1f%%", smooth.core0.value());
        snprintf(CPU_Core1_c, sizeof(CPU_Core1_c), "%.1f%%", smooth.core1.value());
        snprintf(CPU_Core2_c, sizeof(CPU_Core2_c), "%.1f%%", smooth.core2.value());
        snprintf(CPU_Core3_c, sizeof(CPU_Core3_c), "%.1f%%", smooth.core3.value());
        snprintf(GPU_Load_c, sizeof(GPU_Load_c), "%.1f%%", smooth.gpuLoad.value());
        snprintf(RAM_load_c, sizeof(RAM_load_c), "%.1f%%", smooth.ramLoad.value());
        snprintf(RAM_breakdown_c, sizeof(RAM_breakdown_c), "CPU %.0f%%  GPU %.0f%%",
                 smooth.ramCpu.value(), smooth.ramGpu.value());
        snprintf(CPU_temp_c, sizeof(CPU_temp_c), "%.1f°C", smooth.cpuTemp.value());
        snprintf(GPU_temp_c, sizeof(GPU_temp_c), "%.1f°C", smooth.gpuTemp.value());
        snprintf(MEM_temp_c, sizeof(MEM_temp_c), "%.1f°C", smooth.ramTemp.value());
        snprintf(SOC_temperature_c, sizeof(SOC_temperature_c), "%.1f°C", smooth.socTemp.value());
        snprintf(PCB_temperature_c, sizeof(PCB_temperature_c), "%.1f°C", smooth.pcbTemp.value());
        snprintf(skin_temperature_c, sizeof(skin_temperature_c), "%.1f°C", smooth.skinTemp.value());
        snprintf(Rotation_SpeedLevel_c, sizeof(Rotation_SpeedLevel_c), "%.1f%%", smooth.fan.value());
        snprintf(PFPS_value_c, sizeof(PFPS_value_c), "%.0f", smooth.pfps.value(static_cast<float>(FPS)));
        snprintf(FPS_value_c, sizeof(FPS_value_c), "%.1f", smooth.fps.value(useOldFPSavg ? FPSavg_old : FPSavg));
        snprintf(BatteryDraw_c, sizeof(BatteryDraw_c), "%.2f W%.0f%% [%s]",
                 smooth.power.value(), smooth.battery.value(), BatteryTime_c);
        
        if (!skipOnce) {
            if (runOnce) {
                if (!(tsl::notification && tsl::notification->isActive())) {
                    isRendering = true;
                    leventClear(&renderingStopEvent);
                } else {
                    wasRendering = true;
                }
                runOnce = false;
            }
        } else {
            skipOnce = false;
        }
    }


    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        // Minus collapses only the Full dashboard. The process, telemetry
        // threads, transparent canvas and pinned cards all stay alive.
        if ((keysDown & KEY_MINUS) && !(keysHeld & ~KEY_MINUS & ALL_KEYS_MASK)) {
            fullPanelHidden = !fullPanelHidden;
            focusCard = FocusCard::None;
            focusCardOpening = false;
            focusCardProgress = 0.0f;
            focusTouchWasDown = false;
            focusPinching = false;
            touchTargetCard = FocusCard::None;
            touchTargetsFocusedCard = false;
            touchTargetsPinnedCard = false;
            touchTargetsPinButton = false;
            fullPanelHidden ? triggerOffFeedback(true) : triggerOnFeedback();
            return true;
        }

        // Tesla owns the only HID sampling pass. Full consumes that complete
        // snapshot: one finger drags, two fingers resize the focused card.
        const u32 touchCount = ult::currentTouchCount.load(std::memory_order_acquire);
        const bool touchDown = touchCount > 0;
        if (touchDown) {
            const int screenX = static_cast<int>(touchPos.x);
            const int screenY = static_cast<int>(touchPos.y);
            const int localX = screenX - layerScreenX;
            const int localY = screenY;
            const bool multiTouch = touchCount > 1;

            if (multiTouch && focusCard != FocusCard::None && focusCardProgress > 0.75f) {
                const HidTouchScreenState& state = ult::currentTouchState;
                const float pinchDx = static_cast<float>(state.touches[1].x) - static_cast<float>(state.touches[0].x);
                const float pinchDy = static_cast<float>(state.touches[1].y) - static_cast<float>(state.touches[0].y);
                const float distance = std::max(1.0f, std::sqrt(pinchDx * pinchDx + pinchDy * pinchDy));
                const float centerX = (static_cast<float>(state.touches[0].x) + static_cast<float>(state.touches[1].x)) * 0.5f - layerScreenX;
                const float centerY = (static_cast<float>(state.touches[0].y) + static_cast<float>(state.touches[1].y)) * 0.5f;

                if (!focusTouchWasDown) {
                    focusTouchWasDown = true;
                    focusTouchStartNs = armTicksToNs(armGetSystemTick());
                }
                if (!focusPinching) {
                    focusPinching = true;
                    pinchStartDistance = distance;
                    pinchStartScale = focusCardScale;
                    const float oldW = 400.0f * focusCardScale;
                    const float oldH = 438.0f * focusCardScale;
                    pinchAnchorX = std::clamp((centerX - focusCardX) / oldW, 0.0f, 1.0f);
                    pinchAnchorY = std::clamp((centerY - focusCardY) / oldH, 0.0f, 1.0f);
                    touchTargetCard = focusCard;
                    touchTargetsFocusedCard = true;
                    touchTargetsPinnedCard = false;
                    touchTargetsPinButton = false;
                }

                focusCardScale = std::clamp(pinchStartScale * (distance / pinchStartDistance),
                                            0.60f, maxFocusScale());
                focusCardX = static_cast<int>(std::lround(centerX - pinchAnchorX * 400.0f * focusCardScale));
                focusCardY = static_cast<int>(std::lround(centerY - pinchAnchorY * 438.0f * focusCardScale));
                clampFocusedCard();
                focusTouchMoved = true;
                focusSwipeConsumed = false;
            } else if (multiTouch && focusCard == FocusCard::None) {
                const HidTouchScreenState& state = ult::currentTouchState;
                const float pinchDx = static_cast<float>(state.touches[1].x) - static_cast<float>(state.touches[0].x);
                const float pinchDy = static_cast<float>(state.touches[1].y) - static_cast<float>(state.touches[0].y);
                const float distance = std::max(1.0f, std::sqrt(pinchDx * pinchDx + pinchDy * pinchDy));
                const float centerX = (static_cast<float>(state.touches[0].x) +
                                       static_cast<float>(state.touches[1].x)) * 0.5f - layerScreenX;
                const float centerY = (static_cast<float>(state.touches[0].y) +
                                       static_cast<float>(state.touches[1].y)) * 0.5f;

                if (!focusTouchWasDown) {
                    focusTouchWasDown = true;
                    focusTouchStartNs = armTicksToNs(armGetSystemTick());
                }

                if (!focusPinching) {
                    FocusCard pinchCard = hitPinnedWidget(
                        static_cast<int>(state.touches[0].x) - layerScreenX,
                        static_cast<int>(state.touches[0].y));
                    if (pinchCard == FocusCard::None) {
                        pinchCard = hitPinnedWidget(
                            static_cast<int>(state.touches[1].x) - layerScreenX,
                            static_cast<int>(state.touches[1].y));
                    }
                    if (pinchCard == FocusCard::None)
                        pinchCard = hitPinnedWidget(static_cast<int>(centerX), static_cast<int>(centerY));

                    if (pinchCard != FocusCard::None) {
                        auto& widget = widgets[cardIndex(pinchCard)];
                        focusPinching = true;
                        pinchStartDistance = distance;
                        pinchStartScale = widget.scale;
                        const float oldW = WIDGET_W * widget.scale;
                        const float oldH = WIDGET_H * widget.scale;
                        pinchAnchorX = std::clamp((centerX - widget.x) / oldW, 0.0f, 1.0f);
                        pinchAnchorY = std::clamp((centerY - widget.y) / oldH, 0.0f, 1.0f);
                        touchTargetCard = pinchCard;
                        touchTargetsFocusedCard = false;
                        touchTargetsPinnedCard = true;
                        touchTargetsPinButton = false;
                    } else {
                        focusSwipeConsumed = true;
                    }
                }

                if (focusPinching && touchTargetsPinnedCard && touchTargetCard != FocusCard::None) {
                    auto& widget = widgets[cardIndex(touchTargetCard)];
                    widget.scale = std::clamp(pinchStartScale * (distance / pinchStartDistance),
                                              0.60f, maxWidgetScale());
                    widget.x = static_cast<int>(std::lround(
                        centerX - pinchAnchorX * WIDGET_W * widget.scale));
                    widget.y = static_cast<int>(std::lround(
                        centerY - pinchAnchorY * WIDGET_H * widget.scale));
                    clampWidgetPlacement(widget);
                    focusTouchMoved = true;
                    focusSwipeConsumed = false;
                }
            } else if (focusPinching) {
                // After a pinch, wait for both fingers to leave. This prevents
                // the remaining contact from becoming a surprise drag or tap.
                focusTouchMoved = true;
            } else if (!focusTouchWasDown) {
                focusTouchWasDown = true;
                focusTouchStartX = localX;
                focusTouchStartY = localY;
                touchStartScreenX = screenX;
                touchStartScreenY = screenY;
                focusTouchStartNs = armTicksToNs(armGetSystemTick());
                focusTouchMoved = multiTouch;
                focusSwipeConsumed = multiTouch;
                touchTargetCard = FocusCard::None;
                touchTargetsFocusedCard = false;
                touchTargetsPinnedCard = false;
                touchTargetsPinButton = false;
                dragStartLayerX = layerScreenX;
                dragStartFocusX = focusCardX;
                dragStartFocusY = focusCardY;

                if (focusCard != FocusCard::None) {
                    touchTargetCard = focusCard;
                    touchTargetsFocusedCard = true;
                    const int focusW = static_cast<int>(std::lround(400.0f * focusCardScale));
                    const int buttonLeft = focusCardX + focusW - static_cast<int>(std::lround(58.0f * focusCardScale));
                    touchTargetsPinButton = localX >= buttonLeft && localX < focusCardX + focusW &&
                                            localY >= focusCardY + 6 &&
                                            localY < focusCardY + static_cast<int>(std::lround(55.0f * focusCardScale));
                } else {
                    touchTargetCard = hitPinnedWidget(localX, localY);
                    if (touchTargetCard != FocusCard::None) {
                        touchTargetsPinnedCard = true;
                        const auto& widget = widgets[cardIndex(touchTargetCard)];
                        dragStartWidgetX = widget.x;
                        dragStartWidgetY = widget.y;
                    } else if (!fullPanelHidden && localY >= 104 && localY < 278) {
                        if (localX >= 16 && localX < 149) touchTargetCard = FocusCard::CPU;
                        else if (localX >= 157 && localX < 290) touchTargetCard = FocusCard::GPU;
                        else if (localX >= 298 && localX < 432) touchTargetCard = FocusCard::RAM;
                    } else if (!fullPanelHidden && localX >= 16 && localX < 432) {
                        if (localY >= 286 && localY < 368) touchTargetCard = FocusCard::Game;
                        else if (localY >= 376 && localY < 478) touchTargetCard = FocusCard::Temperatures;
                        else if (localY >= 486 && localY < 618) touchTargetCard = FocusCard::System;
                    }
                }
            } else if (multiTouch) {
                focusTouchMoved = true;
                focusSwipeConsumed = true;
            } else {
                const int deltaScreenX = screenX - touchStartScreenX;
                const int deltaScreenY = screenY - touchStartScreenY;
                if (std::abs(deltaScreenX) > 12 || std::abs(deltaScreenY) > 12)
                    focusTouchMoved = true;

                if (focusTouchMoved && !focusSwipeConsumed) {
                    if (touchTargetsFocusedCard && focusCardProgress > 0.75f) {
                        focusCardX = dragStartFocusX + deltaScreenX;
                        focusCardY = dragStartFocusY + deltaScreenY;
                        clampFocusedCard();
                    } else if (touchTargetsPinnedCard && touchTargetCard != FocusCard::None) {
                        auto& widget = widgets[cardIndex(touchTargetCard)];
                        widget.x = dragStartWidgetX + deltaScreenX;
                        widget.y = dragStartWidgetY + deltaScreenY;
                        clampWidgetPlacement(widget);
                    } else if (touchTargetCard == FocusCard::None && focusCard == FocusCard::None) {
                        constexpr u64 SWIPE_WINDOW_NS = 150'000'000ULL;
                        constexpr int SWIPE_DISTANCE = 84;
                        constexpr int EDGE_SIZE = 16;
                        const u64 elapsed = armTicksToNs(armGetSystemTick()) - focusTouchStartNs;
                        const bool fromLeft = layerScreenX == 0 && focusTouchStartX <= EDGE_SIZE;
                        const bool fromRight = layerScreenX == 832 && focusTouchStartX >= (448 - EDGE_SIZE);
                        if (elapsed <= SWIPE_WINDOW_NS &&
                            ((fromLeft && deltaScreenX >= SWIPE_DISTANCE) ||
                             (fromRight && deltaScreenX <= -SWIPE_DISTANCE))) {
                            applyPositionFlip(layerScreenX == 0);
                            focusSwipeConsumed = true;
                            triggerMoveFeedback();
                        }
                    }
                }
            }
        } else if (focusTouchWasDown) {
            focusTouchWasDown = false;
            if (!focusSwipeConsumed) {
                if (focusTouchMoved) {
                    if (touchTargetsFocusedCard) {
                        saveFreePosition();
                        triggerMoveFeedback();
                    } else if (touchTargetsPinnedCard && touchTargetCard != FocusCard::None) {
                        saveWidget(touchTargetCard);
                        triggerMoveFeedback();
                    }
                } else if (touchTargetsFocusedCard && focusCard != FocusCard::None) {
                    if (touchTargetsPinButton) {
                        togglePinned(focusCard);
                    } else {
                        focusCardOpening = false;
                        triggerExitFeedback();
                    }
                    touchTargetsFocusedCard = false;
                    touchTargetsPinButton = false;
                    return true;
                } else if (touchTargetCard != FocusCard::None) {
                    if (fullPanelHidden) {
                        touchTargetsPinnedCard = false;
                        touchTargetCard = FocusCard::None;
                        return true;
                    }
                    focusCard = touchTargetCard;
                    focusCardOpening = true;
                    focusCardProgress = 0.0f;
                    if (touchTargetsPinnedCard)
                        focusCardY = widgets[cardIndex(touchTargetCard)].y - 18;
                    clampFocusedCard();
                    triggerOnFeedback();
                    touchTargetsPinnedCard = false;
                    return true;
                }
            }
            focusSwipeConsumed = false;
            focusPinching = false;
            touchTargetCard = FocusCard::None;
            touchTargetsFocusedCard = false;
            touchTargetsPinnedCard = false;
            touchTargetsPinButton = false;
        }

        if ((keysDown & KEY_X) && focusCard != FocusCard::None) {
            togglePinned(focusCard);
            return true;
        }

        if (focusCard != FocusCard::None && focusCardProgress > 0.75f &&
            (keysDown & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN))) {
            if (keysDown & KEY_LEFT)  focusCardX -= 14;
            if (keysDown & KEY_RIGHT) focusCardX += 14;
            if (keysDown & KEY_UP)    focusCardY -= 14;
            if (keysDown & KEY_DOWN)  focusCardY += 14;
            clampFocusedCard();
            saveFreePosition();
            triggerMoveFeedback();
            return true;
        }

        if ((keysDown & KEY_B) && focusCard != FocusCard::None) {
            focusCardOpening = false;
            triggerExitFeedback();
            return true;
        }

        // -- Focus-end: re-enable frame limiter one frame after color reverts ----
        // Checked at the TOP of handleInput so there is always at least one full
        // loop() cycle (update -> draw -> endFrame, no block since isRendering is
        // still false) between when focusClearOnRelease is set (bottom of this
        // function, the frame focus ends) and when isRendering is restored here.
        // This mirrors Mini's clearOnRelease pattern exactly.
        if (focusClearOnRelease && !isRendering) {
            focusClearOnRelease = false;
            isRendering = true;
            leventClear(&renderingStopEvent);
        }

        // -- Plus-hold focus/reposition mode -------------------------------------
        // The poll thread sets plusFocusActive after a 1-second Plus hold and
        // stops the frame limiter. Here we:
        //   1. Re-enable the frame limiter the frame after focus mode ends.
        //   2. While active, map either-joystick full-left/right to position flips
                //      (toggle setPosRight and move the layer on the UI thread).
        {
            const bool focusNow = plusFocusActive.load(std::memory_order_acquire);

            if (focusNow) {
                // Frame limiter is already stopped by the poll thread.
                // Check both joysticks for left/right snap.
                static constexpr int JOYSTICK_SNAP_THRESHOLD = 28000; // ~85 % of 32767
                static bool joystickFlipArmed = true; // re-arm once stick returns to center

                // Accept EITHER stick: use whichever X axis is deflected further, so
                // PLUS + left stick and PLUS + right stick both flip the position.
                const int jxL = joyStickPosLeft.x;
                const int jxR = joyStickPosRight.x;
                const int jx  = (abs(jxR) > abs(jxL)) ? jxR : jxL;
                const bool stickAtRight = (jx >=  JOYSTICK_SNAP_THRESHOLD);
                const bool stickAtLeft  = (jx <= -JOYSTICK_SNAP_THRESHOLD);
                const bool stickNeutral = (abs(jx) < JOYSTICK_SNAP_THRESHOLD / 2);

                if (stickNeutral) {
                    joystickFlipArmed = true; // stick returned to center; allow next flip
                }

                if (joystickFlipArmed && (stickAtRight || stickAtLeft)) {
                    applyPositionFlip(stickAtRight);
                    triggerMoveFeedback();
                    joystickFlipArmed = false; // require stick to return to center before next flip
                }
            }

            // Edge: focus mode just deactivated -> defer frame-limiter re-enable
            // by one frame so loop() draws the reverted background color before
            // endFrame() starts blocking at the slow TeslaFPS interval again.
            if (!focusNow && plusFocusWasActive) {
                focusClearOnRelease = true;
            }
            plusFocusWasActive = focusNow;
        }

        if (isKeyComboPressed(keysHeld, keysDown)) {
            if (fullPanelHidden) {
                fullPanelHidden = false;
                triggerOnFeedback();
                return true;
            }
            isRendering = false;
            leventSignal(&renderingStopEvent);
            
            skipOnce = true;
            runOnce = true;
            TeslaFPS = 60;
            lastSelectedItem = "Full";
            lastMode = "";
            tsl::swapTo<MainMenu>();
            if (skipMain) {
                //lastSelectedItem = "Micro";
                lastMode = "returning";
                tsl::Overlay::get()->close();
                
            }
            else {
                triggerExitFeedback();
                tsl::swapTo<MainMenu>();
            }
            return true;
        }
        return false;
    }
};
