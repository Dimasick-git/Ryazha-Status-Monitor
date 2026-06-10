/********************************************************************************
 * File: ryazha_audio.hpp
 * Description:
 *   Ryazha sound-feedback port for the Status-Monitor-Deux engine.
 *
 *   The Audio class is ported from libryazhahand (Dimasick-git/libryazhahand,
 *   itself derived from ppkantorski's Ultrahand-Overlay audio engine):
 *   render-thread-safe WAV playback through libnx audout with a single shared
 *   DMA buffer. Sounds are read from the shared Ryazhahand pack directory
 *   (sdmc:/config/ryazhahand/.loaded_sounds/) so the sound pack selected in
 *   the Ryazhahand menu applies to this overlay too.
 *
 *   RyazhaSound is a small facade that owns a dedicated playback thread —
 *   audoutPlayBuffer blocks for the WAV duration, so UI input handlers only
 *   signal the worker instead of blocking the render loop.
 *
 *   Licensed under both GPLv2 and CC-BY-4.0
 *   Copyright (c) 2025-2026 ppkantorski
 ********************************************************************************/

#pragma once
#include <switch.h>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <atomic>
#include <cstring>
#include <mutex>
#include <sys/stat.h>

namespace ryz {
    class Audio {
    public:
        enum class SoundType : uint8_t {
            Navigate,
            Enter,
            Exit,
            Wall,
            On,
            Off,
            Settings,
            Move,
            Notification,
            Count
        };

        struct CachedSound {
            // Compact raw PCM — native channel count, 16-bit, no volume applied.
            void*    rawBuf     = nullptr;
            uint32_t rawSize    = 0;     // actual data bytes
            uint32_t rawCap     = 0;     // allocated (aligned) bytes
            uint32_t sampleRate = 48000; // native rate read from WAV header
            bool     isMono     = false;
        };

        static bool initialize();
        static void exit();

        static inline bool anySoundExists() {
            struct stat st;
            for (const auto& path : m_soundPaths)
                if (stat(path, &st) == 0) return true;
            return false;
        }

        static void playSound(SoundType type);
        static void playSounds(const SoundType* types, uint32_t count);

        static void setMasterVolume(float volume);
        static void setEnabled(bool enabled);
        static bool isEnabled();
        static void setDocked(bool docked);
        static void reloadAllSounds();

        static std::mutex m_audioMutex;
        static bool       m_initialized;

    private:
        static std::atomic<bool>        m_enabled;
        static std::atomic<int32_t>     m_masterVolumeFixed;  // volume as 0–256 fixed-point
        static std::atomic<bool>        m_docked;
        static std::vector<CachedSound> m_cachedSounds;

        // Single shared DMA playback buffer — sized to the largest sound's
        // 48 kHz stereo output. Reused on every playSound(); safe because
        // audout is always drained before the buffer is written.
        static void*          m_playBuf;
        static uint32_t       m_playBufCap;
        static AudioOutBuffer m_audoutBuf;

        // The Ryazhahand menu unpacks the selected ZIP pack from the visible
        // /config/ryazhahand/sounds/ into .loaded_sounds/ — shared by every
        // overlay in the Ryazhahand ecosystem.
        inline static constexpr const char* m_soundPaths[static_cast<size_t>(SoundType::Count)] = {
            "sdmc:/config/ryazhahand/.loaded_sounds/tick.wav",
            "sdmc:/config/ryazhahand/.loaded_sounds/enter.wav",
            "sdmc:/config/ryazhahand/.loaded_sounds/exit.wav",
            "sdmc:/config/ryazhahand/.loaded_sounds/wall.wav",
            "sdmc:/config/ryazhahand/.loaded_sounds/on.wav",
            "sdmc:/config/ryazhahand/.loaded_sounds/off.wav",
            "sdmc:/config/ryazhahand/.loaded_sounds/settings.wav",
            "sdmc:/config/ryazhahand/.loaded_sounds/move.wav",
            "sdmc:/config/ryazhahand/.loaded_sounds/notification.wav"
        };

        static bool     loadSoundFromWav(SoundType type, const char* path);
        static void     growPlayBuf();
        static uint32_t blendSound(const CachedSound& s, s16* dst, int32_t vol,
                                   uint32_t primFrames, bool mixMode);
        static void     submitPlayBuf(uint32_t outBytes);
        static void     playSoundImpl(const SoundType* types, uint32_t count);
    };

    // ── RyazhaSound: async feedback facade (sound + haptics) ──────────────────
    // start() reads /config/ryazhahand/config.ini ([ryazhahand] section):
    //   sound_effects    — sound master switch (default true)
    //   sound_volume     — 0..100 (default 60)
    //   sound_navigation / sound_enter / sound_exit — per-event sound toggles
    //   haptic_feedback  — rumble click on UI events (default true)
    // Haptics are ported from libryazhahand: a short rumble click is sent to
    // the handheld/player-1 vibration devices alongside each UI sound.
    class RyazhaSound {
    public:
        static void start();
        static void stop();

        static void trigger(Audio::SoundType type);

        static inline void navigate() { trigger(Audio::SoundType::Navigate); }
        static inline void enter()    { trigger(Audio::SoundType::Enter);    }
        static inline void back()     { trigger(Audio::SoundType::Exit);     }

        static inline bool active() { return m_running.load(std::memory_order_relaxed); }

    private:
        static void workerLoop(void* arg);
        static void initHaptics();
        static void rumbleClick();
        static bool soundAllowed(Audio::SoundType type);

        static constexpr uint32_t QUEUE_CAP = 4;

        static std::atomic<bool> m_running;
        static Thread            m_worker;
        static Mutex             m_queueMutex;
        static CondVar           m_queueCv;
        static Audio::SoundType  m_queue[QUEUE_CAP];
        static uint32_t          m_queueCount;

        static bool m_soundsActive;
        static bool m_hapticsActive;
        static bool m_useNavigate;
        static bool m_useEnter;
        static bool m_useExit;

        static HidVibrationDeviceHandle m_vibHandheld[2];
        static HidVibrationDeviceHandle m_vibPlayer1[2];
        static u32 m_handheldStyle;
        static u32 m_player1Style;
    };
}
