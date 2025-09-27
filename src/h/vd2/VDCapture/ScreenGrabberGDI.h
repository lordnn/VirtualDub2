#ifndef f_VD2_VDCAPTURE_SCREENGRABBERGDI_H
#define f_VD2_VDCAPTURE_SCREENGRABBERGDI_H

#include <vd2/system/vdtypes.h>
#include <vd2/system/vdstl.h>
#include <vd2/system/error.h>
#include <vd2/system/profile.h>
#include <vd2/system/vectors.h>
#include <windows.h>
#include <vd2/VDCapture/ScreenGrabber.h>

class VDScreenGrabberGDI : public IVDScreenGrabber {
	VDScreenGrabberGDI(const VDScreenGrabberGDI&);
	VDScreenGrabberGDI& operator=(const VDScreenGrabberGDI&);
public:
	VDScreenGrabberGDI();
	~VDScreenGrabberGDI() override;

	bool	Init(IVDScreenGrabberCallback *cb) override;
	void	Shutdown() override;

	bool	InitCapture(uint32 srcw, uint32 srch, uint32 dstw, uint32 dsth, VDScreenGrabberFormat format) override;
	void	ShutdownCapture() override;

	void	SetCaptureOffset(int x, int y) override;
	void	SetCapturePointer(bool enable) override;

	uint64	GetCurrentTimestamp() override;
	sint64	ConvertTimestampDelta(uint64 t, uint64 base) override;

	bool	AcquireFrame(bool dispatch) override;

	bool	InitDisplay(HWND hwndParent, bool preview) override;
	void	ShutdownDisplay() override;

	void	SetDisplayArea(const vdrect32& r) override;
	void	SetDisplayVisible(bool vis) override;

protected:
	sint64	ComputeGlobalTime();
	void	DispatchFrame(const void *data, uint32 pitch, sint64 timestamp);

	static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND	mhwnd;
	ATOM	mWndClass;
	IVDScreenGrabberCallback *mpCB;

	bool	mbCapBuffersInited;
	HDC		mhdcOffscreen;
	HBITMAP	mhbmOffscreen;
	void	*mpOffscreenData;
	uint32	mOffscreenSize;
	uint32	mOffscreenPitch;

	HCURSOR	mCachedCursor;
	int		mCachedCursorWidth;
	int		mCachedCursorHeight;
	int		mCachedCursorHotspotX;
	int		mCachedCursorHotspotY;
	bool	mbCachedCursorXORMode;

	HDC		mhdcCursorBuffer;
	HBITMAP	mhbmCursorBuffer;
	HGDIOBJ	mhbmCursorBufferOld;
	uint32	*mpCursorBuffer;
	HCURSOR cap_cursor;

	bool	mbVisible;
	bool	mbDisplayPreview;
	bool	mbExcludeSelf;
	vdrect32	mDisplayArea;

	int		mTrackX;
	int		mTrackY;

	bool	mbDrawMousePointer;

	uint32	mCaptureWidth;
	uint32	mCaptureHeight;
};

#endif
