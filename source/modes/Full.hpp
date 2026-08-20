class MainMenu;

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
    char Resolutions_c[64] = "";
    char readSpeed_c[32] = "";
    
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

    // Swipe-to-flip position detection (left/right for Full, mirrors Micro's
    // top/bottom version). fullSwipeExitEvent is a global (zero-initialized
    // like threadexit) - using a class member LEvent without leventCreate
    // causes uninitialized handle corruption on the 2nd/3rd open when heap
    // memory is reused with stale content.
    // swipeFlipPending is the one-way signal from poll thread -> handleInput.
    // plusFocusActive is set by the poll thread while Plus is held long enough
    // to activate focus/reposition mode; cleared by the poll thread on release.
    std::atomic<bool> swipeFlipPending{false};
    std::atomic<bool> plusFocusActive{false};   // true while Plus-hold focus mode is live
    bool plusFocusWasActive = false;             // edge-detect: was active last handleInput frame
    bool swipeClearOnRelease = false;
    bool focusClearOnRelease = false;            // deferred frame-limiter re-enable after focus ends
    Thread swipePollThread;

    bool originalUseRightAlignment = ult::useRightAlignment;
    tsl::Color originalBackgroundColor = tsl::defaultBackgroundColor;

    // Poll thread: wakes every ~32 ms via leventWait (same pattern as Micro's
    // swipePollFunc / gpuLoadThread / BatteryChecker in Utils.hpp). leventWait
    // returns true when fullSwipeExitEvent is signalled -> thread exits immediately.
    // On swipe trigger: stops the frame limiter (isRendering=false + leventSignal) and
    // sets swipeFlipPending. handleInput re-enables the limiter via swipeClearOnRelease.
    // Plus-hold focus mode: sets plusFocusActive after a configurable hold
    // ("Button Move Delay", default 1000 ms); cleared on release.
    // While active, handleInput switches to focusBackgroundColor and handles joystick flips.
    // The touch-swipe edge/distance bounds mirror tesla.hpp's own swipe-to-open
    // detection (16 px edge guard, 84 px travel, 150 ms window) since Full's
    // flip is a left/right gesture just like swipe-to-open.
    static void swipePollFunc(void* arg) {
        auto* self = static_cast<FullOverlay*>(arg);

        static constexpr u64 POLL_NS          = 32'000'000ULL;   // 32 ms sleep / exit check
        static constexpr u64 SWIPE_WINDOW_NS  = 150'000'000ULL;  // 150 ms gesture window
        static constexpr int SWIPE_DIST_PX    = 84;              // framebuffer pixels
        static constexpr int SWIPE_EDGE_PX    = 16;              // touch must start within 16 px of left/right edge
        static constexpr int SCREEN_WIDTH_PX  = 1280;            // framebuffer width
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

        HidTouchScreenState state = {0};
        bool touching             = false;
        int  initialX             = 0;
        u64  touchStartNs         = 0;
        u64  plusHoldStart        = 0;

        do {
            // Sleep gate: swipe and button input are impossible while the
            // system is sleeping and the display is off. Block here until
            // wake; the while condition checks fullSwipeExitEvent on resume
            // so shutdown during sleep still exits cleanly within one POLL_NS.
            if (tsl::hlp::waitWhileSleeping(POLL_NS)) continue;

            const u64 nowNs = armTicksToNs(armGetSystemTick());

            // -- Touch-swipe-to-flip ------------------------------------------
            if (hidGetTouchScreenStates(&state, 1) && state.count > 0) {
                const int tx = static_cast<int>(state.touches[0].x);

                if (!touching) {
                    // Finger just placed - record origin
                    touching     = true;
                    initialX     = tx;
                    touchStartNs = nowNs;
                } else if (!self->swipeFlipPending.load(std::memory_order_acquire)) {
                    // Gesture in progress, no flip queued yet - check thresholds
                    const u64 elapsed = nowNs - touchStartNs;
                    if (elapsed <= SWIPE_WINDOW_NS) {
                        const int  deltaX  = tx - initialX;
                        const bool atLeft  = !self->settings.setPosRight;
                        const bool atRight =  self->settings.setPosRight;
                        // Touch must have started within SWIPE_EDGE_PX of the
                        // relevant screen edge - same guard tesla uses for swipe-to-open.
                        const bool startedAtEdge = (atLeft  && initialX <= SWIPE_EDGE_PX) ||
                                                   (atRight && initialX >= SCREEN_WIDTH_PX - SWIPE_EDGE_PX);
                        if (startedAtEdge &&
                            ((atLeft  && deltaX >=  SWIPE_DIST_PX) ||
                             (atRight && deltaX <= -SWIPE_DIST_PX))) {
                            if (isRendering) {
                                isRendering = false;
                                leventSignal(&renderingStopEvent);
                            }
                            self->swipeFlipPending.store(true, std::memory_order_release);
                            triggerMoveFeedback();
                        }
                    }
                }
            } else {
                // No touch - reset so the next finger-down starts a fresh gesture
                touching = false;
            }

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
        if (wantRight == settings.setPosRight) return;
        settings.setPosRight = wantRight;
        ult::setIniFileValue(configIniPath, "full", "layer_width_align", wantRight ? "right" : "left");
        ult::useRightAlignment = wantRight;
        const auto [horizontalUnderscanPixels, verticalUnderscanPixels] = tsl::gfx::getUnderscanPixels();
        if (wantRight) {
            tsl::gfx::Renderer::get().setLayerPos(1280-32 - horizontalUnderscanPixels, 0);
        } else {
            tsl::gfx::Renderer::get().setLayerPos(0, 0);
        }
        // Note: callers are responsible for triggering move feedback.
        // The swipe path already fires it in the poll thread; the joystick
        // path calls triggerMoveFeedback() explicitly after applyPositionFlip.
    }

public:
    FullOverlay() { 
        disableJumpTo = true;
        GetConfigSettings(&settings);
        if (settings.useRyazhaTheme) {
            // Full is the sole Status Monitor mode that follows the active
            // libryazhahand theme. Compact modes retain their own palettes.
            tsl::initializeTheme();
            tsl::initializeThemeVars();
            settings.backgroundColor = tsl::defaultBackgroundColor.rgba;
            settings.focusBackgroundColor = tsl::unfocusedColor.rgba;
            settings.separatorColor = tsl::headerSeparatorColor.rgba;
            settings.catColor1 = tsl::headerTextColor.rgba;
            settings.catColor2 = tsl::bottomTextColor.rgba;
            settings.textColor = tsl::defaultTextColor.rgba;
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
        if (settings.setPosRight) {
            const auto [horizontalUnderscanPixels, verticalUnderscanPixels] = tsl::gfx::getUnderscanPixels();
            tsl::gfx::Renderer::get().setLayerPos(1280-32 - horizontalUnderscanPixels, 0);
            ult::useRightAlignment = true;
        } else {
            tsl::gfx::Renderer::get().setLayerPos(0, 0);
            ult::useRightAlignment = false;
        }
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
            // Full-mode card layout. Keep every live metric in a bounded cell:
            //  CPU | GPU
            //  RAM | Game
            //  Board and power (full width)
            static const std::vector<std::string> specialChars = {""};

            constexpr int outerX = 14;
            constexpr int outerW = 420;
            constexpr int gap = 10;
            constexpr int cardW = (outerW - gap) / 2;
            constexpr int topY = 102;
            constexpr int topH = 164;
            constexpr int middleY = topY + topH + gap;
            constexpr int middleH = 158;
            constexpr int boardY = middleY + middleH + gap;
            constexpr int boardH = 188;
            constexpr int leftX = outerX;
            constexpr int rightX = outerX + cardW + gap;
            constexpr int labelInset = 12;
            constexpr int valueInset = 84;
            constexpr int lineH = 15;

            const tsl::Color cardFill(1, 2, 7, 0xF);
            const tsl::Color titleColor(15, 13, 4, 0xF);
            const tsl::Color labelColor(15, 15, 14, 0xF);
            const tsl::Color textColor(14, 14, 15, 0xF);
            const tsl::Color warmBorder(15, 11, 2, 0xF);
            const tsl::Color redAccent(15, 4, 6, 0xF);
            const auto ryazhaWheel = tsl::makeSwitch2Wheel(
                tsl::Color(15, 9, 2, 0xF),   // gold
                tsl::Color(15, 15, 14, 0xF), // white
                titleColor,                  // bright gold
                tsl::Color(10, 5, 1, 0xF),   // deep amber
                redAccent,                   // red/pink accent
                tsl::Color(10, 1, 4, 0xF),   // deep crimson
                5.4f
            );

            const auto drawCard = [&](int cx, int cy, int cw, int ch, const char* title) {
                renderer->drawRoundedRectSingleThreaded(cx + 2, cy + 2, cw - 4, ch - 4, 15, cardFill);
                renderer->drawBorderedRoundedRect(cx, cy, cw, ch, 2, 16, warmBorder, &ryazhaWheel);
                renderer->drawString(title, false, cx + labelInset, cy + 26, 18, titleColor);
            };
            const auto drawRow = [&](int cx, int baseline, const char* label, const char* value, int font = 13) {
                renderer->drawString(label, false, cx + labelInset, baseline, font, labelColor);
                renderer->drawString(value, false, cx + valueInset, baseline, font, textColor);
            };
            const auto firstLine = [](const char* value) {
                std::string compact(value ? value : "--");
                const auto newline = compact.find('\n');
                if (newline != std::string::npos) compact.resize(newline);
                return compact;
            };
            const auto firstToken = [](const char* value) {
                std::string compact(value ? value : "--");
                const auto separator = compact.find_first_of(" \n");
                if (separator != std::string::npos) compact.resize(separator);
                return compact;
            };

            drawCard(leftX, topY, cardW, topH, "ЦПУ");
            const int cpuY = topY + 51;
            if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck)) {
                if (realCPU_Hz && settings.showRealFreqs)
                    drawRow(leftX, cpuY, "Реал.", RealCPU_Hz_c);
                if (settings.showTargetFreqs)
                    drawRow(leftX, cpuY + lineH, "Цель", CPU_Hz_c);
                if (settings.showDeltas && (settings.showRealFreqs || settings.showTargetFreqs))
                    drawRow(leftX, cpuY + lineH * 2, "Δ", DeltaCPU_c);
                const int coreY = cpuY + lineH * 4;
                drawRow(leftX, coreY, "Ядро 0", CPU_Core0_c, 12);
                drawRow(leftX, coreY + 13, "Ядро 1", CPU_Core1_c, 12);
                drawRow(leftX, coreY + 26, "Ядро 2", CPU_Core2_c, 12);
                drawRow(leftX, coreY + 39, "Ядро 3", CPU_Core3_c, 12);
            } else {
                drawRow(leftX, cpuY, "Статус", "н/д");
            }

            drawCard(rightX, topY, cardW, topH, "ГПУ");
            const int gpuY = topY + 51;
            if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck) || R_SUCCEEDED(nvCheck)) {
                if (realGPU_Hz && settings.showRealFreqs)
                    drawRow(rightX, gpuY, "Реал.", RealGPU_Hz_c);
                if (settings.showTargetFreqs)
                    drawRow(rightX, gpuY + lineH, "Цель", GPU_Hz_c);
                if (settings.showDeltas && (settings.showRealFreqs || settings.showTargetFreqs))
                    drawRow(rightX, gpuY + lineH * 2, "Δ", DeltaGPU_c);
                if (R_SUCCEEDED(nvCheck))
                    drawRow(rightX, gpuY + lineH * 3, "Загр.", GPU_Load_c);
            } else {
                drawRow(rightX, gpuY, "Статус", "н/д");
            }

            drawCard(leftX, middleY, cardW, middleH, "ОЗУ");
            const int ramY = middleY + 51;
            if (R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck) || R_SUCCEEDED(Hinted)) {
                if (realRAM_Hz && settings.showRealFreqs)
                    drawRow(leftX, ramY, "Реал.", RealRAM_Hz_c);
                if (settings.showTargetFreqs)
                    drawRow(leftX, ramY + lineH, "Цель", RAM_Hz_c);
                const std::string totalLoad = firstToken(RAM_load_c);
                drawRow(leftX, ramY + lineH * 2, "Загр.", totalLoad.c_str(), 11);
                const std::string totalRam = firstLine(RAM_var_compressed_c);
                if (!totalRam.empty())
                    drawRow(leftX, ramY + lineH * 4, "Всего", totalRam.c_str(), 11);
            } else {
                drawRow(leftX, ramY, "Статус", "н/д");
            }

            drawCard(rightX, middleY, cardW, middleH, "Игра");
            const int gameY = middleY + 52;
            if (GameRunning) {
                if (settings.showFPS) {
                    drawRow(rightX, gameY, "PFPS", PFPS_value_c);
                    drawRow(rightX, gameY + lineH, "FPS", FPS_value_c);
                }
                if (settings.showRES && NxFps && SharedMemoryUsed && (NxFps->API >= 1)) {
                    drawRow(rightX, gameY + lineH * 3, "Разр.", Resolutions_c, 11);
                }
                if (settings.showRDSD) {
                    drawRow(rightX, gameY + lineH * 4, "Чтение", readSpeed_c, 11);
                }
            } else {
                renderer->drawString("Игра не запущена", false, rightX + labelInset, gameY + 8, 13, labelColor);
            }

            drawCard(outerX, boardY, outerW, boardH, "Плата и питание");
            const int boardX = outerX;
            drawRow(boardX, boardY + 53, "Батарея", BatteryDraw_c);
            drawRow(boardX, boardY + 70, "Вент.", Rotation_SpeedLevel_c);
            renderer->drawString("Температуры", false, boardX + labelInset, boardY + 96, 13, titleColor);

            const int tempLabelY = boardY + 119;
            const int tempValueY = boardY + 136;
            renderer->drawString("ЦПУ", false, boardX + 16, tempLabelY, 12, labelColor);
            renderer->drawString(CPU_temp_c, false, boardX + 16, tempValueY, 12,
                                 settings.useDynamicColors ? tsl::GradientColor(componentCPU_mC / 1000.0f, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColor);
            renderer->drawString("ГПУ", false, boardX + 87, tempLabelY, 12, labelColor);
            renderer->drawString(GPU_temp_c, false, boardX + 87, tempValueY, 12,
                                 settings.useDynamicColors ? tsl::GradientColor(componentGPU_mC / 1000.0f, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColor);
            renderer->drawString("ОЗУ", false, boardX + 158, tempLabelY, 12, labelColor);
            renderer->drawString(MEM_temp_c, false, boardX + 158, tempValueY, 12,
                                 settings.useDynamicColors ? tsl::GradientColor(componentRAM_mC / 1000.0f, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColor);
            renderer->drawString("SoC", false, boardX + 229, tempLabelY, 12, labelColor);
            renderer->drawString(SOC_temperature_c, false, boardX + 229, tempValueY, 12,
                                 settings.useDynamicColors ? tsl::GradientColor(SOC_temperatureF) : textColor);
            renderer->drawString("Плата", false, boardX + 292, tempLabelY, 12, labelColor);
            renderer->drawString(PCB_temperature_c, false, boardX + 292, tempValueY, 12,
                                 settings.useDynamicColors ? tsl::GradientColor(PCB_temperatureF) : textColor);
            renderer->drawString("Корпус", false, boardX + 355, tempLabelY, 12, labelColor);
            renderer->drawString(skin_temperature_c, false, boardX + 355, tempValueY, 12,
                                 settings.useDynamicColors ? tsl::GradientColor(static_cast<float>(skin_temperaturemiliC) / 1000.0f) : textColor);

            static const auto pressWidth = renderer->getTextDimensions("Нажмите ", false, 20).first;
            static const auto keyComboWidth = renderer->getTextDimensions(formattedKeyCombo.c_str(), false, 20).first;
            renderer->drawString("Нажмите ", false, 24, 690, 20, tsl::bottomTextColor);
            renderer->drawStringWithColoredSections(formattedKeyCombo, false, KEY_SYMBOLS, 24 + pressWidth, 690, 20, tsl::bottomTextColor, tsl::buttonColor);
            renderer->drawString(" для выхода", false, 24 + pressWidth + keyComboWidth, 690, 20, tsl::bottomTextColor);
        });

        auto rootFrame = new tsl::elm::HeaderOverlayFrame("Ряжа-Монитор", APP_VERSION);
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
        }

        mutexLock(&mutex_Misc);
        snprintf(CPU_Hz_c, sizeof(CPU_Hz_c), "%u.%u MHz", CPU_Hz / 1000000, (CPU_Hz / 100000) % 10);
        if (realCPU_Hz) {
            snprintf(RealCPU_Hz_c, sizeof(RealCPU_Hz_c), "%u.%u MHz", realCPU_Hz / 1000000, (realCPU_Hz / 100000) % 10);
            const int32_t deltaCPU = (int32_t)(realCPU_Hz / 1000) - (CPU_Hz / 1000);
            snprintf(DeltaCPU_c, sizeof(DeltaCPU_c), "Δ %d.%u", deltaCPU / 1000, abs(deltaCPU / 100) % 10);
        }
        
        ///GPU
        snprintf(GPU_Hz_c, sizeof GPU_Hz_c, "%u.%u MHz", GPU_Hz / 1000000, (GPU_Hz / 100000) % 10);
        if (realGPU_Hz) {
            snprintf(RealGPU_Hz_c, sizeof(RealGPU_Hz_c), "%u.%u MHz", realGPU_Hz / 1000000, (realGPU_Hz / 100000) % 10);
            const int32_t deltaGPU = (int32_t)(realGPU_Hz / 1000) - (GPU_Hz / 1000);
            snprintf(DeltaGPU_c, sizeof(DeltaGPU_c), "Δ %d.%u", deltaGPU / 1000, abs(deltaGPU / 100) % 10);
        }
        snprintf(GPU_Load_c, sizeof GPU_Load_c, "%u.%u%%", GPU_Load_u / 10, GPU_Load_u % 10);
        
        ///RAM
        snprintf(RAM_Hz_c, sizeof RAM_Hz_c, "%u.%u MHz", RAM_Hz / 1000000, (RAM_Hz / 100000) % 10);
        if (realRAM_Hz) {
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
        const int RAMPct_all          = (int)((RAM_Used_all_f          / RAM_Total_all_f)        * 100.0f );
        const int RAMPct_app          = (int)((RAM_Used_application_f / RAM_Total_application_f) * 100.0f );
        const int RAMPct_applet       = (int)((RAM_Used_applet_f      / RAM_Total_applet_f)      * 100.0f );
        const int RAMPct_system       = (int)((RAM_Used_system_f      / RAM_Total_system_f)      * 100.0f );
        const int RAMPct_systemunsafe = (int)((RAM_Used_systemunsafe_f/ RAM_Total_systemunsafe_f)* 100.0f );
        
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

        snprintf(Rotation_SpeedLevel_c, sizeof Rotation_SpeedLevel_c, "%.1f%%", Rotation_Duty);
        
        ///FPS
        if (settings.showFPS == true) {
            snprintf(PFPS_value_c, sizeof PFPS_value_c, "%1u", FPS);
            snprintf(FPS_value_c, sizeof FPS_value_c, "%.1f", useOldFPSavg ? FPSavg_old : FPSavg);
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
        char remainingBatteryLife[8];
        
        /* Normalise "-0.00" → "0.00" W */
        const float drawW = (fabsf(PowerConsumption) < 0.01f) ? 0.0f
                                                         : PowerConsumption;
        
        mutexLock(&mutex_BatteryChecker);
        
        /* keep "--:--" whenever estimate is negative */
        if (batTimeEstimate >= 0 && !(drawW <= 0.01f && drawW >= -0.01f)) {
            snprintf(remainingBatteryLife, sizeof(remainingBatteryLife),
                     "%d:%02d", batTimeEstimate / 60, batTimeEstimate % 60);
        } else {
            strcpy(remainingBatteryLife, "--:--");
        }
        
        const float batteryPercent = (float)_batteryChargeInfoFields.RawBatteryCharge / 1000.0f;
        
        snprintf(BatteryDraw_c, sizeof(BatteryDraw_c),
                 "%.2f W%.0f%% [%s]",
                 drawW,
                 batteryPercent,
                 remainingBatteryLife);
        
        mutexUnlock(&mutex_BatteryChecker);
        } // end shouldUpdateData
        
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
        //      (same logic as swipeFlipPending: toggle setPosRight and move layer).
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

        // -- Swipe-to-flip: re-enable rendering after position transition --------
        // Poll thread stopped the frame limiter when the swipe fired. Once the
        // flip is applied and one frame is drawn with the new position, re-enable.
        if (swipeClearOnRelease && !isRendering) {
            swipeClearOnRelease = false;
            isRendering = true;
            leventClear(&renderingStopEvent);
        }

        // -- Swipe-to-flip: apply position change ---------------------------------
        // Poll thread sets swipeFlipPending; consumed here (main thread) so all
        // settings/render state mutations are safe and single-threaded.
        if (swipeFlipPending.exchange(false, std::memory_order_acq_rel)) {
            applyPositionFlip(!settings.setPosRight);
            swipeClearOnRelease = true;
        }

        if (isKeyComboPressed(keysHeld, keysDown)) {
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