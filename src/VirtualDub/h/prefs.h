//	VirtualDub - Video processing and capture application
//	Copyright (C) 1998-2001 Avery Lee
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef f_PREFS_H
#define f_PREFS_H

///////////////////////////////////////////////////////////////////////////

enum {
	// The order of these must match the flags in cpuaccel.h!!
	OPTF_FORCE       = 0x01,
	OPTF_FPU         = 0x02,
	OPTF_MMX         = 0x04,
	OPTF_INTEGER_SSE = 0x08,  // Athlon MMX extensions or Intel SSE
	OPTF_SSE         = 0x10,  // Full SSE (PIII)
	OPTF_SSE2        = 0x20,  // (PIV)
	OPTF_3DNOW       = 0x40,
	OPTF_3DNOW_EXT   = 0x80,  // Athlon 3DNow! extensions
	OPTF_SSE3        = 0x100,
	OPTF_SSSE3       = 0x200,
	OPTF_SSE4_1      = 0x400,
	OPTF_AVX         = 0x800,
};

enum {
	kDisplayDither16     = 0x01,
	kDisplayDisableDX    = 0x02,
	kDisplayUseDXWithTS  = 0x04,
	kDisplayEnableD3D9   = 0x08,
	kDisplayEnableOpenGL = 0x10,
	kDisplayEnableD3D9FX = 0x20,
	kDisplayEnableVSync  = 0x40
};

enum {
	pref_time_hmst,
	pref_time_hmst_r,
	pref_time_m,
	pref_time_m_r,
	pref_time_s,
	pref_time_s_r,
	pref_time_ms,
	pref_time_ms_r,
	pref_time_r100,
};

enum {
	PREFERENCES_DISPLAY = 1,
	PREFERENCES_OPTF = 2,
	PREFERENCES_TIMELINE = 4,
};

///////////////////////////////////////////////////////////////////////////

void LoadPreferences();

#endif
