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

#include "stdafx.h"

#include <windows.h>
#include <vd2/system/thread.h>
#include <vd2/system/profile.h>
#include <vd2/system/time.h>
#include <vector>
#include <list>
#include <algorithm>

#include "gui.h"
#include "oshelper.h"
#include "RTProfileDisplay.h"

extern HINSTANCE g_hInst;

const wchar_t g_szRTProfileDisplayControl2Name[]=L"VDRTProfileDisplay2";

bool RegisterRTProfileDisplayControl2();
extern sint32 VDPreferencesGetFilterThreadCount();

/////////////////////////////////////////////////////////////////////////////

struct VDProfileSample {
	uint64	mTimestamp;
	uint64	data;
};

struct VDSampleBlock {
	enum { N = 1024 };
	VDProfileSample mSample[N];
};

struct VDThreadEvent {
	uint64	mTimestamp;
	uint32	mScopeId;
	unsigned	mbOpen : 1;
	unsigned	mAncillaryData : 31;
};

struct VDThreadEventScope {
	const char* name;
	uint32 flags;
	const char* comment;
};

struct VDThreadEventBlock {
	enum { N = 1024 };
	VDThreadEvent mEvents[N];
};

struct VDThreadEventInfo {
	uint32 mEventCount;
	uint32 mEventStartIndex;
	uint32 mUsedFlags;
	vdfastvector<VDThreadEventBlock *> mEventBlocks;
};

class VDEventProfilerW32 : public IVDEventProfiler {
	VDEventProfilerW32(const VDEventProfilerW32&);
	VDEventProfilerW32& operator=(const VDEventProfilerW32&);
public:
	VDEventProfilerW32();
	~VDEventProfilerW32();

	void Attach();
	void Detach();

	void InsertSample(int name, uint64 data);
	void BeginScope(const char *name, uintptr *cache, uint32 data, uint32 flags=0);
	void BeginDynamicScope(const char *name, uintptr *cache, uint32 data, uint32 flags=0);
	void SetComment(uintptr scopeId, const char* data);
	void EndScope();
	void ExitThread();

	void Clear();

	uint32 GetThreadCount() const;
	void UpdateThreadInfo(uint32 threadIndex, VDThreadEventInfo& info) const;

	void UpdateScopes(vdfastvector<VDThreadEventScope>& scopes) const;

	void UpdateSample(int name, vdfastvector<VDProfileSample>& sample) const;

	VDAtomicInt	reset_count;

protected:
	typedef VDThreadEvent Event;
	typedef VDThreadEventBlock EventBlock;

	struct PerThreadInfo {
		VDThreadId	mThreadId;
		vdfastvector<EventBlock *> mEventBlocks;
		EventBlock *mpCurrentBlock;
		VDAtomicInt mCurrentIndex;
		uint32		mStartIndex;
		uint32		mUsedFlags;

		PerThreadInfo()
			: mThreadId(VDGetCurrentThreadID())
			, mpCurrentBlock(NULL)
			, mCurrentIndex(EventBlock::N)
			, mStartIndex(0)
			, mUsedFlags(0)
		{
		}
	};

	struct PerSampleInfo {
		vdfastvector<VDSampleBlock *> mBlocks;
		VDSampleBlock *mpCurrentBlock;
		VDAtomicInt mCurrentIndex;
		uint32		mStartIndex;

		PerSampleInfo()
			: mpCurrentBlock(NULL)
			, mCurrentIndex(VDSampleBlock::N)
			, mStartIndex(0)
		{
		}
	};

	uintptr InitScope(const char *name, uint32 flags, uintptr *cache);
	uintptr InitDynamicScope(const char *name, uint32 flags, uintptr *cache);

	PerThreadInfo *AllocPerThreadInfo();
	bool AllocBlock(PerThreadInfo *pti);
	bool AllocSampleBlock(PerSampleInfo *psi);

	uint32	mTlsIndex;
	std::atomic_int	mEnableCount{};

	mutable VDCriticalSection mMutex;
	vdfastvector<VDThreadEventScope> mScopes;
	vdfastvector<char *> mDynScopeStrings;

	typedef vdfastvector<PerThreadInfo *> Threads;
	Threads mThreads;
	PerSampleInfo mSample[sample_count];
};

VDEventProfilerW32::VDEventProfilerW32()
	: mTlsIndex(::TlsAlloc())
{
	VDThreadEventScope scope;
	scope.name = "";
	scope.flags = 0;
	scope.comment = 0;
	mScopes.push_back(scope);
	reset_count = 0;
}

VDEventProfilerW32::~VDEventProfilerW32() {
	while(!mThreads.empty()) {
		PerThreadInfo *pti = mThreads.back();

		while(!pti->mEventBlocks.empty()) {
			delete pti->mEventBlocks.back();
			pti->mEventBlocks.pop_back();
		}

		delete pti;
		mThreads.pop_back();
	}

	while(!mDynScopeStrings.empty()) {
		delete mDynScopeStrings.back();
		mDynScopeStrings.pop_back();
	}

	{for(int i=0; i<sample_count; i++){
		PerSampleInfo* psi = &mSample[i];

		while(!psi->mBlocks.empty()) {
			delete psi->mBlocks.back();
			psi->mBlocks.pop_back();
		}
	}}

	::TlsFree(mTlsIndex);
}

void VDEventProfilerW32::Attach() {
	mEnableCount.fetch_add(1, std::memory_order_release);
}

void VDEventProfilerW32::Detach() {
	mEnableCount.fetch_sub(1, std::memory_order_release);
}

void VDEventProfilerW32::SetComment(uintptr scopeId, const char* data) {
	if (!mEnableCount.load(std::memory_order_acquire))
		return;

	if (!data || !data[0])
		return;

	vdsynchronized(mMutex) {
		VDThreadEventScope& scope = mScopes[scopeId];

		if (!scope.comment) {
			mDynScopeStrings.push_back();
			char *dynData = _strdup(data);
			mDynScopeStrings.back() = dynData;

			scope.comment = dynData;
		}
	}
}

void VDEventProfilerW32::InsertSample(int name, uint64 data) {
	if (!mEnableCount.load(std::memory_order_acquire))
		return;

	PerSampleInfo* psi = &mSample[name];

	uint32 idx = psi->mCurrentIndex;

	if (idx >= VDSampleBlock::N) {
		if (!AllocSampleBlock(psi))
			return;

		idx = 0;
	}

	VDProfileSample& ev = psi->mpCurrentBlock->mSample[idx++];
	ev.mTimestamp = VDGetPreciseTick();
	ev.data = data;
	psi->mCurrentIndex = idx;
}

void VDEventProfilerW32::BeginScope(const char *name, uintptr *cache, uint32 data, uint32 flags) {
	if (!mEnableCount.load(std::memory_order_acquire))
		return;

	uintptr scopeId = *cache;

	if (!scopeId)
		scopeId = InitScope(name, flags, cache);

	PerThreadInfo *pti = (PerThreadInfo *)::TlsGetValue(mTlsIndex);
	if (!pti)
		pti = AllocPerThreadInfo();

	uint32 idx = pti->mCurrentIndex;

	if (idx >= EventBlock::N) {
		if (!AllocBlock(pti))
			return;

		idx = 0;
	}

	Event& ev = pti->mpCurrentBlock->mEvents[idx++];
	ev.mTimestamp = VDGetPreciseTick();
	ev.mScopeId = scopeId;
	if (flags & (vdprofiler_flag_loop|vdprofiler_flag_event)) ev.mScopeId |= 0x80000000;
	ev.mbOpen = true;
	ev.mAncillaryData = data;

	pti->mCurrentIndex = idx;
	pti->mUsedFlags |= flags;
}

void VDEventProfilerW32::BeginDynamicScope(const char *name, uintptr *cache, uint32 data, uint32 flags) {
	if (!mEnableCount.load(std::memory_order_acquire))
		return;

	uintptr scopeId = *cache;

	if (!scopeId)
		scopeId = InitDynamicScope(name, flags, cache);

	PerThreadInfo *pti = (PerThreadInfo *)::TlsGetValue(mTlsIndex);
	if (!pti)
		pti = AllocPerThreadInfo();

	uint32 idx = pti->mCurrentIndex;

	if (idx >= EventBlock::N) {
		if (!AllocBlock(pti))
			return;

		idx = 0;
	}

	Event& ev = pti->mpCurrentBlock->mEvents[idx++];
	ev.mTimestamp = VDGetPreciseTick();
	ev.mScopeId = scopeId;
	ev.mbOpen = true;
	ev.mAncillaryData = data;

	pti->mCurrentIndex = idx;
}

void VDEventProfilerW32::EndScope() {
	PerThreadInfo *pti = (PerThreadInfo *)::TlsGetValue(mTlsIndex);

	if (!pti)
		return;

	if (!mEnableCount.load(std::memory_order_acquire))
		return;

	uint32 idx = pti->mCurrentIndex;

	if (idx >= EventBlock::N) {
		if (!AllocBlock(pti))
			return;

		idx = 0;
	}

	Event& ev = pti->mpCurrentBlock->mEvents[idx++];
	ev.mTimestamp = VDGetPreciseTick();
	ev.mScopeId = 0;
	ev.mbOpen = false;

	pti->mCurrentIndex = idx;
}

void VDEventProfilerW32::ExitThread() {
}

void VDEventProfilerW32::Clear() {
	mMutex.Lock();
	for(Threads::iterator it(mThreads.begin()), itEnd(mThreads.end()); it != itEnd; ++it) {
		PerThreadInfo *pti = *it;

		while(!pti->mEventBlocks.empty()) {
			EventBlock *b = pti->mEventBlocks.back();

			if (b && b != pti->mpCurrentBlock)
				delete b;

			pti->mEventBlocks.pop_back();
		}

		pti->mEventBlocks.push_back(pti->mpCurrentBlock);
		pti->mStartIndex = pti->mCurrentIndex;
	}

	for(int i=0; i<mScopes.size(); i++) {
		mScopes[i].comment = 0;
	}

	{for(int i=0; i<sample_count; i++){
		PerSampleInfo* psi = &mSample[i];

		while(!psi->mBlocks.empty()) {
			VDSampleBlock *b = psi->mBlocks.back();

			if (b && b != psi->mpCurrentBlock)
				delete b;

			psi->mBlocks.pop_back();
		}

		psi->mBlocks.push_back(psi->mpCurrentBlock);
		psi->mStartIndex = psi->mCurrentIndex;
	}}

	mMutex.Unlock();
}

uint32 VDEventProfilerW32::GetThreadCount() const {
	mMutex.Lock();
	uint32 n = mThreads.size();
	mMutex.Unlock();

	return n;
}

void VDEventProfilerW32::UpdateThreadInfo(uint32 threadIndex, VDThreadEventInfo& info) const {
	info.mEventBlocks.clear();
	info.mEventCount = 0;
	info.mEventStartIndex = 0;
	info.mUsedFlags = 0;

	mMutex.Lock();
	uint32 n = mThreads.size();
	if (threadIndex < n) {
		PerThreadInfo& pti = *mThreads[threadIndex];

		size_t n1 = info.mEventBlocks.size();
		size_t n2 = pti.mEventBlocks.size();

		if (n1 < n2)
			info.mEventBlocks.insert(info.mEventBlocks.end(), pti.mEventBlocks.begin() + n1, pti.mEventBlocks.end());

		info.mEventCount = VDThreadEventBlock::N * pti.mEventBlocks.size() - VDThreadEventBlock::N + pti.mCurrentIndex - pti.mStartIndex;
		info.mEventStartIndex = pti.mStartIndex;
		info.mUsedFlags = pti.mUsedFlags;
	}
	mMutex.Unlock();

}

void VDEventProfilerW32::UpdateScopes(vdfastvector<VDThreadEventScope>& scopes) const {
	mMutex.Lock();
	size_t n1 = scopes.size();
	size_t n2 = mScopes.size();

	if (n1 < n2)
		scopes.insert(scopes.end(), mScopes.begin() + n1, mScopes.end());

	for(int i=0; i<mScopes.size(); i++) {
		scopes[i].comment = mScopes[i].comment;
	}

	mMutex.Unlock();
}

void VDEventProfilerW32::UpdateSample(int name, vdfastvector<VDProfileSample>& sample) const {
	mMutex.Lock();

	const PerSampleInfo& psi = mSample[name];
	int n = VDSampleBlock::N * psi.mBlocks.size() - VDSampleBlock::N + psi.mCurrentIndex - psi.mStartIndex;
	int n0 = sample.size();
	sample.resize(n);

	{for(int i=n0; i<n; i++){
		int idx = i+psi.mStartIndex;
		const VDProfileSample& ev = psi.mBlocks[idx / VDSampleBlock::N]->mSample[idx % VDSampleBlock::N];
		sample[i].mTimestamp = ev.mTimestamp;
		sample[i].data = ev.data;
	}}

	mMutex.Unlock();
}

uintptr VDEventProfilerW32::InitScope(const char *name, uint32 flags, uintptr *cache) {
	uintptr h;

	vdsynchronized(mMutex) {
		h = *(volatile uintptr *)cache;
		if (!h) {
			h = mScopes.size();
			VDThreadEventScope scope;
			scope.name = name;
			scope.flags = flags;
			scope.comment = 0;
			mScopes.push_back(scope);
			*cache = h;
		}
	}

	return h;
}

uintptr VDEventProfilerW32::InitDynamicScope(const char *name, uint32 flags, uintptr *cache) {
	uintptr h;

	vdsynchronized(mMutex) {
		h = *(volatile uintptr *)cache;
		if (!h) {
			h = mScopes.size();

			mDynScopeStrings.push_back();
			char *dynName = _strdup(name);
			mDynScopeStrings.back() = dynName;

			VDThreadEventScope scope;
			scope.name = dynName;
			scope.flags = flags;
			scope.comment = 0;
			mScopes.push_back(scope);
			*cache = h;
		}
	}

	return h;
}

VDEventProfilerW32::PerThreadInfo *VDEventProfilerW32::AllocPerThreadInfo() {
	PerThreadInfo *pti = new PerThreadInfo;

	::TlsSetValue(mTlsIndex, pti);

	mMutex.Lock();
	mThreads.push_back(pti);
	mMutex.Unlock();

	return pti;
}

bool VDEventProfilerW32::AllocBlock(PerThreadInfo *pti) {
	mMutex.Lock();
	bool enabled = mEnableCount.load(std::memory_order_acquire) > 0;
	if (enabled) {
		EventBlock *block = new EventBlock;
		pti->mEventBlocks.push_back(block);
		pti->mpCurrentBlock = block;
		pti->mCurrentIndex = 0;
	} else {
		while(!pti->mEventBlocks.empty()) {
			delete pti->mEventBlocks.back();
			pti->mEventBlocks.pop_back();
		}

		::TlsSetValue(mTlsIndex, NULL);

		Threads::iterator it(std::find(mThreads.begin(), mThreads.end(), pti));

		if (it != mThreads.end()) {
			*it = mThreads.back();
			mThreads.pop_back();
		} else {
			VDASSERT(!"Invalid thread profiling context.");
		}

		delete pti;
	}
	mMutex.Unlock();

	return enabled;
}

bool VDEventProfilerW32::AllocSampleBlock(PerSampleInfo *psi) {
	mMutex.Lock();
	bool enabled = mEnableCount.load(std::memory_order_acquire) > 0;
	if (enabled) {
		VDSampleBlock *block = new VDSampleBlock;
		psi->mBlocks.push_back(block);
		psi->mpCurrentBlock = block;
		psi->mCurrentIndex = 0;
	} else {
		while(!psi->mBlocks.empty()) {
			delete psi->mBlocks.back();
			psi->mBlocks.pop_back();
		}
	}
	mMutex.Unlock();

	return enabled;
}

/////////////////////////////////////////////////////////////////////////////

void VDInitEventProfiler() {
	if (!g_pVDEventProfiler)
		g_pVDEventProfiler = new VDEventProfilerW32;
}

void VDShutdownEventProfiler() {
	if (g_pVDEventProfiler) {
		delete static_cast<VDEventProfilerW32 *>(g_pVDEventProfiler);
		g_pVDEventProfiler = NULL;
	}
}

void VDRestartEventProfiler() {
	VDEventProfilerW32 *p = static_cast<VDEventProfilerW32 *>(g_pVDEventProfiler);
	if (p) {
		p->Clear();
		p->reset_count++;
	}
}

/////////////////////////////////////////////////////////////////////////////

struct VDProfileTrackedEvent {
	uint64	mStartTime;
	uint64	mEndTime;
	uint32	mScopeId;
	uint32	mChildScopes;
	uint32	mAncillaryData;
};

class VDEventProfileThreadTracker {
public:
	VDEventProfileThreadTracker(uint32 index);

	vdspan<VDProfileTrackedEvent> GetEvents() const;
	uint32 GetUsedFlags() const { return mThreadInfo.mUsedFlags; }

	void Update();

protected:
	const uint32 mThreadIndex;
	uint32 mLastEventCount;

	VDThreadEventInfo mThreadInfo;

	vdfastvector<VDProfileTrackedEvent> mEvents;
	vdfastvector<size_t> mEventStack;
};

VDEventProfileThreadTracker::VDEventProfileThreadTracker(uint32 index)
	: mThreadIndex(index)
	, mLastEventCount(0)
{
}

vdspan<VDProfileTrackedEvent> VDEventProfileThreadTracker::GetEvents() const {
	return mEvents;
}

void VDEventProfileThreadTracker::Update() {
	VDEventProfilerW32 *p = static_cast<VDEventProfilerW32 *>(g_pVDEventProfiler);
	if (!p)
		return;

	p->UpdateThreadInfo(mThreadIndex, mThreadInfo);

	while(mLastEventCount < mThreadInfo.mEventCount) {
		int idx = mLastEventCount + mThreadInfo.mEventStartIndex;
		const VDThreadEvent& ev = mThreadInfo.mEventBlocks[idx / VDThreadEventBlock::N]->mEvents[idx % VDThreadEventBlock::N];

		if (ev.mbOpen) {
			VDProfileTrackedEvent& tev = mEvents.push_back();
			tev.mStartTime = ev.mTimestamp;
			tev.mEndTime = 0;
			tev.mScopeId = ev.mScopeId;
			tev.mChildScopes = 0xFFFFFFFF;
			tev.mAncillaryData = ev.mAncillaryData;
			bool is_event = (ev.mScopeId & 0x80000000)!=0;
			tev.mScopeId &= ~0x80000000;
			if (is_event) {
				tev.mEndTime = tev.mStartTime;
				tev.mChildScopes = 0;
			} else {
				mEventStack.push_back(mEvents.size() - 1);
			}
		} else if (!mEventStack.empty()) {
			size_t idx = mEventStack.back();
			mEventStack.pop_back();

			VDProfileTrackedEvent& tev = mEvents[idx];
			tev.mEndTime = ev.mTimestamp;
			tev.mChildScopes = mEvents.size() - (idx + 1);
		}

		++mLastEventCount;
	}
}

/////////////////////////////////////////////////////////////////////////////

class VDRTProfileDisplay2 {
public:
	VDRTProfileDisplay2(HWND hwnd);
	~VDRTProfileDisplay2();

	static VDRTProfileDisplay2 *Create(HWND hwndParent, int x, int y, int cx, int cy, UINT id);

	static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK StaticListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);
	void OnSize(int w, int h);
	void OnPaint();
	void OnSetFont(HFONT hfont, bool bRedraw);
	bool OnKey(INT wParam);
	void OnTimer();
	void UpdateList();
	void UpdateSummary();
	void UpdateListTop(bool sync);

protected:
	void Clear();
	void DeleteThreadProfiles();
	void UpdateThreadProfiles();
	void UpdateScale();
	void UpdateRange();
	void SetDisplayResolution(int res);
	void SetDisplayAutoZoom();
	void ScrollByPixels(int dx, int dy);
	void InvalidateEventRect(const VDProfileTrackedEvent& ev);
	void GoToEvent(const VDProfileTrackedEvent& ev);
	void GoToEvent(int px, int py);

	typedef VDRTProfiler::Event	Event;
	typedef VDRTProfiler::Channel	Channel;

	const HWND mhwnd;
	HWND    mhwndList;
	void*   old_list_proc;
	HWND    mhwndSummary;
	HFONT		mhfont;
	HPEN    pen1;
	HPEN    pen2;
	HPEN    pen5;
	HBRUSH  br1;
	HBRUSH  br2;
	HBRUSH  br3;
	HBRUSH  br4;
	HBRUSH  br5;
	HCURSOR cr_hand;
	VDRTProfiler	*mpProfiler;
	int     mode;
	int     reset_count;
	int     event_count;
	int     display_event_count;
	int     list_event_count;
	int     list_top;

	int			mDisplayResolution;
	int			mWidth;
	int			mHeight;

	int			mFontHeight;
	int			mFontAscent;
	int			mFontInternalLeading;
	int			mScrollHeight;

	int			mWheelDeltaAccum;
	int			mDragOffsetX;
	int			mDragOffsetY;

	uint64 baseTime;
	uint64 range_start;
	uint64 range_end;
	bool use_range_start;
	bool use_range_end;

	sint64		mBaseTimeOffset;
	sint64		mBasePixelOffset;
	int			mMajorScaleMinWidth;
	int			mMajorScaleInterval;
	int			mMajorScaleDecimalPlaces;

	VDStringA	mTempStr;

	vdfastvector<VDEventProfileThreadTracker *> mThreadProfiles;
	vdfastvector<VDThreadEventScope> mScopes;
	vdfastvector<const VDProfileTrackedEvent *>	mSortedList;
	vdfastvector<VDProfileSample>	sample_cpu;
	const VDProfileTrackedEvent* selection;

	friend bool VDRegisterRTProfileDisplayControl2();
	static ATOM sWndClass;
};

ATOM VDRTProfileDisplay2::sWndClass;

/////////////////////////////////////////////////////////////////////////////

VDRTProfileDisplay2::VDRTProfileDisplay2(HWND hwnd)
	: mhwnd(hwnd)
	, mhfont(0)
	, mpProfiler(0)
	, mDisplayResolution(0)
	, mWidth(1)
	, mHeight(1)
	, mWheelDeltaAccum(0)
	, mBaseTimeOffset(0)
	, mBasePixelOffset(0)
	, mMajorScaleMinWidth(1)
	, mMajorScaleInterval(1)
{
	SetDisplayResolution(-10);
	mode = 1;
	reset_count = 0;
	event_count = 0;
	display_event_count = 0;
	list_event_count = 0;
	list_top = 0;
	mhwndList = 0;
	mhwndSummary = 0;
	selection = 0;
	pen1 = 0;
	pen2 = 0;
	pen5 = 0;
	br1 = 0;
	br2 = 0;
	br3 = 0;
	br4 = 0;
	br5 = 0;

	pen1 = CreatePen(PS_SOLID,1,RGB(0,0,0));
	pen2 = CreatePen(PS_SOLID,1,RGB(0,100,255));
	pen5 = CreatePen(PS_SOLID,1,RGB(0,150,0));
	br1 = CreateSolidBrush(RGB(240,240,240));
	br2 = CreateSolidBrush(RGB(0,100,255));
	br3 = CreateSolidBrush(RGB(255,200,200));
	br4 = CreateSolidBrush(RGB(255,255,0));
	br5 = CreateSolidBrush(RGB(90,255,90));
	cr_hand = LoadCursor(0,IDC_HAND);
}

VDRTProfileDisplay2::~VDRTProfileDisplay2() {
	DeleteThreadProfiles();
	if(mhfont)
		DeleteObject(mhfont);
	if(pen1)
		DeleteObject(pen1);
	if(pen2)
		DeleteObject(pen2);
	if(pen5)
		DeleteObject(pen5);
	if(br1)
		DeleteObject(br1);
	if(br2)
		DeleteObject(br2);
	if(br3)
		DeleteObject(br3);
	if(br4)
		DeleteObject(br4);
	if(br5)
		DeleteObject(br5);
}

VDRTProfileDisplay2 *VDRTProfileDisplay2::Create(HWND hwndParent, int x, int y, int cx, int cy, UINT id) {
	HWND hwnd = CreateWindowW(MAKEINTATOM(sWndClass), L"", WS_VISIBLE|WS_CHILD, x, y, cx, cy, hwndParent, (HMENU)(UINT_PTR)id, g_hInst, NULL);

	if (hwnd)
		return (VDRTProfileDisplay2 *)GetWindowLongPtr(hwnd, 0);

	return NULL;
}

bool VDRegisterRTProfileDisplayControl2() {
	WNDCLASS wc;

	wc.style		= 0;
	wc.lpfnWndProc	= VDRTProfileDisplay2::StaticWndProc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= sizeof(VDRTProfileDisplay2 *);
	wc.hInstance	= g_hInst;
	wc.hIcon		= NULL;
	wc.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground= (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName	= NULL;
	wc.lpszClassName= g_szRTProfileDisplayControl2Name;

	VDRTProfileDisplay2::sWndClass = RegisterClass(&wc);
	return VDRTProfileDisplay2::sWndClass != 0;
}

/////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK VDRTProfileDisplay2::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	VDRTProfileDisplay2 *pThis = (VDRTProfileDisplay2 *)GetWindowLongPtr(hwnd, 0);

	switch(msg) {
	case WM_NCCREATE:
		if (!(pThis = new_nothrow VDRTProfileDisplay2(hwnd)))
			return FALSE;

		SetWindowLongPtr(hwnd, 0, (LONG_PTR)pThis);
		break;

	case WM_NCDESTROY:
		delete pThis;
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	if (pThis)
		return pThis->WndProc(msg, wParam, lParam);
	else
		return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK VDRTProfileDisplay2::StaticListWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	VDRTProfileDisplay2* pthis = (VDRTProfileDisplay2*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
	if (msg==WM_DESTROY) {
		SetWindowLongPtr(hwnd,GWLP_WNDPROC,(LPARAM)pthis->old_list_proc);
	}
	if (msg==WM_HSCROLL) {
		pthis->UpdateListTop(true);
	}

	return CallWindowProc((WNDPROC)pthis->old_list_proc, hwnd, msg, wParam, lParam);
}

LRESULT VDRTProfileDisplay2::WndProc(UINT msg, WPARAM wParam, LPARAM lParam) {
	VDEventProfilerW32 *p = static_cast<VDEventProfilerW32 *>(g_pVDEventProfiler);
	if (p==0 || p->reset_count>reset_count) selection = 0;

	switch(msg) {
		case WM_CREATE:
			OnSetFont(NULL, FALSE);
			UpdateScale();
			SetTimer(mhwnd, 1, 1000, NULL);
			VDSetDialogDefaultIcons(mhwnd);
			VDInitEventProfiler();
			{
				VDEventProfilerW32 *p = static_cast<VDEventProfilerW32 *>(g_pVDEventProfiler);

				p->Attach();
			}
			return 0;

		case WM_USER+600: // set list
			mhwndList = (HWND)lParam;
			old_list_proc = (void*)SetWindowLongPtr(mhwndList,GWLP_WNDPROC,(LPARAM)StaticListWndProc);
			SetWindowLongPtr(mhwndList,GWLP_USERDATA,(LPARAM)this);
			return 0;

		case WM_USER+601: // list selection change
			{
				int x = SendMessage(mhwndList, LB_GETCURSEL, 0, 0);
				if(selection)
					InvalidateEventRect(*selection);
				if(x<mSortedList.size())
					selection = mSortedList[x];
				else
					selection = 0;
				if(selection)
					InvalidateEventRect(*selection);
				UpdateListTop(true);
				return 0;
			}
		case WM_USER+602: // list double click
			{
				if(selection){
					SetFocus(mhwnd);
					GoToEvent(*selection);
				}
				return 0;
			}

		case WM_USER+603: // set summary list
			mhwndSummary = (HWND)lParam;
			return 0;

		case WM_USER+604: // set mode
			if (lParam==3)
				Clear();
			else
				mode = lParam;
			return 0;

		case WM_DESTROY:
			{
				VDEventProfilerW32 *p = static_cast<VDEventProfilerW32 *>(g_pVDEventProfiler);

				p->Detach();
				p->Clear();
			}
			return 0;

		case WM_SIZE:
			OnSize(LOWORD(lParam), HIWORD(lParam));
			return 0;

		case WM_GETMINMAXINFO:
			{
				MINMAXINFO& mmi = *(MINMAXINFO *)lParam;

				int tc = 0;
				for(size_t i=0; i<mThreadProfiles.size(); ++i) {
					VDEventProfileThreadTracker *tracker = mThreadProfiles[i];
					if(!tracker) continue;
					if(!tracker->GetEvents().size()) continue;
					tc++;
				}
				int tc2 = VDPreferencesGetFilterThreadCount()+2;
				if(tc2>tc) tc = tc2;
				int maxH = tc*20+55+mFontHeight;
				mmi.ptMaxTrackSize.y = maxH;
			}
			return 0;

		case WM_PAINT:
			OnPaint();
			return 0;

		case WM_ERASEBKGND:
			return 0;

		case WM_SETFONT:
			OnSetFont((HFONT)wParam, lParam != 0);
			return 0;

		case WM_GETFONT:
			return (LRESULT)mhfont;

		case WM_KEYDOWN:
			if (OnKey(wParam))
				return 0;
			break;

		case WM_TIMER:
			OnTimer();
			return 0;

		case WM_GETDLGCODE:
			{
				MSG* msg = (MSG*)lParam;
				if (!msg) return 0;
				if (msg->message==WM_CHAR) return DLGC_WANTMESSAGE;
				if (msg->message==WM_KEYDOWN && (wParam==VK_LEFT || wParam==VK_RIGHT || wParam==VK_MULTIPLY || wParam==VK_ADD || wParam==VK_SUBTRACT || wParam=='X'))
					return DLGC_WANTMESSAGE;
			}
			return 0;

		case WM_MOUSEWHEEL:
			{
				int dz = GET_WHEEL_DELTA_WPARAM(wParam);

				mWheelDeltaAccum += dz;

				int dr = mWheelDeltaAccum / WHEEL_DELTA;

				if (dr) {
					mWheelDeltaAccum %= WHEEL_DELTA;
					SetDisplayResolution(mDisplayResolution + dr);
				}
			}
			break;

		case WM_LBUTTONDOWN:
			if (::GetKeyState(VK_MENU) >= 0) {
				POINT pt;
				GetCursorPos(&pt);
				RECT r;
				GetClientRect(mhwnd,&r);
				MapWindowPoints(0,mhwnd,&pt,1);
				if (pt.y<r.bottom-mScrollHeight) {
					::SetFocus(mhwnd);
					GoToEvent(LOWORD(lParam),HIWORD(lParam));
					UpdateListTop(false);
					break;
				}
			}
			// fall through
		case WM_MBUTTONDOWN:
			mDragOffsetX = (short)LOWORD(lParam);
			mDragOffsetY = (short)HIWORD(lParam);
			::SetCapture(mhwnd);
			::SetCursor(::LoadCursor(NULL, IDC_SIZEALL));
			::SetFocus(mhwnd);
			break;

		case WM_RBUTTONDOWN:
			::SetFocus(mhwnd);
			break;

		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
			if (::GetCapture() == mhwnd)
				::ReleaseCapture();
			break;

		case WM_MOUSEMOVE:
			if (::GetCapture() == mhwnd) {
				int x = (short)LOWORD(lParam);
				int y = (short)HIWORD(lParam);
				int dx = x - mDragOffsetX;
				int dy = y - mDragOffsetY;

				mDragOffsetX = x;
				mDragOffsetY = y;

				ScrollByPixels(-dx, -dy);
			}
			break;

		case WM_SETCURSOR:
			if (::GetCapture() == mhwnd) {
				::SetCursor(::LoadCursor(NULL, IDC_SIZEALL));
				return TRUE;
			} else {
				POINT pt;
				GetCursorPos(&pt);
				RECT r;
				GetClientRect(mhwnd,&r);
				MapWindowPoints(0,mhwnd,&pt,1);
				if (pt.y>r.bottom-mScrollHeight) {
					SetCursor(cr_hand);
					return TRUE;
				}
			}
			break;

		case WM_SYSKEYDOWN:
			if (wParam == VK_MENU)
				return 0;
			break;

		case WM_SYSKEYUP:
			if (wParam == VK_MENU)
				return 0;
			break;
	}
	return DefWindowProc(mhwnd, msg, wParam, lParam);
}

void VDRTProfileDisplay2::OnSize(int w, int h) {
	mWidth = w;

	if (mHeight != h) {
		mHeight = h;
		::InvalidateRect(mhwnd, NULL, TRUE);
	}
}

void VDRTProfileDisplay2::UpdateRange() {
	baseTime = 0;
	range_start = 0;
	range_end = 0;
	bool baseTimeSet = false;
	use_range_start = false;
	use_range_end = false;

	size_t threadCount = mThreadProfiles.size();
	for(size_t i=0; i<threadCount; ++i) {
		VDEventProfileThreadTracker *tracker = mThreadProfiles[i];

		if (!tracker)
			continue;

		const vdspan<VDProfileTrackedEvent>& events = tracker->GetEvents();

		if (!events.empty()) {
			const VDProfileTrackedEvent& ev = events.front();
			uint64 t = ev.mStartTime;

			if (!baseTimeSet || (sint64)(t - baseTime) < 0) {
				baseTimeSet = true;
				baseTime = t;
			}

			for(uint32 j=0; j<events.size(); ++j) {
				const VDProfileTrackedEvent& ev = events[j];
				uint64 t = ev.mStartTime;

				uint32 flags = mScopes[ev.mScopeId].flags;
				if(flags & vdprofiler_flag_mode) {
					int m = ev.mAncillaryData;
					if (m==1) {
						use_range_start = true;
						range_start = t;
					}
					if (m==0 && use_range_start && t>=range_start) {
						use_range_end = true;
						range_end = t;
					}
				}
			}
		}
	}

	if (use_range_start) baseTime = range_start;
}

void VDRTProfileDisplay2::GoToEvent(int px, int py) {
	double pixelsPerMicrosec = pow(2.0, (double)mDisplayResolution);
	double pixelsPerTick = VDGetPreciseSecondsPerTick() * 1000000.0 * pixelsPerMicrosec;

	int next = -1;
	for(size_t i=0; i<mSortedList.size(); ++i) {
		const VDProfileTrackedEvent& ev = *mSortedList[i];
		double xf1 = (double)(sint64)(ev.mStartTime - baseTime) * pixelsPerTick;
		sint64 x1 = VDCeilToInt64(xf1 - 0.5) - mBasePixelOffset;
		if (x1<=px) next = i; else break;
	}

	if (next!=-1) {
		if(selection)
			InvalidateEventRect(*selection);
		selection = mSortedList[next];
		InvalidateEventRect(*selection);
		SendMessage(mhwndList,LB_SETCURSEL,next,0);
	}
}

void VDRTProfileDisplay2::GoToEvent(const VDProfileTrackedEvent& ev) {
	RECT r0;
	GetClientRect(mhwnd,&r0);

	double pixelsPerMicrosec = pow(2.0, (double)mDisplayResolution);
	double pixelsPerTick = VDGetPreciseSecondsPerTick() * 1000000.0 * pixelsPerMicrosec;

	double xf1 = (double)(sint64)(ev.mStartTime - baseTime) * pixelsPerTick;
	sint64 offset = VDCeilToInt64(xf1 - 0.5) - r0.right/2;
	ScrollByPixels(int(offset-mBasePixelOffset),0);
}

void VDRTProfileDisplay2::InvalidateEventRect(const VDProfileTrackedEvent& ev) {
	RECT r0;
	GetClientRect(mhwnd,&r0);

	double pixelsPerMicrosec = pow(2.0, (double)mDisplayResolution);
	double pixelsPerTick = VDGetPreciseSecondsPerTick() * 1000000.0 * pixelsPerMicrosec;

	double xf1 = (double)(sint64)(ev.mStartTime - baseTime) * pixelsPerTick;
	double xf2 = (double)(sint64)(ev.mEndTime - baseTime) * pixelsPerTick;

	sint64 x1 = VDCeilToInt64(xf1 - 0.5) - mBasePixelOffset;
	sint64 x2 = VDCeilToInt64(xf2 - 0.5) - mBasePixelOffset;

	if (x1 >= r0.right)
		return;

	if (x2 <= 0)
		return;

	int ix1 = VDClampToSint32(x1);
	int ix2 = VDClampToSint32(x2);

	r0.left = ix1-3;
	r0.right = ix2+3;
	InvalidateRect(mhwnd,&r0,false);
}

void VDRTProfileDisplay2::OnPaint() {
	PAINTSTRUCT ps;
	if (HDC hdc = BeginPaint(mhwnd, &ps)) {
		HBRUSH hbrBackground = (HBRUSH)GetClassLongPtr(mhwnd, GCLP_HBRBACKGROUND);
		HGDIOBJ hOldFont = 0;
		if (mhfont)
			hOldFont = SelectObject(hdc, mhfont);

		RECT rClient;
		GetClientRect(mhwnd, &rClient);

		FillRect(hdc, &rClient, hbrBackground);

		double pixelsPerMicrosec = pow(2.0, (double)mDisplayResolution);
		double microsecsPerPixel = pow(2.0, -(double)mDisplayResolution);
		double pixelsPerTick = VDGetPreciseSecondsPerTick() * 1000000.0 * pixelsPerMicrosec;

		size_t threadCount = mThreadProfiles.size();
		size_t actualCount = 0;
		for(size_t i=0; i<threadCount; ++i) {
			VDEventProfileThreadTracker *tracker = mThreadProfiles[i];

			if (!tracker)
				continue;

			const vdspan<VDProfileTrackedEvent>& events = tracker->GetEvents();
			size_t n = events.size();
			if(!n) continue;

			actualCount++;
		}

		sint64 timeStartOffset = VDRoundToInt64((ps.rcPaint.left + mBasePixelOffset) * microsecsPerPixel);
		sint64 majorTick = timeStartOffset;

		--majorTick;
		majorTick -= majorTick % mMajorScaleInterval;

		HPEN pen0 = (HPEN)SelectObject(hdc,pen1);
		HBRUSH br0 = (HBRUSH)SelectObject(hdc,br5);

		// cpu usage
		{
			int y1 = actualCount*20;
			int y2 = y1 + 50;
			int ix0 = 0;
			int y0 = 0;
			uint64 sw = 0;
			uint64 sm = 0;

			const vdspan<VDProfileSample>& events = sample_cpu;
			size_t n = events.size();
			MoveToEx(hdc,ps.rcPaint.left, y2-41, 0);
			LineTo(hdc,ps.rcPaint.right,y2-41);
			MoveToEx(hdc,ps.rcPaint.left, y2-1, 0);
			LineTo(hdc,ps.rcPaint.right,y2-1);
			SelectObject(hdc,pen5);

			{for(uint32 j=0; j<n; ++j) {
				const VDProfileSample& ev = events[j];

				double xf1 = (double)(sint64)(ev.mTimestamp - baseTime) * pixelsPerTick;

				sint64 x1 = VDCeilToInt64(xf1 - 0.5) - mBasePixelOffset;

				if (ix0-3 >= ps.rcPaint.right)
					break;

				int ix1 = VDClampToSint32(x1);
				if ((ix1<ix0+3) && j>0) {
					uint64 d1 = ev.mTimestamp-events[j-1].mTimestamp;
					sw += d1;
					sm += d1*ev.data;
					continue;
				}

				uint64 data = ev.data;
				if (sw>0) data = sm/sw;
				int y = y2 - int(40*data/100);

				if (x1+3 > ps.rcPaint.left) {
					if (microsecsPerPixel<5000) {
						Rectangle(hdc, ix0-1, y, ix1, y2);
					} else {
						POINT p[2] = {ix0,y0,ix1,y};
						Polyline(hdc,p,2);
					}
				}

				ix0 = ix1;
				y0 = y;
				sw = 0;
				sm = 0;
			}}
		}

		::SetTextAlign(hdc, TA_LEFT | TA_BOTTOM);
		::SetTextColor(hdc, 0);
		SetBkMode(hdc,TRANSPARENT);
		SelectObject(hdc,pen1);
		SelectObject(hdc,br1);
		for(;;) {
			int x = VDFloorToInt(majorTick * pixelsPerMicrosec) - (int)mBasePixelOffset;

			if (x >= ps.rcPaint.right)
				break;

			RECT r = ps.rcPaint;
			r.left = x;
			r.right = x+1;
			FillRect(hdc, &r, (HBRUSH)(COLOR_GRAYTEXT + 1));

			char buf[64];
			sprintf(buf, "%.*fs", mMajorScaleDecimalPlaces, (double)majorTick / 1000000.0);

			::TextOutA(hdc, x + 2, mHeight - 2, buf, strlen(buf));
			majorTick += mMajorScaleInterval;
		}

		if(selection){
			double xf1 = (double)(sint64)(selection->mStartTime - baseTime) * pixelsPerTick;
			sint64 x1 = VDCeilToInt64(xf1 - 0.5) - mBasePixelOffset;

			if ((x1-3 < ps.rcPaint.right) && (x1+3 > ps.rcPaint.left)){
				int x = VDClampToSint32(x1);
				RECT r = ps.rcPaint;
				r.left = x-1;
				r.right = x+2;
				FillRect(hdc, &r, br2);
			}
		}

		::SetTextAlign(hdc, TA_LEFT | TA_TOP);
		int y2 = 0;
		for(size_t i=0; i<threadCount; ++i) {
			VDEventProfileThreadTracker *tracker = mThreadProfiles[i];

			if (!tracker)
				continue;

			const vdspan<VDProfileTrackedEvent>& events = tracker->GetEvents();
			size_t n = events.size();
			if(!n) continue;

			int y1 = y2;
			y2 = y1 + 20;

			{for(uint32 j=0; j<n; ++j) {
				const VDProfileTrackedEvent& ev = events[j];
				if (ev.mChildScopes == 0xFFFFFFFF)
					continue;
				if (use_range_start && ev.mStartTime<range_start) continue;
				if (use_range_end && ev.mStartTime>range_end) break;

				double xf1 = (double)(sint64)(ev.mStartTime - baseTime) * pixelsPerTick;
				double xf2 = (double)(sint64)(ev.mEndTime - baseTime) * pixelsPerTick;

				sint64 x1 = VDCeilToInt64(xf1 - 0.5) - mBasePixelOffset;
				sint64 x2 = VDCeilToInt64(xf2 - 0.5) - mBasePixelOffset;

				if (x1 >= ps.rcPaint.right)
					break;

				if (x2 <= ps.rcPaint.left)
					continue;

				int ix1 = VDClampToSint32(x1);
				int ix2 = VDClampToSint32(x2);

				if (ix2 > ix1) {
					if(&ev==selection){
						SelectObject(hdc,pen2);
						SelectObject(hdc,br2);
						Rectangle(hdc, ix1, y1, ix2, y2);
						SelectObject(hdc,pen1);
					} else {
						uint32 flags = mScopes[ev.mScopeId].flags;
						if(flags & vdprofiler_flag_wait)
							SelectObject(hdc,br3);
						else
							SelectObject(hdc,br1);
						Rectangle(hdc, ix1, y1, ix2, y2);
					}

					const char *s = mScopes[ev.mScopeId].name;

					if (ix2 - ix1 > 4 && y2 - y1 > 4) {
						RECT inside;
						inside.left = ix1 + 2;
						inside.right = ix2 - 2;
						inside.top = y1 + 2;
						inside.bottom = y2 - 2;

						if (ev.mAncillaryData) {
							mTempStr.sprintf("%s (%u)", s, ev.mAncillaryData);
							DrawTextA(hdc, mTempStr.data(), mTempStr.size(), &inside, DT_LEFT | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE);
						} else {
							DrawTextA(hdc, s, strlen(s), &inside, DT_LEFT | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE);
						}
					}
				}
			}}

			{for(uint32 j=0; j<n; ++j) {
				const VDProfileTrackedEvent& ev = events[j];
				uint32 flags = mScopes[ev.mScopeId].flags;
				if (!(flags & (vdprofiler_flag_loop|vdprofiler_flag_event))) continue;
				if (use_range_start && ev.mStartTime<range_start) continue;
				if (use_range_end && ev.mStartTime>range_end) break;

				double xf1 = (double)(sint64)(ev.mStartTime - baseTime) * pixelsPerTick;

				sint64 x1 = VDCeilToInt64(xf1 - 0.5) - mBasePixelOffset;

				if (x1-3 >= ps.rcPaint.right)
					break;

				if (x1+3 <= ps.rcPaint.left)
					continue;

				int ix1 = VDClampToSint32(x1);

				SelectObject(hdc,br4);
				Rectangle(hdc, ix1-2, (y1+y2)/2-2, ix1+3, (y1+y2)/2+3);
			}}
		}

		if (hOldFont)
			SelectObject(hdc, mhfont);

		SelectObject(hdc,pen0);
		SelectObject(hdc,br0);

		EndPaint(mhwnd, &ps);

		display_event_count = event_count;
	}
}

struct EventSort {
	bool operator()(const VDProfileTrackedEvent *x, const VDProfileTrackedEvent *y) {
		return x->mStartTime<y->mStartTime;
	}
};

struct EventSortId {
	bool operator()(const VDProfileTrackedEvent *x, const VDProfileTrackedEvent *y) {
		if (x->mScopeId==y->mScopeId)
			return x->mStartTime<y->mStartTime;
		return x->mScopeId<y->mScopeId;
	}
};

void VDRTProfileDisplay2::UpdateSummary() {
	vdfastvector<const VDProfileTrackedEvent *>	list;

	size_t threadCount = mThreadProfiles.size();
	double msPerTick = VDGetPreciseSecondsPerTick()*1000;

	for(size_t i=0; i<threadCount; ++i) {
		VDEventProfileThreadTracker *tracker = mThreadProfiles[i];

		if (!tracker)
			continue;

		const vdspan<VDProfileTrackedEvent>& events = tracker->GetEvents();

		if (!events.empty()) {
			for(uint32 j=0; j<events.size(); ++j) {
				const VDProfileTrackedEvent& ev = events[j];
				if (ev.mChildScopes == 0xFFFFFFFF)
					continue;
				//if (ev.mChildScopes != 0)
				//	continue;

				if (use_range_start && ev.mStartTime<range_start) continue;
				if (use_range_end && ev.mStartTime>range_end) break;

				list.push_back(&ev);
			}
		}
	}

	std::sort(list.begin(), list.end(), EventSortId());

	HWND wnd = mhwndSummary;
	SendMessage(wnd, LB_RESETCONTENT, 0, 0);
	int tabs[] = {30,80,130,180};
	SendMessage(wnd, LB_SETTABSTOPS, 4, (LPARAM)tabs);
	SendMessageA(wnd, LB_ADDSTRING, 0, (LPARAM)"num\t min\t average\t rms\t id");

	std::vector<uint32> scope_comment;

	for(size_t i=0; i<list.size();) {
		uint32 id = list[i]->mScopeId;
		bool loop = (mScopes[id].flags & vdprofiler_flag_loop)!=0;
		uint64 t0 = list[i]->mStartTime;
		if (loop) i++;
		double d0 = 0;
		double da = 0;
		double ds = 0;
		int n = 0;
		bool childScopes = false;
		for(; i<list.size(); ++i) {
			const VDProfileTrackedEvent& ev = *list[i];
			if (ev.mScopeId!=id) break;
			uint64 dt = loop ? ev.mStartTime-t0 : ev.mEndTime-ev.mStartTime;
			t0 = ev.mStartTime;
			double d = (double)(dt) * msPerTick;
			da += d;
			ds += d*d;
			if(n==0) d0 = d;
			if(d<d0) d0 = d;
			if(ev.mChildScopes) childScopes = true;
			n++;
		}

		if (da/n<0.1) continue;

		const char *s = mScopes[id].name;
		const char *s1 = childScopes ? "+":"";
		if (n>0)
			mTempStr.sprintf("%d \t %5.1fms \t %5.1fms \t %5.1fms \t %s%s", n, d0, da/n, sqrt(ds/n), s, s1);
		else
			mTempStr.sprintf("- \t - \t - \t - \t %s%s", s, s1);
		SendMessageA(wnd, LB_ADDSTRING, 0, (LPARAM)mTempStr.c_str());

		if (mScopes[id].comment) scope_comment.push_back(id);
	}

	if (use_range_start && use_range_end) {
		SendMessageA(wnd, LB_ADDSTRING, 0, (LPARAM)"");

		double d = (double)(range_end-range_start) * msPerTick;
		mTempStr.sprintf("  \t \t %5.1fms \t \t %s", d, "Benchmark total");
		SendMessageA(wnd, LB_ADDSTRING, 0, (LPARAM)mTempStr.c_str());

		uint64 sw = 0;
		uint64 sm = 0;

		const vdspan<VDProfileSample>& events = sample_cpu;
		size_t n = events.size();
		{for(uint32 j=1; j<n; ++j) {
			const VDProfileSample& ev = events[j];
			if (ev.mTimestamp<range_start) continue;
			if (ev.mTimestamp>range_end) break;
			uint64 d1 = ev.mTimestamp-events[j-1].mTimestamp;
			sw += d1;
			sm += d1*ev.data;
		}}

		mTempStr.sprintf("  \t \t %d%% \t \t %s", int(sm/sw), "CPU usage");
		SendMessageA(wnd, LB_ADDSTRING, 0, (LPARAM)mTempStr.c_str());
	}

	if (scope_comment.size()) {
		SendMessageA(wnd, LB_ADDSTRING, 0, (LPARAM)"");
		for(size_t i=0; i<scope_comment.size(); i++) {
			uint32 id = scope_comment[i];

			const char *s = mScopes[id].name;
			mTempStr.sprintf("* %s: %s", s, mScopes[id].comment);
			SendMessageA(wnd, LB_ADDSTRING, 0, (LPARAM)mTempStr.c_str());
		}
	}
}

void VDRTProfileDisplay2::UpdateList() {
	SendMessage(mhwndList, WM_SETREDRAW, false, 0);
	SendMessage(mhwndList, LB_RESETCONTENT, 0, 0);
	int tabs[] = {50,90,130,200};
	SendMessage(mhwndList, LB_SETTABSTOPS, 4, (LPARAM)tabs);

	size_t threadCount = mThreadProfiles.size();
	double msPerTick = VDGetPreciseSecondsPerTick()*1000;

	int list_max = 5000;
	mSortedList.clear();

	for(size_t i=0; i<threadCount; ++i) {
		VDEventProfileThreadTracker *tracker = mThreadProfiles[i];

		if (!tracker)
			continue;

		const vdspan<VDProfileTrackedEvent>& events = tracker->GetEvents();

		if (!events.empty()) {
			for(uint32 j=0; j<events.size(); ++j) {
				const VDProfileTrackedEvent& ev = events[j];
				if (ev.mChildScopes == 0xFFFFFFFF)
					continue;
				//if (ev.mChildScopes != 0)
				//	continue;

				if (use_range_start && ev.mStartTime<range_start) continue;
				if (use_range_end && ev.mStartTime>range_end) break;

				int flags = mScopes[ev.mScopeId].flags;
				if (flags & vdprofiler_flag_wait) {
					double d = (double)(ev.mEndTime-ev.mStartTime) * msPerTick;
					if (d<0.1) continue;
				}

				mSortedList.push_back(&ev);
				if (mSortedList.size()>=list_max) break;
			}
		}
		if (mSortedList.size()>=list_max) break;
	}

	std::sort(mSortedList.begin(), mSortedList.end(), EventSort());

	for(size_t i=0; i<mSortedList.size(); ++i) {
		const VDProfileTrackedEvent& ev = *mSortedList[i];
		const char *s = mScopes[ev.mScopeId].name;
		int flags = mScopes[ev.mScopeId].flags;
		double t = (double)(ev.mStartTime-baseTime) * msPerTick;

		if (flags & (vdprofiler_flag_event|vdprofiler_flag_loop)){
			if (ev.mAncillaryData) {
				mTempStr.sprintf("%5.1fms \t event \t (%u) \t %s", t, ev.mAncillaryData, s);
			} else {
				mTempStr.sprintf("%5.1fms \t event \t \t %s", t, s);
			}
		} else {
			double d = (double)(ev.mEndTime-ev.mStartTime) * msPerTick;
			const char* s1 = ev.mChildScopes ? "+":"";

			if (ev.mAncillaryData) {
				mTempStr.sprintf("%5.1fms \t %5.1fms \t (%u) \t %s%s", t, d, ev.mAncillaryData, s, s1);
			} else {
				mTempStr.sprintf("%5.1fms \t %5.1fms \t \t %s%s", t, d, s, s1);
			}
		}

		SendMessageA(mhwndList, LB_ADDSTRING, 0, (LPARAM)mTempStr.c_str());
	}

	if (mSortedList.size()>=list_max)
		SendMessageA(mhwndList, LB_ADDSTRING, 0, (LPARAM)"\t\t\t truncated...");

	SendMessage(mhwndList, WM_SETREDRAW, true, 0);
	InvalidateRect(mhwndList,0,true);
	UpdateSummary();

	list_event_count = event_count;
}

void VDRTProfileDisplay2::UpdateListTop(bool sync) {
	int x = SendMessage(mhwndList,LB_GETTOPINDEX,0,0);
	if (x!=list_top) {
		list_top = x;

		if (sync) {
			GoToEvent(*mSortedList[x]);
		}
	}
}

void VDRTProfileDisplay2::OnTimer() {
	if (mode==0) return;

	VDEventProfilerW32 *p = static_cast<VDEventProfilerW32 *>(g_pVDEventProfiler);

	if (mode==2){
		if (GetActiveWindow()==GetParent(mhwnd) && selection) return;
		if (GetCapture()==mhwnd) return;

		DeleteThreadProfiles();
		UpdateThreadProfiles();
		p->Clear();
		mBaseTimeOffset = 0;
		mBasePixelOffset = 0;
		selection = 0;
		list_top = 0;

		if (event_count!=display_event_count || event_count>0){
			InvalidateRect(mhwnd, NULL, TRUE);
			UpdateWindow(mhwnd);
			SendMessage(GetParent(mhwnd),WM_SIZE,0,0);
		}
		if (event_count!=list_event_count || event_count>0){
			UpdateList();
		}

	} else {
		if (p->reset_count>reset_count) {
			DeleteThreadProfiles();
			reset_count = p->reset_count;
			display_event_count = -1;
			list_event_count = -1;
			selection = 0;
		}

		UpdateThreadProfiles();

		if (event_count!=display_event_count){
			VDPROFILEBEGINEX2("Profile1",0,vdprofiler_flag_profile);
			if (use_range_end) SetDisplayAutoZoom();
			InvalidateRect(mhwnd, NULL, TRUE);
			UpdateWindow(mhwnd);
			SendMessage(GetParent(mhwnd),WM_SIZE,0,0);
			VDPROFILEEND();
		}

		if (event_count!=list_event_count){
			VDPROFILEBEGINEX2("Profile2",0,vdprofiler_flag_profile);
			UpdateList();
			VDPROFILEEND();
		}

		if (use_range_end) mode = 0;
	}
}

void VDRTProfileDisplay2::OnSetFont(HFONT hfont, bool bRedraw) {
	mhfont = hfont;
	if (bRedraw)
		InvalidateRect(mhwnd, NULL, TRUE);

	// stuff in some reasonable defaults if we fail measuring font metrics
	mFontHeight				= 16;
	mFontAscent				= 12;
	mFontInternalLeading	= 0;
	mScrollHeight	= 60;

	// measure font metrics
	if (HDC hdc = GetDC(mhwnd)) {
		HGDIOBJ hOldFont = 0;
		if (mhfont)
			hOldFont = SelectObject(hdc, mhfont);

		TEXTMETRIC tm;
		if (GetTextMetrics(hdc, &tm)) {
			mFontHeight				= tm.tmHeight;
			mFontAscent				= tm.tmAscent;
			mFontInternalLeading	= tm.tmInternalLeading;
		}

		SIZE sz;
		if (GetTextExtentPoint32A(hdc, "00000000", 8, &sz))
			mMajorScaleMinWidth = sz.cx + 10;

		if (hOldFont)
			SelectObject(hdc, mhfont);
	}
}

bool VDRTProfileDisplay2::OnKey(INT wParam) {
	if (wParam == VK_MULTIPLY) {
		SetDisplayAutoZoom();
		return true;
	} else if (wParam == VK_LEFT || wParam==VK_SUBTRACT) {
		SetDisplayResolution(mDisplayResolution - 1);
		return true;
	} else if (wParam == VK_RIGHT || wParam==VK_ADD) {
		SetDisplayResolution(mDisplayResolution + 1);
		return true;
	} else if (wParam == 'X') {
		if (::GetKeyState(VK_CONTROL) < 0) {
			Clear();
			return true;
		}
	}
	return false;
}

void VDRTProfileDisplay2::Clear() {
	VDEventProfilerW32 *p = static_cast<VDEventProfilerW32 *>(g_pVDEventProfiler);

	if (p)
		p->Clear();

	DeleteThreadProfiles();

	mBaseTimeOffset = 0;
	mBasePixelOffset = 0;
	InvalidateRect(mhwnd, NULL, TRUE);
	SendMessage(mhwndList, LB_RESETCONTENT, 0, 0);
	SendMessage(mhwndSummary, LB_RESETCONTENT, 0, 0);

	selection = 0;
	list_top = 0;
	display_event_count = 0;
	list_event_count = 0;
	mode = 1;
}

void VDRTProfileDisplay2::DeleteThreadProfiles() {
	while(!mThreadProfiles.empty()) {
		VDEventProfileThreadTracker *p = mThreadProfiles.back();
		mThreadProfiles.pop_back();

		delete p;
	}

	sample_cpu.clear();
}

void VDRTProfileDisplay2::UpdateThreadProfiles() {
	VDEventProfilerW32 *p = static_cast<VDEventProfilerW32 *>(g_pVDEventProfiler);

	if (!p)
		return;

	event_count = 0;

	uint32 n = p->GetThreadCount();

	if (n > mThreadProfiles.size())
		mThreadProfiles.resize(n, NULL);

	for(uint32 i=0; i<n; ++i) {
		VDEventProfileThreadTracker *tracker = mThreadProfiles[i];

		if (!tracker) {
			tracker = new VDEventProfileThreadTracker(i);

			mThreadProfiles[i] = tracker;
		}

		tracker->Update();

		// do not trigger refresh for own thread
		if (!(tracker->GetUsedFlags() & vdprofiler_flag_profile))
			event_count += tracker->GetEvents().size();
	}

	p->UpdateScopes(mScopes);

	p->UpdateSample(IVDEventProfiler::sample_cpu,sample_cpu);

	UpdateRange();
}

void VDRTProfileDisplay2::UpdateScale() {
	double decimalPlaces = ceil(log10((double)(mMajorScaleMinWidth << -mDisplayResolution)));
	mMajorScaleDecimalPlaces = 6 - (int)decimalPlaces;

	if (mMajorScaleDecimalPlaces < 0)
		mMajorScaleDecimalPlaces = 0;

	if (mMajorScaleDecimalPlaces > 6)
		mMajorScaleDecimalPlaces = 6;

	mMajorScaleInterval = VDRoundToInt(pow(10.0, decimalPlaces));
}

void VDRTProfileDisplay2::SetDisplayResolution(int res) {
	if (res > 0)
		res = 0;

	if (res < -20)
		res = -20;

	if (res == mDisplayResolution)
		return;

	int halfw = 0;

	RECT r;
	if (::GetClientRect(mhwnd, &r))
		halfw = r.right >> 1;

	mBaseTimeOffset += (halfw << -mDisplayResolution) - (halfw << -res);
	if (mBaseTimeOffset < 0)
		mBaseTimeOffset = 0;

	mDisplayResolution = res;
	mBasePixelOffset = VDRoundToInt64((double)mBaseTimeOffset * pow(2.0, (double)mDisplayResolution));

	if (use_range_end) {
		double pixelsPerMicrosec = pow(2.0, (double)res);
		double pixelsPerTick = VDGetPreciseSecondsPerTick() * 1000000.0 * pixelsPerMicrosec;
		double x1 = (double)(sint64)(range_end - baseTime) * pixelsPerTick;
		if (x1<r.right) {
			mBaseTimeOffset = 0;
			mBasePixelOffset = 0;
		}
	}

	UpdateScale();

	InvalidateRect(mhwnd, NULL, TRUE);
}

void VDRTProfileDisplay2::SetDisplayAutoZoom() {
	if (!use_range_end) return;
	RECT r0;
	GetClientRect(mhwnd,&r0);

	for(int res=-20; res<0; res++){
		double pixelsPerMicrosec = pow(2.0, (double)res);
		double pixelsPerTick = VDGetPreciseSecondsPerTick() * 1000000.0 * pixelsPerMicrosec;
		double x1 = (double)(sint64)(range_end - baseTime) * pixelsPerTick;
		if (x1>r0.right) {
			SetDisplayResolution(res-1);
			mBaseTimeOffset = 0;
			return;
		}
	}
}

void VDRTProfileDisplay2::ScrollByPixels(int dx, int dy) {
	sint64 newOffset = mBaseTimeOffset + (dx << -mDisplayResolution);
	if (newOffset < 0)
		newOffset = 0;

	if (mBaseTimeOffset != newOffset) {
		mBaseTimeOffset = newOffset;

		sint64 pixOffset = VDRoundToInt64((double)mBaseTimeOffset * pow(2.0, (double)mDisplayResolution));

		if (mBasePixelOffset != pixOffset) {
			sint64 pdx = mBasePixelOffset - pixOffset;
			mBasePixelOffset = pixOffset;

			::ScrollWindow(mhwnd, (int)pdx, 0, NULL, NULL);
		}
	}
}
