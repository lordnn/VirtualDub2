//	VirtualDub - Video processing and capture application
//	Copyright (C) 1998-2009 Avery Lee
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
//	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

#ifndef f_VD2_FILTERFRAMECONVERTER_H
#define f_VD2_FILTERFRAMECONVERTER_H

#include "FilterInstance.h"
#include "FilterFrameManualSource.h"
#include <vd2/Kasumi/blitter.h>

class VDFilterFrameConverter : public VDFilterFrameManualSource {
	friend class VDFilterFrameConverterNode;

	VDFilterFrameConverter(const VDFilterFrameConverter&);
	VDFilterFrameConverter& operator=(const VDFilterFrameConverter&);
public:
	VDFilterFrameConverter();
	~VDFilterFrameConverter() override;

	void Init(IVDFilterFrameSource *source, const VDPixmapLayout& outputLayout, const VDPixmapLayout *sourceLayoutOverride, bool normalize16);
	void Start(IVDFilterFrameEngine *frameEngine) override;
	void Stop() override;

	bool GetDirectMapping(sint64 outputFrame, sint64& sourceFrame, int& sourceIndex) override;
	sint64 GetSourceFrame(sint64 outputFrame) override;
	sint64 GetSymbolicFrame(sint64 outputFrame, IVDFilterFrameSource *source) override;
	sint64 GetNearestUniqueFrame(sint64 outputFrame) override;

	int AllocateNodes(int threads) override;
	RunResult RunRequests(const uint32 *batchNumberLimit, int index) override;
	RunResult RunProcess(int index) override;

protected:
	bool InitNewRequest(VDFilterFrameRequest *req, sint64 outputFrame, bool writable, uint32 batchNumber) override;

	IVDFilterFrameEngine *mpEngine;
	IVDFilterFrameSource *mpSource;
	VDPixmapLayout		mSourceLayout;
	bool mNormalize16;

	VDFilterFrameConverterNode* node;
	int node_count;
	uintptr scope;
};

#endif
