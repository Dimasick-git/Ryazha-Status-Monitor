/********************************************************************************
 * File: ryazha_audio.cpp
 * Description:
 *   Ryazha sound-feedback port for the Status-Monitor-Deux engine.
 *   Audio engine ported from libryazhahand / Ultrahand-Overlay:
 *   - rawBuf    : only per-sound allocation — compact native-channel 16-bit PCM.
 *   - m_playBuf : single shared DMA-ready buffer sized to the largest sound's
 *                 48 kHz stereo output.
 *   - blendSound() resamples to 48 kHz (linear interp), expands mono → L+R and
 *                 applies volume in one pass; supports WAV rates ≤ 48 kHz.
 *
 *   Licensed under both GPLv2 and CC-BY-4.0
 *   Copyright (c) 2025-2026 ppkantorski
 ********************************************************************************/

#include "ryazha_audio.hpp"
#include "ini_funcs.hpp"
#include <strings.h>

namespace ryz {

    // ── Static member definitions ─────────────────────────────────────────────
    bool                            Audio::m_initialized = false;
    std::atomic<bool>               Audio::m_enabled{true};
    std::atomic<int32_t>            Audio::m_masterVolumeFixed{154};  // 0.6 * 256 ≈ 154
    std::atomic<bool>               Audio::m_docked{false};
    std::vector<Audio::CachedSound> Audio::m_cachedSounds;
    std::mutex                      Audio::m_audioMutex;
    void*                           Audio::m_playBuf    = nullptr;
    uint32_t                        Audio::m_playBufCap = 0;
    AudioOutBuffer                  Audio::m_audoutBuf  = {};

    // 4 KB — required by Switch audout DMA
    static constexpr uint32_t AUDIO_ALIGN = 0x1000;
    static constexpr uint32_t TARGET_RATE = 48000;

    // ── initialize ────────────────────────────────────────────────────────────
    bool Audio::initialize() {
        std::lock_guard<std::mutex> lock(m_audioMutex);

        if (m_initialized) return true;

        if (R_FAILED(audoutInitialize()) || R_FAILED(audoutStartAudioOut())) {
            audoutExit();
            return false;
        }

        m_initialized = true;
        m_cachedSounds.resize(static_cast<uint32_t>(SoundType::Count));

        for (uint32_t i = 0; i < static_cast<uint32_t>(SoundType::Count); ++i)
            loadSoundFromWav(static_cast<SoundType>(i), m_soundPaths[i]);

        // Prime audout with a silent buffer: the very first audoutPlayBuffer
        // call pays a ~200-300 ms engine wake-up cost. Paying it here (before
        // the overlay is visible) keeps the first audible click instant.
        {
            void* primeBuf = aligned_alloc(AUDIO_ALIGN, AUDIO_ALIGN);
            if (primeBuf) {
                std::memset(primeBuf, 0, AUDIO_ALIGN);
                AudioOutBuffer prime = {};
                prime.buffer      = primeBuf;
                prime.buffer_size = AUDIO_ALIGN;
                prime.data_size   = AUDIO_ALIGN;
                prime.data_offset = 0;
                AudioOutBuffer* rel = nullptr;
                audoutPlayBuffer(&prime, &rel);
                free(primeBuf);
            }
        }

        return true;
    }

    // ── exit ──────────────────────────────────────────────────────────────────
    void Audio::exit() {
        std::lock_guard<std::mutex> lock(m_audioMutex);

        for (auto& s : m_cachedSounds) {
            free(s.rawBuf);
            s = CachedSound{};
        }

        free(m_playBuf);
        m_playBuf    = nullptr;
        m_playBufCap = 0;
        m_audoutBuf  = {};

        if (m_initialized) {
            audoutStopAudioOut();
            audoutExit();
            m_initialized = false;
        }
    }

    // ── reloadAllSounds ───────────────────────────────────────────────────────
    void Audio::reloadAllSounds() {
        std::lock_guard<std::mutex> lock(m_audioMutex);
        if (!m_initialized) return;
        for (uint32_t i = 0; i < static_cast<uint32_t>(SoundType::Count); ++i)
            loadSoundFromWav(static_cast<SoundType>(i), m_soundPaths[i]);
    }

    // ── growPlayBuf ───────────────────────────────────────────────────────────
    // Must hold m_audioMutex.
    void Audio::growPlayBuf() {
        uint32_t maxNeeded = 0;

        for (const auto& s : m_cachedSounds) {
            if (!s.rawBuf || s.rawSize == 0) continue;

            const uint32_t srcPerChan = s.isMono
                ? (s.rawSize / sizeof(s16))
                : (s.rawSize / sizeof(s16)) / 2;

            const uint32_t outPerChan = (s.sampleRate == TARGET_RATE || s.sampleRate == 0)
                ? srcPerChan
                : static_cast<uint32_t>(
                    ((uint64_t)srcPerChan * TARGET_RATE + s.sampleRate - 1) / s.sampleRate);

            const uint32_t stereoBytes = outPerChan * 2 * sizeof(s16);
            const uint32_t needed      = (stereoBytes + AUDIO_ALIGN - 1) & ~(AUDIO_ALIGN - 1);
            if (needed > maxNeeded) maxNeeded = needed;
        }

        if (maxNeeded <= m_playBufCap) return;

        free(m_playBuf);
        m_playBuf = aligned_alloc(AUDIO_ALIGN, maxNeeded);
        m_playBufCap = m_playBuf ? maxNeeded : 0;
    }

    // ── blendSound ────────────────────────────────────────────────────────────
    // Must be called under m_audioMutex.
    uint32_t Audio::blendSound(const CachedSound& s, s16* dst, int32_t vol,
                               uint32_t primFrames, bool mixMode) {
        if (!s.rawBuf || s.rawSize == 0) return 0;

        const uint32_t srcSamples = s.rawSize / sizeof(s16);
        const uint32_t srcPerChan = s.isMono ? srcSamples : srcSamples / 2;
        const uint32_t outPerChan = (s.sampleRate == TARGET_RATE || s.sampleRate == 0)
            ? srcPerChan
            : static_cast<uint32_t>(
                ((uint64_t)srcPerChan * TARGET_RATE + s.sampleRate - 1) / s.sampleRate);

        const s16*     src           = static_cast<const s16*>(s.rawBuf);
        const bool     needsResample = (s.sampleRate != TARGET_RATE && s.sampleRate != 0);
        const uint64_t step          = needsResample
            ? (((uint64_t)s.sampleRate << 16) / TARGET_RATE)
            : 0;
        uint64_t srcFixed = 0;

        for (uint32_t i = 0; i < outPerChan; ++i) {
            int32_t l, r;

            if (needsResample) {
                const uint32_t i0   = static_cast<uint32_t>(srcFixed >> 16);
                const uint32_t i1   = (i0 + 1 < srcPerChan) ? i0 + 1 : i0;
                const int32_t  frac = static_cast<int32_t>(srcFixed & 0xFFFF);
                if (s.isMono) {
                    const int32_t s0 = src[i0], s1 = src[i1];
                    l = r = ((s0 + (((s1 - s0) * frac) >> 16)) * vol) >> 8;
                } else {
                    const int32_t l0 = src[i0*2],     l1 = src[i1*2];
                    const int32_t r0 = src[i0*2 + 1], r1 = src[i1*2 + 1];
                    l = ((l0 + (((l1 - l0) * frac) >> 16)) * vol) >> 8;
                    r = ((r0 + (((r1 - r0) * frac) >> 16)) * vol) >> 8;
                }
                srcFixed += step;
            } else {
                if (s.isMono) {
                    l = r = (static_cast<int32_t>(src[i]) * vol) >> 8;
                } else {
                    l = (static_cast<int32_t>(src[i*2])     * vol) >> 8;
                    r = (static_cast<int32_t>(src[i*2 + 1]) * vol) >> 8;
                }
            }

            if (mixMode && i < primFrames) {
                dst[i*2]     = static_cast<s16>(std::clamp<int32_t>(dst[i*2]     + l, INT16_MIN, INT16_MAX));
                dst[i*2 + 1] = static_cast<s16>(std::clamp<int32_t>(dst[i*2 + 1] + r, INT16_MIN, INT16_MAX));
            } else {
                dst[i*2]     = static_cast<s16>(l);
                dst[i*2 + 1] = static_cast<s16>(r);
            }
        }

        return outPerChan * 2u * sizeof(s16);
    }

    // ── loadSoundFromWav ──────────────────────────────────────────────────────
    // Must be called under m_audioMutex.
    bool Audio::loadSoundFromWav(SoundType type, const char* path) {
        const uint32_t idx = static_cast<uint32_t>(type);
        if (!m_initialized || idx >= static_cast<uint32_t>(SoundType::Count)) return false;

        CachedSound& s = m_cachedSounds[idx];

        free(s.rawBuf);
        s = CachedSound{};

        FILE* f = fopen(path, "rb");
        if (!f) return false;

        char hdr[12];
        if (fread(hdr, 1, 12, f) != 12 ||
            memcmp(hdr,     "RIFF", 4) ||
            memcmp(hdr + 8, "WAVE", 4)) {
            fclose(f); return false;
        }

        u16  fmt = 0, ch = 0, bits = 0;
        u32  rate = 0, dSize = 0;
        long dPos = 0;

        while (fread(hdr, 1, 8, f) == 8) {
            u32 sz = 0;
            memcpy(&sz, hdr + 4, sizeof(sz));
            if (!memcmp(hdr, "fmt ", 4)) {
                fread(&fmt,  2, 1, f);
                fread(&ch,   2, 1, f);
                fread(&rate, 4, 1, f);
                fseek(f, 6, SEEK_CUR);  // skip byte rate + block align
                fread(&bits, 2, 1, f);
                fseek(f, (long)sz - 16, SEEK_CUR);
            } else if (!memcmp(hdr, "data", 4)) {
                dSize = sz;
                dPos  = ftell(f);
                break;
            } else {
                fseek(f, sz, SEEK_CUR);
            }
        }

        // Reject rates above 48 kHz — downsampling would expand rawBuf beyond
        // its target-rate output and defeat the purpose of small source files.
        if (!dSize || fmt != 1 || ch == 0 || ch > 2 ||
            (bits != 8 && bits != 16) || rate == 0 || rate > TARGET_RATE) {
            fclose(f); return false;
        }

        const uint32_t inSamples = dSize / (bits / 8);
        const uint32_t rawBytes  = inSamples * sizeof(s16);
        const uint32_t rawCap    = (rawBytes + AUDIO_ALIGN - 1) & ~(AUDIO_ALIGN - 1);

        void* buf = aligned_alloc(AUDIO_ALIGN, rawCap);
        if (!buf) { fclose(f); return false; }

        fseek(f, dPos, SEEK_SET);
        s16*     out       = static_cast<s16*>(buf);
        uint32_t remaining = inSamples;
        uint32_t outIdx    = 0;

        constexpr uint32_t CHUNK = 512;

        if (bits == 8) {
            u8 chunk[CHUNK];
            while (remaining > 0) {
                const uint32_t toRead = std::min(remaining, CHUNK);
                if (fread(chunk, 1, toRead, f) != toRead) {
                    free(buf); fclose(f); return false;
                }
                for (uint32_t i = 0; i < toRead; ++i)
                    out[outIdx++] = static_cast<s16>((static_cast<int32_t>(chunk[i]) - 128) << 8);
                remaining -= toRead;
            }
        } else {
            s16 chunk[CHUNK];
            while (remaining > 0) {
                const uint32_t toRead = std::min(remaining, CHUNK);
                if (fread(chunk, sizeof(s16), toRead, f) != toRead) {
                    free(buf); fclose(f); return false;
                }
                memcpy(out + outIdx, chunk, toRead * sizeof(s16));
                outIdx    += toRead;
                remaining -= toRead;
            }
        }

        fclose(f);

        if (rawBytes < rawCap)
            memset(static_cast<u8*>(buf) + rawBytes, 0, rawCap - rawBytes);

        s.rawBuf     = buf;
        s.rawSize    = rawBytes;
        s.rawCap     = rawCap;
        s.sampleRate = rate;
        s.isMono     = (ch == 1);

        growPlayBuf();
        return (m_playBuf != nullptr);
    }

    // ── submitPlayBuf ─────────────────────────────────────────────────────────
    // Must be called under m_audioMutex after a successful render.
    void Audio::submitPlayBuf(uint32_t outBytes) {
        const uint32_t bufCap   = (outBytes + AUDIO_ALIGN - 1) & ~(AUDIO_ALIGN - 1);
        m_audoutBuf             = {};
        m_audoutBuf.buffer      = m_playBuf;
        m_audoutBuf.buffer_size = bufCap;
        m_audoutBuf.data_size   = outBytes;
        m_audoutBuf.data_offset = 0;
        m_audoutBuf.next        = nullptr;
        AudioOutBuffer* rel     = nullptr;
        audoutPlayBuffer(&m_audoutBuf, &rel);
    }

    // ── playSoundImpl ─────────────────────────────────────────────────────────
    void Audio::playSoundImpl(const SoundType* types, uint32_t count) {
        std::lock_guard<std::mutex> lock(m_audioMutex);
        if (!m_initialized || !m_playBuf || count == 0) return;

        int32_t vol = m_masterVolumeFixed.load(std::memory_order_relaxed);
        if (m_docked.load(std::memory_order_relaxed)) vol >>= 1;

        AudioOutBuffer* released = nullptr;
        u32 releasedCount        = 0;
        audoutGetReleasedAudioOutBuffer(&released, &releasedCount);

        s16*     dst          = static_cast<s16*>(m_playBuf);
        uint32_t filledFrames = 0;
        uint32_t totalBytes   = 0;

        const uint32_t kCount = static_cast<uint32_t>(SoundType::Count);

        for (uint32_t i = 0; i < count; ++i) {
            const uint32_t idx = static_cast<uint32_t>(types[i]);
            if (idx >= kCount) continue;
            const CachedSound& s = m_cachedSounds[idx];

            const bool     mixMode  = (filledFrames > 0);
            const uint32_t outBytes = blendSound(s, dst, vol, filledFrames, mixMode);
            if (outBytes == 0) continue;

            const uint32_t outFrames = outBytes / (2u * sizeof(s16));
            if (outFrames > filledFrames) filledFrames = outFrames;
            if (outBytes  > totalBytes)   totalBytes   = outBytes;
        }

        if (totalBytes == 0) return;

        const uint32_t needed = (totalBytes + AUDIO_ALIGN - 1) & ~(AUDIO_ALIGN - 1);
        if (totalBytes < needed)
            memset(static_cast<u8*>(m_playBuf) + totalBytes, 0, needed - totalBytes);

        submitPlayBuf(totalBytes);
    }

    void Audio::playSound(SoundType type) {
        if (!m_enabled.load(std::memory_order_relaxed)) return;
        playSoundImpl(&type, 1);
    }

    void Audio::playSounds(const SoundType* types, uint32_t count) {
        if (!m_enabled.load(std::memory_order_relaxed)) return;
        if (!types || count == 0) return;
        playSoundImpl(types, count);
    }

    void Audio::setMasterVolume(float v) {
        const int32_t fixed = static_cast<int32_t>(std::clamp(v, 0.0f, 1.0f) * 256.0f);
        m_masterVolumeFixed.store(fixed, std::memory_order_relaxed);
    }

    void Audio::setEnabled(bool e) {
        m_enabled.store(e, std::memory_order_relaxed);
    }

    bool Audio::isEnabled() {
        return m_enabled.load(std::memory_order_relaxed);
    }

    void Audio::setDocked(bool docked) {
        m_docked.store(docked, std::memory_order_relaxed);
    }

    // ── RyazhaSound facade ────────────────────────────────────────────────────
    // Uses libnx Thread/Mutex/CondVar instead of std::thread: the overlay wraps
    // all C++ exception entry points into __builtin_unreachable(), and the
    // std::thread constructor throws on failure.
    std::atomic<bool> RyazhaSound::m_running{false};
    Thread            RyazhaSound::m_worker;
    Mutex             RyazhaSound::m_queueMutex = {};
    CondVar           RyazhaSound::m_queueCv = {};
    Audio::SoundType  RyazhaSound::m_queue[RyazhaSound::QUEUE_CAP];
    uint32_t          RyazhaSound::m_queueCount = 0;
    bool              RyazhaSound::m_soundsActive  = false;
    bool              RyazhaSound::m_hapticsActive = false;
    bool              RyazhaSound::m_useNavigate = true;
    bool              RyazhaSound::m_useEnter    = true;
    bool              RyazhaSound::m_useExit     = true;
    HidVibrationDeviceHandle RyazhaSound::m_vibHandheld[2] = {};
    HidVibrationDeviceHandle RyazhaSound::m_vibPlayer1[2]  = {};
    u32               RyazhaSound::m_handheldStyle = 0;
    u32               RyazhaSound::m_player1Style  = 0;

    // Rumble presets ported from libryazhahand's haptics engine.
    static constexpr HidVibrationValue kRumblePreset = {
        .amp_low = 0.80f, .freq_low = 210.0f, .amp_high = 0.00f, .freq_high = 210.0f
    };
    static constexpr HidVibrationValue kRumblePresetHandheld = {
        .amp_low = 0.65f, .freq_low = 210.0f, .amp_high = 0.00f, .freq_high = 210.0f
    };
    static constexpr HidVibrationValue kRumbleStop{};

    static bool ryazhaConfigBool(const char* key, bool defaultValue) {
        std::string v = parseValueFromIniSection("/config/ryazhahand/config.ini", "ryazhahand", key);
        if (v.empty()) return defaultValue;
        return !(strcasecmp(v.c_str(), "false") == 0 || v == "0");
    }

    void RyazhaSound::initHaptics() {
        const u32 handheldStyle = hidGetNpadStyleSet(HidNpadIdType_Handheld);
        const u32 player1Style  = hidGetNpadStyleSet(HidNpadIdType_No1);

        static HidVibrationDeviceHandle tmp[2];
        if (handheldStyle) {
            if (R_SUCCEEDED(hidInitializeVibrationDevices(tmp, 2, HidNpadIdType_Handheld,
                                                          (HidNpadStyleTag)handheldStyle))) {
                m_vibHandheld[0] = tmp[0];
                m_vibHandheld[1] = tmp[1];
            } else {
                m_hapticsActive = false;
            }
        }
        if (player1Style) {
            if (R_SUCCEEDED(hidInitializeVibrationDevices(tmp, 2, HidNpadIdType_No1,
                                                          (HidNpadStyleTag)player1Style))) {
                m_vibPlayer1[0] = tmp[0];
                m_vibPlayer1[1] = tmp[1];
            }
        }
        m_handheldStyle = handheldStyle;
        m_player1Style  = player1Style;
    }

    void RyazhaSound::rumbleClick() {
        const u32 handheldStyle = hidGetNpadStyleSet(HidNpadIdType_Handheld);
        const u32 player1Style  = hidGetNpadStyleSet(HidNpadIdType_No1);
        if (handheldStyle != m_handheldStyle || player1Style != m_player1Style)
            initHaptics();
        if (!m_hapticsActive) return;

        const HidVibrationValue& preset = m_handheldStyle ? kRumblePresetHandheld : kRumblePreset;
        const HidVibrationValue values[2] = { preset, preset };
        const HidVibrationValue stops[2]  = { kRumbleStop, kRumbleStop };

        if (m_player1Style) {
            hidSendVibrationValues(m_vibPlayer1, values, 2);
        } else if (m_handheldStyle) {
            hidSendVibrationValues(m_vibHandheld, values, 2);
        } else {
            return;
        }
        svcSleepThread(30'000'000ULL); // 30 ms click
        if (m_player1Style)       hidSendVibrationValues(m_vibPlayer1, stops, 2);
        else if (m_handheldStyle) hidSendVibrationValues(m_vibHandheld, stops, 2);
    }

    bool RyazhaSound::soundAllowed(Audio::SoundType type) {
        switch (type) {
            case Audio::SoundType::Navigate: return m_useNavigate;
            case Audio::SoundType::Enter:    return m_useEnter;
            case Audio::SoundType::Exit:     return m_useExit;
            default:                         return true;
        }
    }

    void RyazhaSound::start() {
        if (m_running.load(std::memory_order_relaxed)) return;

        m_soundsActive  = ryazhaConfigBool("sound_effects", true) && Audio::anySoundExists();
        m_hapticsActive = ryazhaConfigBool("haptic_feedback", true);
        if (!m_soundsActive && !m_hapticsActive) return;

        m_useNavigate = ryazhaConfigBool("sound_navigation", true);
        m_useEnter    = ryazhaConfigBool("sound_enter", true);
        m_useExit     = ryazhaConfigBool("sound_exit", true);

        if (m_soundsActive) {
            std::string vol = parseValueFromIniSection("/config/ryazhahand/config.ini", "ryazhahand", "sound_volume");
            if (!vol.empty()) {
                unsigned long pct = strtoul(vol.c_str(), nullptr, 10);
                if (pct > 100) pct = 100;
                Audio::setMasterVolume(pct / 100.0f);
            }
            if (!Audio::initialize())
                m_soundsActive = false;
        }
        if (m_hapticsActive)
            initHaptics();
        if (!m_soundsActive && !m_hapticsActive) return;

        mutexInit(&m_queueMutex);
        condvarInit(&m_queueCv);
        m_queueCount = 0;

        // m_running must be true before the worker starts, otherwise the
        // worker's wait loop reads it as "stopped" and exits immediately.
        m_running.store(true, std::memory_order_relaxed);

        // Same core (#3) and low priority as the data-collection threads.
        if (R_FAILED(threadCreate(&m_worker, workerLoop, nullptr, nullptr, 0x2000, 0x3F, -2)) ||
            R_FAILED(threadStart(&m_worker))) {
            m_running.store(false, std::memory_order_relaxed);
            Audio::exit();
            return;
        }
    }

    void RyazhaSound::stop() {
        if (!m_running.exchange(false, std::memory_order_relaxed)) return;
        condvarWakeAll(&m_queueCv);
        threadWaitForExit(&m_worker);
        threadClose(&m_worker);
        Audio::exit();
    }

    void RyazhaSound::trigger(Audio::SoundType type) {
        if (!m_running.load(std::memory_order_relaxed)) return;
        mutexLock(&m_queueMutex);
        // Coalesce bursts: keep the queue tiny so a held d-pad doesn't build
        // up a backlog of stale ticks.
        if (m_queueCount < QUEUE_CAP)
            m_queue[m_queueCount++] = type;
        mutexUnlock(&m_queueMutex);
        condvarWakeOne(&m_queueCv);
    }

    void RyazhaSound::workerLoop(void*) {
        for (;;) {
            Audio::SoundType pending[QUEUE_CAP];
            uint32_t count = 0;

            mutexLock(&m_queueMutex);
            while (m_queueCount == 0 && m_running.load(std::memory_order_relaxed))
                condvarWait(&m_queueCv, &m_queueMutex);
            if (!m_running.load(std::memory_order_relaxed)) {
                mutexUnlock(&m_queueMutex);
                return;
            }
            count = m_queueCount;
            memcpy(pending, m_queue, count * sizeof(Audio::SoundType));
            m_queueCount = 0;
            mutexUnlock(&m_queueMutex);

            if (m_hapticsActive)
                rumbleClick();

            if (m_soundsActive) {
                Audio::SoundType allowed[QUEUE_CAP];
                uint32_t allowedCount = 0;
                for (uint32_t i = 0; i < count; ++i)
                    if (soundAllowed(pending[i]))
                        allowed[allowedCount++] = pending[i];
                if (allowedCount == 1)     Audio::playSound(allowed[0]);
                else if (allowedCount > 1) Audio::playSounds(allowed, allowedCount);
            }
        }
    }
}
