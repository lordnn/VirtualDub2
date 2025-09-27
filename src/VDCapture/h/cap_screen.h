#ifndef f_VD2_VDCAPTURE_CAP_SCREEN_H
#define f_VD2_VDCAPTURE_CAP_SCREEN_H

#include <vd2/system/atomic.h>
#include <vd2/VDCapture/capdriver.h>
#include <vd2/VDCapture/ScreenGrabber.h>
#include <vd2/VDCapture/AudioGrabberWASAPI.h>

class VDAudioGrabberWASAPI;

enum VDCaptureDriverScreenMode {
	kVDCaptureDriverScreenMode_GDI,
	kVDCaptureDriverScreenMode_OpenGL,
	kVDCaptureDriverScreenMode_DXGI12,
	kVDCaptureDriverScreenModeCount
};

struct VDCaptureDriverScreenConfig {
	bool	mbTrackCursor;
	bool	mbTrackActiveWindow;
	bool	mbTrackActiveWindowClient;
	bool	mbDrawMousePointer;
	bool	mbRescaleImage;
	bool	mbRemoveDuplicates;
	VDCaptureDriverScreenMode mMode;
	uint32	mRescaleW;
	uint32	mRescaleH;
	sint32	mTrackOffsetX;
	sint32	mTrackOffsetY;

	VDCaptureDriverScreenConfig();

	void Load();
	void Save();
};

class VDCaptureDriverScreen : public IVDCaptureDriver, public IVDScreenGrabberCallback, IVDTimerCallback, IVDAudioGrabberCallbackWASAPI {
	VDCaptureDriverScreen(const VDCaptureDriverScreen&);
	VDCaptureDriverScreen& operator=(const VDCaptureDriverScreen&);
public:
	VDCaptureDriverScreen();
	~VDCaptureDriverScreen() override;

	void	*AsInterface(uint32 id) override { return nullptr; }

	bool	Init(VDGUIHandle hParent) override;
	void	Shutdown();

	void	SetCallback(IVDCaptureDriverCallback *pCB) override;

	void	LockUpdates() override {}
	void	UnlockUpdates() override {}

	bool	IsHardwareDisplayAvailable() override;

	void	SetDisplayMode(nsVDCapture::DisplayMode m) override;
	nsVDCapture::DisplayMode		GetDisplayMode() override;

	void	SetDisplayRect(const vdrect32& r) override;
	vdrect32	GetDisplayRectAbsolute() override;
	void	SetDisplayVisibility(bool vis) override;

	void	SetFramePeriod(sint32 ms) override;
	sint32	GetFramePeriod() override;

	uint32	GetPreviewFrameCount() override;

	bool	GetVideoFormat(vdstructex<BITMAPINFOHEADER>& vformat) override;
	bool	SetVideoFormat(const BITMAPINFOHEADER *pbih, uint32 size) override;

	bool	SetTunerChannel(int channel) override { return false; }
	int		GetTunerChannel() override { return -1; }
	bool	GetTunerChannelRange(int& minChannel, int& maxChannel) override { return false; }
	uint32	GetTunerFrequencyPrecision() override { return 0; }
	uint32	GetTunerExactFrequency() override { return 0; }
	bool	SetTunerExactFrequency(uint32 freq) override { return false; }
	nsVDCapture::TunerInputMode	GetTunerInputMode() override { return nsVDCapture::kTunerInputUnknown; }
	void	SetTunerInputMode(nsVDCapture::TunerInputMode tunerMode) override {}

	int		GetAudioDeviceCount() override;
	const wchar_t *GetAudioDeviceName(int idx) override;
	bool	SetAudioDevice(int idx) override;
	int		GetAudioDeviceIndex() override;
	bool	IsAudioDeviceIntegrated(int idx) override { return false; }

	int		GetVideoSourceCount() override;
	const wchar_t *GetVideoSourceName(int idx) override;
	bool	SetVideoSource(int idx) override;
	int		GetVideoSourceIndex() override;

	int		GetAudioSourceCount() override;
	const wchar_t *GetAudioSourceName(int idx) override;
	bool	SetAudioSource(int idx) override;
	int		GetAudioSourceIndex() override;

	int		GetAudioInputCount() override;
	const wchar_t *GetAudioInputName(int idx) override;
	bool	SetAudioInput(int idx) override;
	int		GetAudioInputIndex() override;

	int		GetAudioSourceForVideoSource(int idx) override { return -2; }

	bool	IsAudioCapturePossible() override;
	bool	IsAudioCaptureEnabled() override;
	bool	IsAudioPlaybackPossible() override { return false; }
	bool	IsAudioPlaybackEnabled() override { return false; }
	void	SetAudioCaptureEnabled(bool b) override;
	void	SetAudioAnalysisEnabled(bool b) override;
	void	SetAudioPlaybackEnabled(bool b) override {}

	void	GetAvailableAudioFormats(std::list<vdstructex<WAVEFORMATEX> >& aformats) override;
	void	GetExtraFormats(std::list<vdstructex<WAVEFORMATEX> >& aformats);

	bool	GetAudioFormat(vdstructex<WAVEFORMATEX>& aformat) override;
	bool	SetAudioFormat(const WAVEFORMATEX *pwfex, uint32 size) override;

	bool	IsDriverDialogSupported(nsVDCapture::DriverDialog dlg) override;
	void	DisplayDriverDialog(nsVDCapture::DriverDialog dlg) override;

	bool	IsPropertySupported(uint32 id) override { return false; }
	sint32	GetPropertyInt(uint32 id, bool *pAutomatic) override { if (pAutomatic) *pAutomatic = true; return 0; }
	void	SetPropertyInt(uint32 id, sint32 value, bool automatic) override {}
	void	GetPropertyInfoInt(uint32 id, sint32& minVal, sint32& maxVal, sint32& step, sint32& defaultVal, bool& automatic, bool& manual) override {}

	bool	CaptureStart() override;
	void	CaptureStop() override;
	void	CaptureAbort() override;

protected:
	void ReceiveFrame(uint64 timestamp, const void *data, ptrdiff_t pitch, uint32 rowlen, uint32 rowcnt) override;

protected:
	void ReceiveAudioDataWASAPI() override;

protected:
	void	SyncCaptureStop();
	void	SyncCaptureAbort();
	void	InitMixerSupport();
	void	ShutdownMixerSupport();
	bool	InitWaveCapture();
	void	ShutdownWaveCapture();
	bool	InitVideoBuffer();
	void	ShutdownVideoBuffer();
	void	UpdateDisplay();
	sint64	ComputeGlobalTime();
	void	DoFrame();
	void	UpdateTracking();
	void	DispatchFrame(const void *data, uint32 size, sint64 timestamp);

	void	TimerCallback() override;

	void	InitPreviewTimer();
	void	ShutdownPreviewTimer();

	void	LoadSettings();
	void	SaveSettings();

	static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	enum { kPreviewTimerID = 100, kResponsivenessTimerID = 101 };

	HWND	mhwndParent;
	HWND	mhwnd;

	IVDScreenGrabber *mpGrabber;
	bool	mbCapBuffersInited;

	VDAtomicInt	mbCaptureFramePending;
	bool	mbCapturing;
	bool	mbCaptureSetup;
	bool	mbVisible;
	bool	mbAudioHardwarePresent;
	bool	mbAudioHardwareEnabled;
	bool	mbAudioCaptureEnabled;
	bool	mbAudioUseWASAPI;
	bool	mbAudioAnalysisEnabled;
	bool	mbAudioAnalysisActive;

	VDCaptureDriverScreenConfig	mConfig;

	uint32	mFramePeriod;

	uint32	mCaptureWidth;
	uint32	mCaptureHeight;
	VDScreenGrabberFormat mCaptureFormat;

	vdstructex<WAVEFORMATEX>		mAudioFormat;

	IVDCaptureDriverCallback	*mpCB;

	nsVDCapture::DisplayMode	mDisplayMode;
	vdrect32	mDisplayArea;

	uint32	mGlobalTimeBase;
	uint64	mVideoTimeBase;

	UINT	mResponsivenessTimer;
	VDAtomicInt	mResponsivenessCounter;

	VDAtomicInt		mbAudioMessagePosted;

	VDAudioGrabberWASAPI *mpAudioGrabberWASAPI;

	UINT	mPreviewFrameTimer;
	VDAtomicInt	mPreviewFrameCount;

	VDCallbackTimer	mCaptureTimer;

	HMIXER	mhMixer;
	int		mMixerInput;
	MIXERCONTROL	mMixerInputControl;
	typedef std::vector<VDStringW>	MixerInputs;
	MixerInputs	mMixerInputs;

	HWAVEIN mhWaveIn;
	WAVEHDR	mWaveBufHdrs[2];
	vdblock<char>	mWaveBuffer;

	vdblock<uint8, vdaligned_alloc<uint8> > mLinearizationBuffer;

	MyError	mCaptureError;

	static ATOM sWndClass;
};

#endif
