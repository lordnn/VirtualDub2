//	VirtualDub - Video processing and capture application
//	Copyright (C) 1998-2003 Avery Lee
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

#ifndef f_DUBOUTPUT_H
#define f_DUBOUTPUT_H

#include <vector>
#include <vd2/system/VDString.h>
#include <vd2/system/vdalloc.h>
#include <vd2/Kasumi/pixmaputils.h>
#include "AVIStripeSystem.h"
#include <vd2/plugin/vdinputdriver.h>
#include "dub.h"
#include "fixes.h"

class IVDMediaOutput;
class VDExtEncProfile;
class IVDOutputDriver;

class VDINTERFACE IVDDubberOutputSystem {
public:
	virtual IVDMediaOutput *CreateSegment() = 0;
	virtual void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize = true) = 0;
	virtual void SetVideo(const VDXStreamInfo& si, const void *pFormat, int cbFormat) = 0;
	virtual void SetVideoImageLayout(const VDXStreamInfo& si, const VDPixmapLayout& layout) = 0;
	virtual void SetAudio(const VDXStreamInfo& si, const void *pFormat, int cbFormat, bool bInterleaved, bool vbr) = 0;
	virtual bool AcceptsVideo() = 0;
	virtual bool AcceptsAudio() = 0;
	virtual bool IsRealTime() = 0;
	virtual bool IsVideoImageOutputEnabled() = 0;
	virtual bool IsVideoImageOutputRequired() = 0;
	virtual bool AreNullFramesAllowed() = 0;
	virtual bool IsVideoCompressionEnabled() = 0;
	virtual int GetVideoOutputFormatOverride(int last_format) = 0;

	virtual bool IsCompressedAudioAllowed() = 0;
	virtual bool GetInterleavingOverride(DubAudioOptions& opt) = 0;
	virtual bool IsNull() = 0;
	virtual bool GetStreamControl(VDXStreamControl& sc){ return true; }
	virtual const char* GetFormatName() = 0;
};

class VDDubberOutputSystem : public IVDDubberOutputSystem {
public:
	VDDubberOutputSystem();
	~VDDubberOutputSystem();

	void SetVideo(const VDXStreamInfo& si, const void *pFormat, int cbFormat) override;
	void SetVideoImageLayout(const VDXStreamInfo& si, const VDPixmapLayout& layout) override;
	void SetAudio(const VDXStreamInfo& si, const void *pFormat, int cbFormat, bool bInterleaved, bool vbr) override;
	bool AcceptsVideo() override { return false; }
	bool AcceptsAudio() override { return false; }
	bool IsRealTime() override { return false; }
	bool IsVideoImageOutputEnabled() override { return false; }
	bool IsVideoImageOutputRequired() override { return false; }
	bool AreNullFramesAllowed() override { return false; }
	bool IsVideoCompressionEnabled() override { return false; }
	int GetVideoOutputFormatOverride(int last_format) override { return 0; }

	bool IsCompressedAudioAllowed() override { return true; }
	bool GetInterleavingOverride(DubAudioOptions& opt) override { return false; }
	bool IsNull() override { return false; }

protected:
	VDXStreamInfo			mVideoStreamInfo;
	vdfastvector<char>		mVideoFormat;
	VDPixmapLayout			mVideoImageLayout;
	VDXStreamInfo			mAudioStreamInfo;
	vdfastvector<char>		mAudioFormat;
	bool					mbAudioVBR;
	bool					mbAudioInterleaved;
};

class VDAVIOutputFileSystem : public VDDubberOutputSystem {
public:
	VDAVIOutputFileSystem();
	~VDAVIOutputFileSystem();

	void SetCaching(bool bAllowOSCaching);
	void SetIndexing(bool bAllowHierarchicalExtensions);
	void Set1GBLimit(bool bUse1GBLimit);
	void SetBuffer(int bufferSize);
	void SetTextInfo(const std::list<std::pair<uint32, VDStringA> >& info);

	void SetFilename(const wchar_t *pszFilename);
	void SetFilenamePattern(const wchar_t *pszSegmentPrefix, const wchar_t *pszExt, int nMinimumDigits);

	IVDMediaOutput *CreateSegment() override;
	void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize) override;
	void SetVideo(const VDXStreamInfo& si, const void *pFormat, int cbFormat) override;
	bool AcceptsVideo() override;
	bool AcceptsAudio() override;
	bool AreNullFramesAllowed() override { return true; }
	bool IsVideoCompressionEnabled() override { return true; }
	const char* GetFormatName() override { return "avi"; }

private:
	VDStringW	mSegmentBaseName;
	VDStringW	mSegmentExt;
	int			mSegmentDigits;
	int			mCurrentSegment;
	int			mBufferSize;
	int			mAlignment;
	bool		mbAllowCaching;
	bool		mbAllowIndexing;
	bool		mbUse1GBLimit;

	typedef std::list<std::pair<uint32, VDStringA> > tTextInfo;
	tTextInfo	mTextInfo;
};

class VDAVIOutputStripedSystem : public VDDubberOutputSystem {
public:
	VDAVIOutputStripedSystem(const wchar_t *pszFilename);
	~VDAVIOutputStripedSystem();

	void Set1GBLimit(bool bUse1GBLimit);

	IVDMediaOutput *CreateSegment() override;
	void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize) override;
	bool AcceptsVideo() override;
	bool AcceptsAudio() override;
	bool AreNullFramesAllowed() override { return true; }
	bool IsVideoCompressionEnabled() override { return true; }
	const char* GetFormatName() override { return "avi"; }

private:
	bool		mbUse1GBLimit;

	vdautoptr<AVIStripeSystem>	mpStripeSystem;
};

class VDAVIOutputWAVSystem : public VDDubberOutputSystem {
public:
	VDAVIOutputWAVSystem(const wchar_t *pszFilename, bool auto_w64=true);
	~VDAVIOutputWAVSystem();

	void SetBuffer(int size);
	IVDMediaOutput *CreateSegment() override;
	void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize) override;
	bool AcceptsAudio() override;
	const char* GetFormatName() override { return "wav"; }

private:
	VDStringW	mFilename;
	int			mBufferSize;
	bool		auto_w64;
};

class VDAVIOutputPluginSystem : public VDDubberOutputSystem {
public:
	bool fAudioOnly;

	VDAVIOutputPluginSystem(const wchar_t *pszFilename);
	~VDAVIOutputPluginSystem();

	IVDMediaOutput *CreateSegment() override;
	void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize) override;
	bool AcceptsVideo() override { return !fAudioOnly; }
	bool AcceptsAudio() override { return true; }
	bool IsVideoCompressionEnabled() override { return true; }
	bool AreNullFramesAllowed() override { return true; }
	bool GetInterleavingOverride(DubAudioOptions& opt);
	void SetTextInfo(const std::list<std::pair<uint32, VDStringA> >& info);
	void SetDriver(IVDOutputDriver* driver, const char* format) {
		this->driver = driver;
		this->format = format;
	}
	bool GetStreamControl(VDXStreamControl& sc) override;
	const char* GetFormatName() override;

private:
	VDStringW	mFilename;
	typedef std::list<std::pair<uint32, VDStringA> > tTextInfo;
	tTextInfo	mTextInfo;
	IVDOutputDriver* driver;
	VDStringA format;
};

class VDAVIOutputRawSystem : public VDDubberOutputSystem {
public:
	VDAVIOutputRawSystem(const wchar_t *pszFilename);
	~VDAVIOutputRawSystem();

	void SetBuffer(int size);
	IVDMediaOutput *CreateSegment() override;
	void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize) override;
	bool AcceptsAudio() override;
	bool AreNullFramesAllowed() override { return true; }
	const char* GetFormatName() override { return nullptr; }

private:
	VDStringW	mFilename;
	int			mBufferSize;
};

struct VDAVIOutputRawVideoFormat {
	int		mOutputFormat;
	uint32	mScanlineAlignment;
	bool	mbSwapChromaPlanes;
	bool	mbBottomUp;
};

void InitOutputFormat(VDAVIOutputRawVideoFormat& format, const VDExtEncProfile* vp);

class VDAVIOutputRawVideoSystem : public VDDubberOutputSystem {
public:
	VDAVIOutputRawVideoSystem(const wchar_t *pszFilename, const VDAVIOutputRawVideoFormat& format);
	~VDAVIOutputRawVideoSystem();

	void SetBuffer(int size);
	IVDMediaOutput *CreateSegment() override;
	void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize) override;
	bool AcceptsVideo() override;
	bool IsVideoImageOutputEnabled() override { return true; }
	bool IsVideoImageOutputRequired() override { return true; }
	int GetVideoOutputFormatOverride(int last_format) override;
	const char* GetFormatName() override { return nullptr; }

private:
	VDStringW	mFilename;
	int			mBufferSize;
	VDAVIOutputRawVideoFormat	mFormat;
};

class VDAVIOutputCLISystem : public VDDubberOutputSystem {
	VDAVIOutputCLISystem(const VDAVIOutputCLISystem&);
	VDAVIOutputCLISystem& operator=(const VDAVIOutputCLISystem&);
public:
	VDAVIOutputCLISystem(const wchar_t *pszFilename, const wchar_t *setName);
	~VDAVIOutputCLISystem();

	void SetOpt(DubOptions& opt);
	void SetBuffer(int size);
	IVDMediaOutput *CreateSegment() override;
	void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize) override;
	bool AcceptsVideo() override;
	bool AcceptsAudio() override;
	bool IsVideoImageOutputEnabled() override { return true; }
	bool IsVideoImageOutputRequired() override { return true; }
	int GetVideoOutputFormatOverride(int last_format) override;
	bool IsCompressedAudioAllowed() override;
	const char* GetFormatName() override { return nullptr; }

private:
	DubOptions dubOpt;
	VDStringW	mFilename;
	int			mBufferSize;
	VDStringW	mEncSetName;
	bool		mbFinalizeOnAbort;
	bool		mbUseOutputPathAsTemp;

	VDExtEncProfile *mpVidEncProfile;
	VDExtEncProfile *mpAudEncProfile;
	VDExtEncProfile *mpMuxProfile;
};

class VDAVIOutputImagesSystem : public VDDubberOutputSystem {
public:
	VDAVIOutputImagesSystem();
	~VDAVIOutputImagesSystem();

	void SetFilenamePattern(const wchar_t *pszSegmentPrefix, const wchar_t *pszSegmentSuffix, int nMinimumDigits, int startDigit);
	void SetFormat(int format, int quality);
	bool IsVideoImageOutputEnabled() override;
	bool IsVideoImageOutputRequired() override;
	int GetVideoOutputFormatOverride(int last_format) override;

	IVDMediaOutput *CreateSegment() override;
	void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize) override;
	bool AcceptsVideo() override;
	const char* GetFormatName() override { return nullptr; }

private:
	VDStringW	mSegmentPrefix;
	VDStringW	mSegmentSuffix;
	int			mSegmentDigits;
	int			mStartDigit;
	int			mFormat;			// from AVIOutputImages
	int			mQuality;
};

class VDAVIOutputFilmstripSystem : public VDDubberOutputSystem {
public:
	VDAVIOutputFilmstripSystem(const wchar_t *filename);
	~VDAVIOutputFilmstripSystem();

	IVDMediaOutput *CreateSegment() override;
	void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize) override;
	bool AcceptsVideo() override;
	const char* GetFormatName() override { return nullptr; }

private:
	VDStringW	mFilename;
};

class VDAVIOutputGIFSystem : public VDDubberOutputSystem {
public:
	VDAVIOutputGIFSystem(const wchar_t *filename);
	~VDAVIOutputGIFSystem();

	int GetVideoOutputFormatOverride(int last_format) override;

	IVDMediaOutput *CreateSegment() override;
	void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize) override;
	bool AcceptsVideo() override;
	const char* GetFormatName() override { return "gif"; }

	void SetLoopCount(int loopCount) { mLoopCount = loopCount; }

private:
	VDStringW	mFilename;
	int			mLoopCount;
};

class VDAVIOutputAPNGSystem : public VDDubberOutputSystem {
public:
	VDAVIOutputAPNGSystem(const wchar_t *filename);
	~VDAVIOutputAPNGSystem();

	int GetVideoOutputFormatOverride(int last_format) override;

	IVDMediaOutput *CreateSegment() override;
	void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize) override;
	bool AcceptsVideo() override;
	const char* GetFormatName() override { return "apng"; }

	void SetLoopCount(int loopCount) { mLoopCount = loopCount; }
	void SetAlpha(int alpha) { mAlpha = alpha; }
	void SetGrayscale(int grayscale) { mGrayscale = grayscale; }

private:
	VDStringW	mFilename;
	int			mLoopCount;
	int			mAlpha;
	int			mGrayscale;
};

class VDAVIOutputPreviewSystem : public VDDubberOutputSystem {
public:
	VDAVIOutputPreviewSystem();
	~VDAVIOutputPreviewSystem();

	IVDMediaOutput *CreateSegment() override;
	void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize) override;
	bool AcceptsVideo() override;
	bool AcceptsAudio() override;
	bool IsRealTime() override { return true; }
	bool IsVideoImageOutputEnabled() override { return true; }
	bool IsVideoImageOutputRequired() override { return true; }
	bool AreNullFramesAllowed() override { return true; }
	bool IsVideoCompressionEnabled() override { return false; }
	const char* GetFormatName() override { return nullptr; }
};

class VDAVIOutputNullVideoSystem : public VDDubberOutputSystem {
public:
	VDAVIOutputNullVideoSystem();
	~VDAVIOutputNullVideoSystem();

	IVDMediaOutput *CreateSegment() override;
	void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize) override;
	bool AcceptsVideo() override;
	bool AreNullFramesAllowed() override { return true; }
	bool IsVideoCompressionEnabled() override { return true; }
	bool IsNull() override { return true; }
	const char* GetFormatName() override { return nullptr; }
};

class VDAVIOutputSegmentedSystem : public VDDubberOutputSystem {
public:
	VDAVIOutputSegmentedSystem(IVDDubberOutputSystem *pChildSystem, bool intervalIsSeconds, double interval, double preloadInSeconds, sint64 max_bytes, sint64 max_frames);
	~VDAVIOutputSegmentedSystem();

	IVDMediaOutput *CreateSegment() override;
	void CloseSegment(IVDMediaOutput *pSegment, bool bLast, bool finalize) override;
	void SetVideo(const VDXStreamInfo& si, const void *pFormat, int cbFormat) override;
	void SetAudio(const VDXStreamInfo& si, const void *pFormat, int cbFormat, bool bInterleaved, bool vbr) override;
	bool AcceptsVideo() override;
	bool AcceptsAudio() override;
	bool AreNullFramesAllowed() override;
	bool IsVideoCompressionEnabled() override;
	const char* GetFormatName() override { return mpChildSystem->GetFormatName(); }

private:
	IVDDubberOutputSystem *mpChildSystem;
	bool					mbIntervalIsSeconds;
	double					mInterval;
	double					mPreload;
	sint64					mMaxBytes;
	sint64					mMaxFrames;
};

#endif
