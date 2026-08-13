/**
 * Copyright (C) 2020 werwolv
 *
 * This file is part of libtesla.
 *
 * libtesla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * libtesla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libtesla.  If not, see <http://www.gnu.org/licenses/>.
 */

// STB implementation must be defined BEFORE tesla.hpp is included,
// because tesla.hpp includes stb_truetype.h.  The include guard in
// stb_truetype.h ensures the header is processed only once, so the
// implementation code (outside the guard) is compiled here and only here.
#define STB_TRUETYPE_IMPLEMENTATION
#include "tesla.hpp"

// ─── __assert_func ──────────────────────────────────────────────────────────
// Provide the definition that libc expects; tesla.hpp only declares it.

extern "C" {
	void __assert_func(const char *_file, int _line, const char *_func, const char *_expr) {
		abort();
	}
}

// ─── tsl::cfg variable definitions ──────────────────────────────────────────
// These are declared as `extern` in tesla.hpp (inside namespace tsl::cfg)
// and must be defined exactly once, here.

namespace tsl::cfg {

	u16 LayerWidth        = 0;
	u16 LayerHeight       = 0;
	u16 LayerPosX         = 0;
	u16 LayerPosY         = 0;
	u16 FramebufferWidth  = 0;
	u16 FramebufferHeight = 0;
	u16 OrigLayerWidth    = 0;
	u16 OrigLayerHeight   = 0;
	u16 LayerMaxWidth     = 1280;
	u16 LayerMaxHeight    = 720;

}

// ─── libnx overrides ────────────────────────────────────────────────────────

extern "C" void __libnx_init_time(void);

extern "C" {

	u32          __nx_applet_type             = AppletType_None;
	u32          __nx_nv_transfermem_size     = 0x40000;
	ViLayerFlags __nx_vi_stray_layer_flags    = (ViLayerFlags)0;

	u32          __nx_fs_num_sessions         = 1;

	/**
	 * @brief libtesla service initializing function to override libnx's
	 */
	void __appInit(void) {
		tsl::hlp::doWithSmSession([]{
			ASSERT_FATAL(fsInitialize());
			ASSERT_FATAL(fsdevMountSdmc());
			ASSERT_FATAL(hidInitialize());      // Controller inputs and Touch
			if (hosversionAtLeast(16,0,0)) {
				ASSERT_FATAL(plInitialize(PlServiceType_User));       // Font data
			}
			else
				ASSERT_FATAL(plInitialize(PlServiceType_System));
			ASSERT_FATAL(pmdmntInitialize());   // PID querying
			ASSERT_FATAL(hidsysInitialize());   // Focus control
			ASSERT_FATAL(setsysInitialize());   // Settings querying
			ASSERT_FATAL(apmInitialize());
		});
		Service *plSrv = plGetServiceSession();
		Service plClone;
		ASSERT_FATAL(serviceClone(plSrv, &plClone));
		serviceClose(plSrv);
		*plSrv = plClone;
	}

	/**
	 * @brief libtesla service exiting function to override libnx's
	 */
	void __appExit(void) {
		apmExit();
		fsExit();
		hidExit();
		plExit();
		pmdmntExit();
		hidsysExit();
		setsysExit();
	}

}
