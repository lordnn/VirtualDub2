//	VirtualDub - Video processing and capture application
//	A/V interface library
//	Copyright (C) 1998-2004 Avery Lee
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

#ifndef f_VD2_RIZA_W32AUDIOCODEC_H
#define f_VD2_RIZA_W32AUDIOCODEC_H

#include <vd2/system/vdstl.h>
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <vector>
#include <vd2/Riza/audiocodec.h>

class VDAudioCodecW32 : public IVDAudioCodec {
public:
	VDAudioCodecW32();
	~VDAudioCodecW32() override;

	bool Init(const WAVEFORMATEX *pSrcFormat, const WAVEFORMATEX *pDstFormat, bool isCompression, const char *pShortNameDriverHint, bool throwOnError);
	void Shutdown() override;

	bool IsEnded() const override { return mbEnded; }

	unsigned	GetInputLevel() const override { return mBufferHdr.cbSrcLength; }
	unsigned	GetInputSpace() const override { return mInputBuffer.size() - mBufferHdr.cbSrcLength; }
	unsigned	GetOutputLevel() const override { return mBufferHdr.cbDstLengthUsed - mOutputReadPt; }
	const VDWaveFormat *GetOutputFormat() const override { return mDstFormat.data(); }
	unsigned	GetOutputFormatSize() const override { return mDstFormat.size(); }

	void		Restart() override;
	bool		Convert(bool flush, bool requireOutput) override;

	void		*LockInputBuffer(unsigned& bytes) override;
	void		UnlockInputBuffer(unsigned bytes) override;
	const void	*LockOutputBuffer(unsigned& bytes) override;
	void		UnlockOutputBuffer(unsigned bytes) override;
	unsigned	CopyOutput(void *dst, unsigned bytes) override;

protected:
	HACMDRIVER		mhDriver;
	HACMSTREAM		mhStream;
	vdstructex<VDWaveFormat>	mSrcFormat;
	vdstructex<VDWaveFormat>	mDstFormat;
	ACMSTREAMHEADER mBufferHdr;
	char mDriverName[64];
	char mDriverFilename[64];

	unsigned		mOutputReadPt;
	bool			mbFirst;
	bool			mbFlushing;
	bool			mbEnded;

	vdblock<char>	mInputBuffer;
	vdblock<char>	mOutputBuffer;
};

#endif
