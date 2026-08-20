class MainMenu;

class SecuritySpacificateOverlay : public tsl::Gui {
private:
    enum CardId : int {
        PerformanceCard = 0,
        SystemCard = 1,
        ThermalsCard = 2,
        PowerCard = 3,
        CardCount = 4,
    };

    struct CardBounds {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        bool visible = false;
    } cardBounds[CardCount];

    struct TelemetrySnapshot {
        bool gameRunning = false;
        uint8_t fps = 0;
        float fpsAverage = 0.0f;
        uint8_t displayHz = 0;
        uint32_t cpuHz = 0;
        uint32_t gpuHz = 0;
        uint32_t gpuLoadTenths = 0;
        uint32_t ramBandwidthMBs = 0;
        uint64_t ramUsedBytes = 0;
        uint64_t ramTotalBytes = 0;
        float cpuUsage[4] = {0, 0, 0, 0};
        float pcbTemp = 0.0f;
        uint32_t cpuTempMilliC = 0;
        uint32_t gpuTempMilliC = 0;
        uint32_t ramTempMilliC = 0;
        float batteryPercent = 0.0f;
        float batteryVoltage = 0.0f;
        float batteryPower = 0.0f;
        double fanPercent = 0.0;
        bool hasGpuSensor = false;
    } snapshot;

    SecuritySpacificateSettings settings;
    uint64_t tickWindow = systemtickfrequency;
    uint64_t lastDataUpdateTick = 0;
    bool skipOnce = true;
    bool runOnce = true;

    static constexpr int SCREEN_WIDTH = 1280;
    static constexpr int SCREEN_HEIGHT = 720;
    static constexpr int CARD_PADDING = 6;

    bool touchWasDown = false;
    bool isDragging = false;
    bool pinchActive = false;
    int activeCard = -1;
    HidTouchState dragStartTouch = {};
    int dragStartX = 0;
    int dragStartY = 0;
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

    void getCardSettings(CardId card, int*& x, int*& y, uint8_t*& scale) {
        switch (card) {
            case PerformanceCard:
                x = &settings.performanceX; y = &settings.performanceY; scale = &settings.performanceScale; break;
            case SystemCard:
                x = &settings.systemX; y = &settings.systemY; scale = &settings.systemScale; break;
            case ThermalsCard:
                x = &settings.thermalsX; y = &settings.thermalsY; scale = &settings.thermalsScale; break;
            case PowerCard:
            default:
                x = &settings.powerX; y = &settings.powerY; scale = &settings.powerScale; break;
        }
    }

    const char* cardPrefix(CardId card) const {
        switch (card) {
            case PerformanceCard: return "performance";
            case SystemCard:      return "system";
            case ThermalsCard:    return "thermals";
            case PowerCard:       return "power";
            default:              return "performance";
        }
    }

    bool isCardEnabled(CardId card) const {
        switch (card) {
            case PerformanceCard: return settings.showPerformance;
            case SystemCard:      return settings.showSystem;
            case ThermalsCard:    return settings.showThermals;
            case PowerCard:       return settings.showPower;
            default:              return false;
        }
    }

    void persistCard(CardId card, bool includeScale) const {
        int* x = nullptr;
        int* y = nullptr;
        uint8_t* scale = nullptr;
        const_cast<SecuritySpacificateOverlay*>(this)->getCardSettings(card, x, y, scale);
        auto iniData = ult::getParsedDataFromIniFile(configIniPath);
        const std::string prefix(cardPrefix(card));
        iniData["security-spacificate"][prefix + "_x"] = std::to_string(*x);
        iniData["security-spacificate"][prefix + "_y"] = std::to_string(*y);
        if (includeScale)
            iniData["security-spacificate"][prefix + "_scale"] = std::to_string(*scale);
        ult::saveIniFileData(configIniPath, iniData);
    }

    int findCardAt(int x, int y) const {
        // The last drawn card is the topmost card, so touch respects overlaps.
        for (int card = CardCount - 1; card >= 0; --card) {
            const CardBounds& bounds = cardBounds[card];
            if (bounds.visible && x >= bounds.x && x <= bounds.x + bounds.width &&
                y >= bounds.y && y <= bounds.y + bounds.height)
                return card;
        }
        return -1;
    }

    void captureSnapshot() {
        const uint64_t safeTickWindow = std::max<uint64_t>(tickWindow, 1);
        const std::atomic<uint64_t>* idleTicks[] = {&idletick0, &idletick1, &idletick2, &idletick3};
        for (size_t core = 0; core < 4; ++core) {
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
        snapshot.gpuLoadTenths = GPU_Load_u;
        snapshot.ramBandwidthMBs = ramBW_MBs;
        snapshot.ramUsedBytes = RAM_Used_application_u + RAM_Used_applet_u + RAM_Used_system_u + RAM_Used_systemunsafe_u;
        snapshot.ramTotalBytes = RAM_Total_application_u + RAM_Total_applet_u + RAM_Total_system_u + RAM_Total_systemunsafe_u;
        snapshot.pcbTemp = PCB_temperatureF;
        snapshot.cpuTempMilliC = componentCPU_mC;
        snapshot.gpuTempMilliC = componentGPU_mC;
        snapshot.ramTempMilliC = componentRAM_mC;
        snapshot.fanPercent = Rotation_Duty;
        snapshot.hasGpuSensor = R_SUCCEEDED(nvCheck);
        mutexUnlock(&mutex_Misc);

        mutexLock(&mutex_BatteryChecker);
        snapshot.batteryPercent = static_cast<float>(_batteryChargeInfoFields.RawBatteryCharge) / 1000.0f;
        snapshot.batteryVoltage = batVoltageAvg;
        snapshot.batteryPower = PowerConsumption;
        mutexUnlock(&mutex_BatteryChecker);
    }

public:
    SecuritySpacificateOverlay() {
        disableJumpTo = true;
        GetConfigSettings(&settings);
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
        // A 4 MB loader heap has a 448 px-wide layer and cannot host separate cards.
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
            list->addItem(new tsl::elm::ListItem("Для независимых мониторов нужен широкий слой"));
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
            const float measuredSpace = static_cast<float>(renderer->getTextDimensions(" ", false, 16).first);
            const float space = measuredSpace > 0.5f ? measuredSpace : 4.0f;
            const int borderPx = std::max(1, static_cast<int>(std::lround(space * settings.borderThickness / 10.0f)));
            const int radius = std::max(9, static_cast<int>(std::lround(space * settings.cornerRadiusSp / 10.0f)));
            const tsl::Color focusBackground(settings.focusBackgroundColor);
            const tsl::Color titleColor(settings.catColor1);
            const tsl::Color labelColor(settings.catColor2);
            const tsl::Color textColor(settings.textColor);
            const tsl::Color borderColor(settings.borderColor);
            const auto wheel = makeBorderWheel(settings);

            const auto drawCard = [&](CardId card, const char* title, auto drawContent) {
                if (!isCardEnabled(card)) {
                    cardBounds[card].visible = false;
                    return;
                }
                int* storedX = nullptr;
                int* storedY = nullptr;
                uint8_t* storedScale = nullptr;
                getCardSettings(card, storedX, storedY, storedScale);
                const float uiScale = std::clamp(*storedScale / 100.0f, 0.70f, 1.50f);
                const int width = std::max(180, static_cast<int>(std::lround(276.0f * uiScale)));
                const int height = std::max(126, static_cast<int>(std::lround(172.0f * uiScale)));
                *storedX = std::clamp(*storedX, CARD_PADDING, SCREEN_WIDTH - width - CARD_PADDING);
                *storedY = std::clamp(*storedY, CARD_PADDING, SCREEN_HEIGHT - height - CARD_PADDING);
                cardBounds[card] = {*storedX, *storedY, width, height, true};

                const int inset = std::max(7, static_cast<int>(std::lround(12.0f * uiScale)));
                const int titleHeight = std::max(24, static_cast<int>(std::lround(33.0f * uiScale)));
                const int titleFont = std::max(12, static_cast<int>(std::lround(16.0f * uiScale)));
                const int labelFont = std::max(10, static_cast<int>(std::lround(13.0f * uiScale)));
                const int valueX = *storedX + std::max(91, static_cast<int>(std::lround(100.0f * uiScale)));

                renderer->drawRoundedRectSingleThreaded(*storedX, *storedY, width, height, radius, focusBackground);
                if (settings.useBorder)
                    renderer->drawBorderedRoundedRect(*storedX, *storedY, width, height, borderPx, radius, borderColor, &wheel);
                renderer->drawString(title, false, *storedX + inset, *storedY + titleHeight - inset, titleFont, titleColor);

                const auto drawRow = [&](int row, const char* label, const std::string& value, const tsl::Color& valueColor) {
                    const int y = *storedY + titleHeight + inset + row * (labelFont + 5);
                    renderer->drawString(label, false, *storedX + inset, y, labelFont, labelColor);
                    renderer->drawString(value.c_str(), false, valueX, y, labelFont, valueColor);
                };
                drawContent(drawRow, textColor);
            };

            drawCard(PerformanceCard, "Игра и нагрузка", [this](const auto& row, const tsl::Color& textColor) {
                const std::string fpsText = snapshot.gameRunning && snapshot.fpsAverage < 254.0f
                    ? (std::to_string(snapshot.fps) + " / " + std::to_string(static_cast<int>(std::lround(snapshot.fpsAverage))))
                    : "Нет данных";
                row(0, "FPS", fpsText, textColor);
                row(1, "Гц", snapshot.displayHz ? std::to_string(snapshot.displayHz) : "Нет данных", textColor);
                row(2, "ЦПУ", formatPercent((snapshot.cpuUsage[0] + snapshot.cpuUsage[1] + snapshot.cpuUsage[2] + snapshot.cpuUsage[3]) / 4.0f), textColor);
                row(3, "ГПУ", snapshot.hasGpuSensor ? formatPercent(snapshot.gpuLoadTenths / 10.0f) : "Нет данных", textColor);
            });

            drawCard(SystemCard, "Частоты и ОЗУ", [this](const auto& row, const tsl::Color& textColor) {
                row(0, "ЦПУ", formatMHz(snapshot.cpuHz), textColor);
                row(1, "ГПУ", formatMHz(snapshot.gpuHz), textColor);
                row(2, "ОЗУ", formatMiB(snapshot.ramUsedBytes) + " / " + formatMiB(snapshot.ramTotalBytes), textColor);
                row(3, "Шина", snapshot.ramBandwidthMBs ? std::to_string(snapshot.ramBandwidthMBs) + " МБ/с" : "Нет данных", textColor);
            });

            drawCard(ThermalsCard, "Тепло", [this](const auto& row, const tsl::Color& textColor) {
                const auto thermalColor = [this, &textColor](float value) {
                    return settings.useDynamicColors ? tsl::GradientColor(value, tsl::DEFAULT_TEMP_RANGE_HIGH) : textColor;
                };
                row(0, "ЦПУ", formatTemperature(snapshot.cpuTempMilliC / 1000.0f), thermalColor(snapshot.cpuTempMilliC / 1000.0f));
                row(1, "ГПУ", formatTemperature(snapshot.gpuTempMilliC / 1000.0f), thermalColor(snapshot.gpuTempMilliC / 1000.0f));
                row(2, "ОЗУ", formatTemperature(snapshot.ramTempMilliC / 1000.0f), thermalColor(snapshot.ramTempMilliC / 1000.0f));
                row(3, "Плата", formatTemperature(snapshot.pcbTemp), thermalColor(snapshot.pcbTemp));
            });

            drawCard(PowerCard, "Питание", [this](const auto& row, const tsl::Color& textColor) {
                char powerText[32];
                snprintf(powerText, sizeof(powerText), "%.2f Вт", snapshot.batteryPower);
                char voltageText[32];
                snprintf(voltageText, sizeof(voltageText), "%.2f В", snapshot.batteryVoltage);
                row(0, "Заряд", formatPercent(snapshot.batteryPercent), textColor);
                row(1, "Мощн.", powerText, textColor);
                row(2, "Напр.", voltageText, textColor);
                row(3, "Вент.", formatPercent(static_cast<float>(snapshot.fanPercent)), textColor);
            });
        });

        // Empty title and subtitle are deliberate: the only visible UI is the
        // user-positioned cards, so no framework heading hangs in another corner.
        auto* rootFrame = new tsl::elm::HeaderOverlayFrame("", "");
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

        HidTouchScreenState rawTouchState = {};
        const bool touchDetected = hidGetTouchScreenStates(&rawTouchState, 1) > 0 && rawTouchState.count > 0;
        const bool twoFingerTouch = touchDetected && rawTouchState.count >= 2;

        if (twoFingerTouch) {
            if (!pinchActive) {
                activeCard = findCardAt(rawTouchState.touches[0].x, rawTouchState.touches[0].y);
                if (activeCard >= 0) {
                    const float dx = static_cast<float>(rawTouchState.touches[1].x) - static_cast<float>(rawTouchState.touches[0].x);
                    const float dy = static_cast<float>(rawTouchState.touches[1].y) - static_cast<float>(rawTouchState.touches[0].y);
                    pinchStartDistance = std::max(std::sqrt(dx * dx + dy * dy), 1.0f);
                    int* ignoredX = nullptr;
                    int* ignoredY = nullptr;
                    uint8_t* selectedScale = nullptr;
                    getCardSettings(static_cast<CardId>(activeCard), ignoredX, ignoredY, selectedScale);
                    pinchStartPercent = *selectedScale;
                    pinchActive = true;
                }
            } else if (activeCard >= 0) {
                const float dx = static_cast<float>(rawTouchState.touches[1].x) - static_cast<float>(rawTouchState.touches[0].x);
                const float dy = static_cast<float>(rawTouchState.touches[1].y) - static_cast<float>(rawTouchState.touches[0].y);
                const float distance = std::sqrt(dx * dx + dy * dy);
                int* ignoredX = nullptr;
                int* ignoredY = nullptr;
                uint8_t* selectedScale = nullptr;
                getCardSettings(static_cast<CardId>(activeCard), ignoredX, ignoredY, selectedScale);
                const float gestureScale = std::clamp(distance / std::max(pinchStartDistance, 1.0f), 0.70f, 1.30f);
                *selectedScale = static_cast<uint8_t>(std::clamp<int>(std::lround(pinchStartPercent * gestureScale), 70, 150));
            }
            isDragging = false;
            touchWasDown = true;
            return pinchActive;
        }

        if (pinchActive) {
            if (activeCard >= 0) persistCard(static_cast<CardId>(activeCard), true);
            pinchActive = false;
            pinchStartDistance = 0.0f;
            activeCard = -1;
            touchWasDown = touchDetected;
            return true;
        }

        if (touchDetected) {
            const HidTouchState& touch = rawTouchState.touches[0];
            if (!touchWasDown) {
                touchWasDown = true;
                activeCard = findCardAt(touch.x, touch.y);
                if (activeCard >= 0) {
                    int* storedX = nullptr;
                    int* storedY = nullptr;
                    uint8_t* ignoredScale = nullptr;
                    getCardSettings(static_cast<CardId>(activeCard), storedX, storedY, ignoredScale);
                    dragStartTouch = touch;
                    dragStartX = *storedX;
                    dragStartY = *storedY;
                    isDragging = false;
                }
            }
            if (activeCard >= 0) {
                const int deltaX = static_cast<int>(touch.x) - static_cast<int>(dragStartTouch.x);
                const int deltaY = static_cast<int>(touch.y) - static_cast<int>(dragStartTouch.y);
                if (std::abs(deltaX) + std::abs(deltaY) >= 2) isDragging = true;
                if (isDragging) {
                    int* storedX = nullptr;
                    int* storedY = nullptr;
                    uint8_t* ignoredScale = nullptr;
                    getCardSettings(static_cast<CardId>(activeCard), storedX, storedY, ignoredScale);
                    const CardBounds& bounds = cardBounds[activeCard];
                    *storedX = std::clamp(dragStartX + deltaX, CARD_PADDING, SCREEN_WIDTH - bounds.width - CARD_PADDING);
                    *storedY = std::clamp(dragStartY + deltaY, CARD_PADDING, SCREEN_HEIGHT - bounds.height - CARD_PADDING);
                }
            }
        } else if (touchWasDown) {
            if (isDragging && activeCard >= 0) persistCard(static_cast<CardId>(activeCard), false);
            touchWasDown = false;
            isDragging = false;
            activeCard = -1;
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
