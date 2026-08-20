class MainMenu;

class SecuritySpacificateOverlay : public tsl::Gui {
private:
    struct TelemetrySnapshot {
        bool gameRunning = false;
        uint8_t fps = 0;
        float fpsAverage = 0.0f;
        uint8_t displayHz = 0;
        uint32_t cpuHz = 0;
        uint32_t gpuHz = 0;
        uint32_t ramHz = 0;
        uint32_t gpuLoadTenths = 0;
        uint32_t ramLoadTenths = 0;
        uint32_t ramBandwidthMBs = 0;
        uint64_t ramUsedBytes = 0;
        uint64_t ramTotalBytes = 0;
        float cpuUsage[4] = {0, 0, 0, 0};
        float socTemp = 0.0f;
        float pcbTemp = 0.0f;
        int32_t skinTempMilliC = 0;
        uint32_t cpuTempMilliC = 0;
        uint32_t gpuTempMilliC = 0;
        uint32_t ramTempMilliC = 0;
        float batteryPercent = 0.0f;
        float batteryCurrent = 0.0f;
        float batteryVoltage = 0.0f;
        float batteryPower = 0.0f;
        double fanPercent = 0.0;
        bool hasClockService = false;
        bool hasGpuSensor = false;
    } snapshot;

    SecuritySpacificateSettings settings;
    uint64_t tickWindow = systemtickfrequency;
    uint64_t lastDataUpdateTick = 0;
    bool skipOnce = true;
    bool runOnce = true;

    static constexpr int SCREEN_WIDTH = 1280;
    static constexpr int SCREEN_HEIGHT = 720;
    int frameOffsetX = 0;
    int frameOffsetY = 0;
    int lastBaseX = 0;
    int lastBaseY = 0;
    int actualTotalWidth = 0;
    int actualTotalHeight = 0;
    bool touchWasDown = false;
    bool isDragging = false;
    bool dragOriginValid = false;
    HidTouchState dragStartTouch = {};
    int dragStartOffsetX = 0;
    int dragStartOffsetY = 0;
    bool pinchActive = false;
    uint8_t pinchStartPercent = 100;
    float pinchStartDistance = 0.0f;

    bool originalUseRightAlignment = ult::useRightAlignment;
    tsl::Color originalBackgroundColor = tsl::defaultBackgroundColor;

    static std::string formatMHz(uint32_t hz) {
        if (!hz) return "Нет данных";
        char text[24];
        snprintf(text, sizeof(text), "%u.%u МГц", hz / 1000000, (hz / 100000) % 10);
        return text;
    }

    static std::string formatPercent(float value) {
        char text[20];
        snprintf(text, sizeof(text), "%.1f%%", std::clamp(value, 0.0f, 100.0f));
        return text;
    }

    static std::string formatTemperature(float value) {
        if (value <= 0.0f) return "Нет данных";
        char text[20];
        snprintf(text, sizeof(text), "%.1f°C", value);
        return text;
    }

    static std::string formatMiB(uint64_t bytes) {
        if (!bytes) return "Нет данных";
        char text[24];
        snprintf(text, sizeof(text), "%.0f МБ", static_cast<double>(bytes) / (1024.0 * 1024.0));
        return text;
    }

    void updateLayerPos() {
        if (!ult::limitedMemory) return;
        const int viPosition = std::max(0, std::min(
            static_cast<int>(lastBaseX * 1.5f + 0.5f) - tsl::impl::currentUnderscanPixels.first,
            1280 - 32 - tsl::impl::currentUnderscanPixels.first));
        tsl::gfx::Renderer::get().setLayerPos(viPosition, 0);
        ult::layerEdge = lastBaseX;
    }

    void captureSnapshot() {
        const uint64_t safeTickWindow = std::max<uint64_t>(tickWindow, 1);
        for (size_t core = 0; core < 4; ++core) {
            const std::atomic<uint64_t>* idleTicks[] = {&idletick0, &idletick1, &idletick2, &idletick3};
            const uint64_t idle = std::min(idleTicks[core]->load(std::memory_order_acquire), safeTickWindow);
            snapshot.cpuUsage[core] = std::clamp(100.0f * (1.0f - static_cast<float>(idle) / safeTickWindow), 0.0f, 100.0f);
        }

        mutexLock(&mutex_Misc);
        snapshot.gameRunning = GameRunning.load(std::memory_order_acquire);
        snapshot.fps = FPS;
        snapshot.fpsAverage = useOldFPSavg ? FPSavg_old : FPSavg;
        snapshot.displayHz = refreshRate;
        snapshot.cpuHz = realCPU_Hz ? realCPU_Hz : CPU_Hz;
        snapshot.gpuHz = realGPU_Hz ? realGPU_Hz : GPU_Hz;
        snapshot.ramHz = realRAM_Hz ? realRAM_Hz : RAM_Hz;
        snapshot.gpuLoadTenths = GPU_Load_u;
        snapshot.ramLoadTenths = ramLoad[SysClkRamLoad_All];
        snapshot.ramBandwidthMBs = ramBW_MBs;
        snapshot.ramUsedBytes = RAM_Used_application_u + RAM_Used_applet_u + RAM_Used_system_u + RAM_Used_systemunsafe_u;
        snapshot.ramTotalBytes = RAM_Total_application_u + RAM_Total_applet_u + RAM_Total_system_u + RAM_Total_systemunsafe_u;
        snapshot.socTemp = SOC_temperatureF;
        snapshot.pcbTemp = PCB_temperatureF;
        snapshot.skinTempMilliC = skin_temperaturemiliC;
        snapshot.cpuTempMilliC = componentCPU_mC;
        snapshot.gpuTempMilliC = componentGPU_mC;
        snapshot.ramTempMilliC = componentRAM_mC;
        snapshot.fanPercent = Rotation_Duty;
        snapshot.hasClockService = R_SUCCEEDED(clkrstCheck) || R_SUCCEEDED(pcvCheck);
        snapshot.hasGpuSensor = R_SUCCEEDED(nvCheck);
        mutexUnlock(&mutex_Misc);

        mutexLock(&mutex_BatteryChecker);
        snapshot.batteryPercent = static_cast<float>(_batteryChargeInfoFields.RawBatteryCharge) / 1000.0f;
        snapshot.batteryCurrent = batCurrentAvg;
        snapshot.batteryVoltage = batVoltageAvg;
        snapshot.batteryPower = PowerConsumption;
        mutexUnlock(&mutex_BatteryChecker);
    }

    void persistPosition() const {
        auto iniData = ult::getParsedDataFromIniFile(configIniPath);
        iniData["security-spacificate"]["frame_offset_x"] = std::to_string(frameOffsetX);
        iniData["security-spacificate"]["frame_offset_y"] = std::to_string(frameOffsetY);
        ult::saveIniFileData(configIniPath, iniData);
    }

public:
    SecuritySpacificateOverlay() {
        disableJumpTo = true;
        GetConfigSettings(&settings);
        frameOffsetX = settings.frameOffsetX;
        frameOffsetY = settings.frameOffsetY;
        TeslaFPS = settings.refreshRate;
        tickWindow = systemtickfrequency / std::max<uint8_t>(settings.refreshRate, 1);
        idletick0.store(tickWindow, std::memory_order_relaxed);
        idletick1.store(tickWindow, std::memory_order_relaxed);
        idletick2.store(tickWindow, std::memory_order_relaxed);
        idletick3.store(tickWindow, std::memory_order_relaxed);
        tsl::defaultBackgroundColor = tsl::Color(settings.backgroundColor);
        tsl::hlp::requestForeground(false);
        deactivateOriginalFooter = true;
        if (settings.disableScreenshots) tsl::gfx::Renderer::get().removeScreenshotStacks();
        // A 4 MB loader heap has a 448 px-wide layer and cannot show two panels.
        // In that case createUI presents an explicit one-tap upgrade screen instead
        // of starting sensor threads for a cropped interface.
        if (!ult::limitedMemory) {
            mutexInit(&mutex_Misc);
            mutexInit(&mutex_BatteryChecker);
            StartThreads();
        }
    }

    ~SecuritySpacificateOverlay() {
        if (!ult::limitedMemory) CloseThreads();
        tsl::defaultBackgroundColor = originalBackgroundColor;
        ult::useRightAlignment = originalUseRightAlignment;
        if (settings.disableScreenshots) tsl::gfx::Renderer::get().addScreenshotStacks();
        deactivateOriginalFooter = false;
        if (ult::limitedMemory)
            ult::layerEdge = (ult::useRightAlignment && ult::correctFrameSize) ? (1280 - 448) : 0;
    }

    virtual tsl::elm::Element* createUI() override {
        if (ult::limitedMemory) {
            auto* list = new tsl::elm::List();
            list->addItem(new tsl::elm::CategoryHeader("Security-Spacificate"));
            list->addItem(new tsl::elm::ListItem("Для двух панелей нужен широкий слой"));
            auto* enableWideLayer = new tsl::elm::ListItem("Включить 8 МБ и перезапустить");
            enableWideLayer->setClickListener([](uint64_t keys) {
                if (keys & KEY_A) {
                    if (ult::setOverlayHeapSize(ult::OverlayHeapSize::Size_8MB)) {
                        tsl::setNextOverlay(filepath, "-security_spacificate --direct");
                        skipClosingExitFeedback = true;
                        tsl::Overlay::get()->close();
                    }
                    return true;
                }
                return false;
            });
            list->addItem(enableWideLayer);
            auto* rootFrame = new tsl::elm::HeaderOverlayFrame("Ряжа-Монитор", "Подготовка экрана");
            rootFrame->setContent(list);
            return rootFrame;
        }

        auto* status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* renderer, u16, u16, u16, u16) {
            const float requestedScale = std::clamp(settings.touchScalePercent / 100.0f, 0.70f, 1.50f);
            // A 4 MB Tesla layer is only 448 px wide. Keep the diagnostic view usable
            // there instead of allowing a wide card tree to be clipped by the layer.
            const float uiScale = ult::limitedMemory ? 0.70f : requestedScale;
            const int outerWidth = static_cast<int>(std::lround(620.0f * uiScale));
            const int outerHeight = static_cast<int>(std::lround(438.0f * uiScale));
            const int titleHeight = std::max(26, static_cast<int>(std::lround(36.0f * uiScale)));
            const int gap = std::max(5, static_cast<int>(std::lround(10.0f * uiScale)));
            const int inset = std::max(7, static_cast<int>(std::lround(12.0f * uiScale)));
            const int labelFont = std::max(10, static_cast<int>(std::lround(13.0f * uiScale)));
            const int titleFont = std::max(13, static_cast<int>(std::lround(18.0f * uiScale)));
            const int smallFont = std::max(9, static_cast<int>(std::lround(11.0f * uiScale)));
            const float measuredSpace = static_cast<float>(renderer->getTextDimensions(" ", false, 16).first);
            const float space = measuredSpace > 0.5f ? measuredSpace : 4.0f;
            const int borderPx = std::max(1, static_cast<int>(std::lround(space * settings.borderThickness / 10.0f)));
            const int outerBorder = settings.useBorder ? borderPx : 0;
            const int frameWidth = outerWidth + outerBorder * 2;
            const int frameHeight = outerHeight + outerBorder * 2;

            const int baseX = (SCREEN_WIDTH - frameWidth) / 2;
            const int baseY = (SCREEN_HEIGHT - frameHeight) / 2;
            const int minOffsetX = settings.framePadding - baseX;
            const int maxOffsetX = SCREEN_WIDTH - frameWidth - settings.framePadding - baseX;
            const int minOffsetY = settings.framePadding - baseY;
            const int maxOffsetY = SCREEN_HEIGHT - frameHeight - settings.framePadding - baseY;
            frameOffsetX = std::clamp(frameOffsetX, minOffsetX, maxOffsetX);
            frameOffsetY = std::clamp(frameOffsetY, minOffsetY, maxOffsetY);
            const int globalX = baseX + frameOffsetX;
            const int globalY = baseY + frameOffsetY;
            lastBaseX = globalX;
            lastBaseY = globalY;
            actualTotalWidth = frameWidth;
            actualTotalHeight = frameHeight;
            updateLayerPos();

            const int drawX = ult::limitedMemory ? std::max(0, globalX - (SCREEN_WIDTH - 448)) : globalX;
            const int drawY = globalY;
            const int contentX = drawX + outerBorder;
            const int contentY = drawY + outerBorder;
            const int bodyY = contentY + titleHeight + gap;
            const int cardWidth = (outerWidth - gap) / 2;
            const int cardHeight = (outerHeight - titleHeight - gap * 2) / 2;
            const int leftX = contentX;
            const int rightX = contentX + cardWidth + gap;
            const int topY = bodyY;
            const int bottomY = bodyY + cardHeight + gap;

            const tsl::Color background(settings.backgroundColor);
            const tsl::Color focusBackground(settings.focusBackgroundColor);
            const tsl::Color titleColor(settings.catColor1);
            const tsl::Color labelColor(settings.catColor2);
            const tsl::Color textColor(settings.textColor);
            const tsl::Color borderColor(settings.borderColor);
            const auto wheel = makeBorderWheel(settings);
            const int radius = std::max(9, static_cast<int>(std::lround(space * settings.cornerRadiusSp / 10.0f)));

            renderer->drawRoundedRectSingleThreaded(contentX, contentY, outerWidth, outerHeight, radius, background);
            if (settings.useBorder)
                renderer->drawBorderedRoundedRect(drawX, drawY, frameWidth, frameHeight, borderPx, radius + outerBorder, borderColor, &wheel);

            renderer->drawString("SECURITY-SPACIFICATE", false, contentX + inset, contentY + titleHeight - inset, titleFont, titleColor);
            renderer->drawString("Игра и система: два независимых контура", false,
                                 contentX + inset + static_cast<int>(std::lround(230.0f * uiScale)),
                                 contentY + titleHeight - inset, smallFont, labelColor);

            const auto drawCard = [&](int x, int y, const char* title) {
                renderer->drawRoundedRectSingleThreaded(x, y, cardWidth, cardHeight, radius, focusBackground);
                renderer->drawBorderedRoundedRect(x, y, cardWidth, cardHeight, 1, radius, borderColor, &wheel);
                renderer->drawString(title, false, x + inset, y + titleHeight - inset, titleFont, titleColor);
            };
            const auto drawRow = [&](int x, int y, const char* label, const std::string& value, const tsl::Color& valueColor) {
                renderer->drawString(label, false, x + inset, y, labelFont, labelColor);
                renderer->drawString(value.c_str(), false, x + static_cast<int>(std::lround(100.0f * uiScale)), y, labelFont, valueColor);
            };

            if (settings.showPerformance) {
                drawCard(leftX, topY, "Игра и нагрузка");
                const int y = topY + titleHeight + inset;
                const std::string fpsText = snapshot.gameRunning && snapshot.fpsAverage < 254.0f
                    ? (std::to_string(snapshot.fps) + " / " + std::to_string(static_cast<int>(std::lround(snapshot.fpsAverage))))
                    : "Нет данных";
                drawRow(leftX, y, "FPS", fpsText, textColor);
                drawRow(leftX, y + labelFont + 5, "Гц", snapshot.displayHz ? std::to_string(snapshot.displayHz) : "Нет данных", textColor);
                drawRow(leftX, y + (labelFont + 5) * 2, "ЦПУ", formatPercent((snapshot.cpuUsage[0] + snapshot.cpuUsage[1] + snapshot.cpuUsage[2] + snapshot.cpuUsage[3]) / 4.0f), textColor);
                drawRow(leftX, y + (labelFont + 5) * 3, "ГПУ", snapshot.hasGpuSensor ? formatPercent(snapshot.gpuLoadTenths / 10.0f) : "Нет данных", textColor);
            }

            if (settings.showSystem) {
                drawCard(leftX, bottomY, "Частоты и ОЗУ");
                const int y = bottomY + titleHeight + inset;
                drawRow(leftX, y, "ЦПУ", formatMHz(snapshot.cpuHz), textColor);
                drawRow(leftX, y + labelFont + 5, "ГПУ", formatMHz(snapshot.gpuHz), textColor);
                drawRow(leftX, y + (labelFont + 5) * 2, "ОЗУ", formatMiB(snapshot.ramUsedBytes) + " / " + formatMiB(snapshot.ramTotalBytes), textColor);
                drawRow(leftX, y + (labelFont + 5) * 3, "Шина", snapshot.ramBandwidthMBs ? std::to_string(snapshot.ramBandwidthMBs) + " МБ/с" : "Нет данных", textColor);
            }

            if (settings.showThermals) {
                drawCard(rightX, topY, "Тепло");
                const int y = topY + titleHeight + inset;
                const auto thermalColor = [&](float value) {
                    return settings.useDynamicColors ? tsl::GradientColor(value, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColor;
                };
                drawRow(rightX, y, "ЦПУ", formatTemperature(snapshot.cpuTempMilliC / 1000.0f), thermalColor(snapshot.cpuTempMilliC / 1000.0f));
                drawRow(rightX, y + labelFont + 5, "ГПУ", formatTemperature(snapshot.gpuTempMilliC / 1000.0f), thermalColor(snapshot.gpuTempMilliC / 1000.0f));
                drawRow(rightX, y + (labelFont + 5) * 2, "ОЗУ", formatTemperature(snapshot.ramTempMilliC / 1000.0f), thermalColor(snapshot.ramTempMilliC / 1000.0f));
                drawRow(rightX, y + (labelFont + 5) * 3, "Плата", formatTemperature(snapshot.pcbTemp), thermalColor(snapshot.pcbTemp));
            }

            if (settings.showPower) {
                drawCard(rightX, bottomY, "Питание");
                const int y = bottomY + titleHeight + inset;
                char powerText[32];
                snprintf(powerText, sizeof(powerText), "%.2f Вт", snapshot.batteryPower);
                drawRow(rightX, y, "Заряд", formatPercent(snapshot.batteryPercent), textColor);
                drawRow(rightX, y + labelFont + 5, "Мощн.", powerText, textColor);
                char voltageText[32];
                snprintf(voltageText, sizeof(voltageText), "%.2f В", snapshot.batteryVoltage);
                drawRow(rightX, y + (labelFont + 5) * 2, "Напр.", voltageText, textColor);
                drawRow(rightX, y + (labelFont + 5) * 3, "Вент.", formatPercent(static_cast<float>(snapshot.fanPercent)), textColor);
            }
        });

        auto* rootFrame = new tsl::elm::HeaderOverlayFrame("Ряжа-Монитор", "Security-Spacificate");
        rootFrame->setContent(status);
        return rootFrame;
    }

    virtual void update() override {
        if (ult::limitedMemory) return;
        const uint64_t now = armGetSystemTick();
        const uint64_t interval = systemtickfrequency / std::max<uint8_t>(settings.sampleRate, 1);
        if (now - lastDataUpdateTick >= interval) {
            lastDataUpdateTick = now;
            captureSnapshot();
        }
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

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState&, HidAnalogStickState, HidAnalogStickState) override {
        if (ult::limitedMemory) {
            if (isKeyComboPressed(keysHeld, keysDown)) {
                tsl::Overlay::get()->close();
                return true;
            }
            return false;
        }
        // Read the physical panel directly: framework coordinates can lag behind
        // while a movable Tesla layer is repositioned.
        HidTouchScreenState rawTouchState = {};
        const bool touchDetected = hidGetTouchScreenStates(&rawTouchState, 1) > 0 && rawTouchState.count > 0;
        const bool pinchDetected = touchDetected && rawTouchState.count >= 2;

        if (pinchDetected) {
            const float dx = static_cast<float>(rawTouchState.touches[1].x) - static_cast<float>(rawTouchState.touches[0].x);
            const float dy = static_cast<float>(rawTouchState.touches[1].y) - static_cast<float>(rawTouchState.touches[0].y);
            const float distance = std::sqrt(dx * dx + dy * dy);
            if (!pinchActive || pinchStartDistance < 1.0f) {
                pinchActive = true;
                pinchStartDistance = std::max(distance, 1.0f);
                pinchStartPercent = settings.touchScalePercent;
            } else {
                const float gestureScale = std::clamp(distance / pinchStartDistance, 0.70f, 1.30f);
                settings.touchScalePercent = static_cast<uint8_t>(std::clamp<int>(
                    std::lround(pinchStartPercent * gestureScale), 70, 150));
            }
            isDragging = false;
            dragOriginValid = false;
            touchWasDown = true;
        } else {
            if (pinchActive) {
                ult::setIniFileValue(configIniPath, "security-spacificate", "touch_scale", std::to_string(settings.touchScalePercent));
                pinchActive = false;
                pinchStartDistance = 0.0f;
                touchWasDown = touchDetected;
                dragOriginValid = false;
            }

            if (touchDetected) {
                const HidTouchState& touch = rawTouchState.touches[0];
                const int safeWidth = std::max(actualTotalWidth, 1);
                const int safeHeight = std::max(actualTotalHeight, 1);
                const int touchPadding = 6;
                const bool inBounds = touch.x >= lastBaseX - touchPadding &&
                                      touch.x <= lastBaseX + safeWidth + touchPadding &&
                                      touch.y >= lastBaseY - touchPadding &&
                                      touch.y <= lastBaseY + safeHeight + touchPadding;
                if (!touchWasDown) {
                    touchWasDown = true;
                    dragOriginValid = inBounds;
                    if (dragOriginValid) {
                        dragStartTouch = touch;
                        dragStartOffsetX = frameOffsetX;
                        dragStartOffsetY = frameOffsetY;
                        isDragging = false;
                    }
                }
                if (dragOriginValid) {
                    const int deltaX = static_cast<int>(touch.x) - static_cast<int>(dragStartTouch.x);
                    const int deltaY = static_cast<int>(touch.y) - static_cast<int>(dragStartTouch.y);
                    if (std::abs(deltaX) + std::abs(deltaY) >= 2) isDragging = true;
                    if (isDragging) {
                        const int baseX = (SCREEN_WIDTH - safeWidth) / 2;
                        const int baseY = (SCREEN_HEIGHT - safeHeight) / 2;
                        const int minOffsetX = settings.framePadding - baseX;
                        const int maxOffsetX = SCREEN_WIDTH - safeWidth - settings.framePadding - baseX;
                        const int minOffsetY = settings.framePadding - baseY;
                        const int maxOffsetY = SCREEN_HEIGHT - safeHeight - settings.framePadding - baseY;
                        frameOffsetX = std::clamp(dragStartOffsetX + deltaX, minOffsetX, maxOffsetX);
                        frameOffsetY = std::clamp(dragStartOffsetY + deltaY, minOffsetY, maxOffsetY);
                        updateLayerPos();
                    }
                }
            } else if (touchWasDown) {
                if (isDragging) persistPosition();
                touchWasDown = false;
                isDragging = false;
                dragOriginValid = false;
            }
        }

        if (!isDragging && !pinchActive && isKeyComboPressed(keysHeld, keysDown)) {
            isRendering = false;
            leventSignal(&renderingStopEvent);
            skipOnce = true;
            runOnce = true;
            TeslaFPS = 60;
            lastSelectedItem = "Security-Spacificate";
            lastMode = "";
            if (skipMain) {
                lastMode = "returning";
                tsl::Overlay::get()->close();
            } else {
                tsl::setNextOverlay(filepath.c_str(), "--lastSelectedItem 'Security-Spacificate'");
                tsl::Overlay::get()->close();
            }
            return true;
        }
        return isDragging || pinchActive;
    }
};
