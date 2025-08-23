//	VirtualDub - Video processing and capture application
//	Graphics support library
//	Copyright (C) 1998-2008 Avery Lee
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

#ifndef f_VD2_KASUMI_UBERBLIT_SWIZZLE_H
#define f_VD2_KASUMI_UBERBLIT_SWIZZLE_H

#include <vd2/system/cpuaccel.h>
#include "uberblit_base.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	generic converters
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_Swap8In16 : public VDPixmapGenWindowBasedOneSource {
public:
	void Init(IVDPixmapGen *gen, int srcIndex, uint32 w, uint32 h, uint32 bpr);
	void Start() override;

	uint32 GetType(uint32 index) const override;

	const char* dump_name() override { return "Swap8In16"; }

protected:
	void Compute(void *dst0, sint32 y) override;

	uint32 mRowLength;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	32-bit deinterleavers
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_8In16 : public VDPixmapGenWindowBasedOneSource {
public:
	void Init(IVDPixmapGen *gen, int srcIndex, int offset, uint32 w, uint32 h) {
		InitSource(gen, srcIndex);
		mOffset = offset;
		SetOutputSize(w, h);
		gen->AddWindowRequest(0, 0);
	}

	void Start() override {
		StartWindow(mWidth);
	}

	uint32 GetType(uint32 index) const override {
		return (mpSrc->GetType(mSrcIndex) & ~kVDPixType_Mask) | kVDPixType_8;
	}

	const char* dump_name() override {
		if(mOffset==0) return "8In16.0";
		if(mOffset==1) return "8In16.1";
		return "8In16";
	}

protected:
	void Compute(void *dst0, sint32 y) override {
		const uint8 *srcp = (const uint8 *)mpSrc->GetRow(y, mSrcIndex) + mOffset;
		uint8 *dst = (uint8 *)dst0;
		sint32 w = mWidth;
		for(sint32 x=0; x<w; ++x) {
			*dst++ = *srcp;
			srcp += 2;
		}
	}

	int mOffset;
};

class VDPixmapGen_8In32 : public VDPixmapGenWindowBasedOneSource {
public:
	void Init(IVDPixmapGen *gen, int srcIndex, int offset, uint32 w, uint32 h) {
		InitSource(gen, srcIndex);
		mOffset = offset;
		SetOutputSize(w, h);
		gen->AddWindowRequest(0, 0);
	}

	void Start() override {
		StartWindow(mWidth);
	}

	uint32 GetType(uint32 index) const override {
		return (mpSrc->GetType(mSrcIndex) & ~kVDPixType_Mask) | kVDPixType_8;
	}

	const char* dump_name() override {
		if(mOffset==0) return "8In32.0";
		if(mOffset==1) return "8In32.1";
		if(mOffset==2) return "8In32.2";
		if(mOffset==3) return "8In32.3";
		return "8In32";
	}

protected:
	void Compute(void *dst0, sint32 y) override {
		const uint8 *srcp = (const uint8 *)mpSrc->GetRow(y, mSrcIndex) + mOffset;
		uint8 *dst = (uint8 *)dst0;
		sint32 w = mWidth;
		for(sint32 x=0; x<w; ++x) {
			*dst++ = *srcp;
			srcp += 4;
		}
	}

	int mOffset;
};

class VDPixmapGen_16In32 : public VDPixmapGenWindowBasedOneSource {
public:
	void Init(IVDPixmapGen *gen, int srcIndex, int offset, uint32 w, uint32 h) {
		InitSource(gen, srcIndex);
		mOffset = offset;
		SetOutputSize(w, h);
		gen->AddWindowRequest(0, 0);
	}

	void Start() override {
		StartWindow(mWidth*2);
	}

	uint32 GetType(uint32 index) const override {
		return (mpSrc->GetType(mSrcIndex) & ~kVDPixType_Mask) | kVDPixType_16_LE;
	}

	const char* dump_name() override {
		if(mOffset==0) return "16In32.0";
		if(mOffset==1) return "16In32.1";
		return "16In32";
	}

protected:
	void Compute(void *dst0, sint32 y) override;

	int mOffset;
};

class VDPixmapGen_16In64 : public VDPixmapGenWindowBasedOneSource {
public:
	void Init(IVDPixmapGen *gen, int srcIndex, int offset, uint32 w, uint32 h) {
		InitSource(gen, srcIndex);
		mOffset = offset;
		SetOutputSize(w, h);
		gen->AddWindowRequest(0, 0);
	}

	void Start() override {
		StartWindow(mWidth*2);
	}

	uint32 GetType(uint32 index) const override {
		return (mpSrc->GetType(mSrcIndex) & ~kVDPixType_Mask) | kVDPixType_16_LE;
	}

	const char* dump_name() override {
		if(mOffset==0) return "16In64.0";
		if(mOffset==1) return "16In64.1";
		if(mOffset==2) return "16In64.2";
		if(mOffset==3) return "16In64.3";
		return "16In64";
	}

protected:
	void Compute(void *dst0, sint32 y) override;

	int mOffset;
};

class VDPixmapGen_32In128 : public VDPixmapGenWindowBasedOneSource {
public:
	void Init(IVDPixmapGen *gen, int srcIndex, int offset, uint32 w, uint32 h) {
		InitSource(gen, srcIndex);
		mOffset = offset;
		SetOutputSize(w, h);
		gen->AddWindowRequest(0, 0);
	}

	void Start() override {
		StartWindow(mWidth*4);
	}

	uint32 GetType(uint32 index) const override {
		return (mpSrc->GetType(mSrcIndex) & ~kVDPixType_Mask) | kVDPixType_32F_LE;
	}

	const char* dump_name() override {
		if(mOffset==0) return "32In128.0";
		if(mOffset==1) return "32In128.1";
		if(mOffset==2) return "32In128.2";
		if(mOffset==3) return "32In128.3";
		return "32In128";
	}

protected:
	void Compute(void *dst0, sint32 y) override;

	int mOffset;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	16-bit interleavers
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_B8x2_To_B8R8 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *srcCb, uint32 srcindexCb, IVDPixmapGen *srcCr, uint32 srcindexCr);
	void Start() override;
	uint32 GetType(uint32 output) const override;

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo unused;
		mpSrcCb->TransformPixmapInfo(src,unused);
		mpSrcCr->TransformPixmapInfo(src,unused);
	}

	IVDPixmapGen* dump_src(int index) override{
		if(index==0) return mpSrcCb;
		if(index==1) return mpSrcCr;
		return 0; 
	}

	const char* dump_name() override { return "B8x2_To_B8R8"; }

protected:
	void Compute(void *dst0, sint32 y) override;

	IVDPixmapGen *mpSrcCb;
	uint32 mSrcIndexCb;
	IVDPixmapGen *mpSrcCr;
	uint32 mSrcIndexCr;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	32-bit interleavers
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_B8x3_To_G8B8_G8R8 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *srcCr, uint32 srcindexCr, IVDPixmapGen *srcY, uint32 srcindexY, IVDPixmapGen *srcCb, uint32 srcindexCb) {
		mpSrcY = srcY;
		mSrcIndexY = srcindexY;
		mpSrcCb = srcCb;
		mSrcIndexCb = srcindexCb;
		mpSrcCr = srcCr;
		mSrcIndexCr = srcindexCr;
		mWidth = srcY->GetWidth(srcindexY);
		mHeight = srcY->GetHeight(srcindexY);

		srcY->AddWindowRequest(0, 0);
		srcCb->AddWindowRequest(0, 0);
		srcCr->AddWindowRequest(0, 0);
	}

	void Start() override {
		mpSrcY->Start();
		mpSrcCb->Start();
		mpSrcCr->Start();

		StartWindow(((mWidth + 1) & ~1) * 2);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo unused;
		mpSrcY->TransformPixmapInfo(src,unused);
		mpSrcCb->TransformPixmapInfo(src,unused);
		mpSrcCr->TransformPixmapInfo(src,unused);
	}

	uint32 GetType(uint32 output) const override {
		return (mpSrcY->GetType(mSrcIndexY) & ~kVDPixType_Mask) | kVDPixType_B8G8_R8G8;
	}

	IVDPixmapGen* dump_src(int index) override {
		if(index==0) return mpSrcY;
		if(index==1) return mpSrcCb;
		if(index==2) return mpSrcCr;
		return 0; 
	}

	const char* dump_name() override { return "B8x3_To_G8B8_G8R8"; }

protected:
	void Compute(void *dst0, sint32 y) override {
		uint8 *VDRESTRICT dst = (uint8 *)dst0;
		const uint8 *VDRESTRICT srcY = (const uint8 *)mpSrcY->GetRow(y, mSrcIndexY);
		const uint8 *VDRESTRICT srcCb = (const uint8 *)mpSrcCb->GetRow(y, mSrcIndexCb);
		const uint8 *VDRESTRICT srcCr = (const uint8 *)mpSrcCr->GetRow(y, mSrcIndexCr);

		sint32 w = mWidth >> 1;
		for(sint32 x=0; x<w; ++x) {
			uint8 y1 = srcY[0];
			uint8 cb = srcCb[0];
			uint8 y2 = srcY[1];
			uint8 cr = srcCr[0];

			dst[0] = y1;
			dst[1] = cb;
			dst[2] = y2;
			dst[3] = cr;

			srcY += 2;
			++srcCb;
			++srcCr;
			dst += 4;
		}

		if (mWidth & 1) {
			uint8 y1 = srcY[0];
			uint8 cb = srcCb[0];
			uint8 cr = srcCr[0];

			dst[0] = y1;
			dst[1] = cb;
			dst[2] = y1;
			dst[3] = cr;
		}
	}

	IVDPixmapGen *mpSrcY;
	uint32 mSrcIndexY;
	IVDPixmapGen *mpSrcCb;
	uint32 mSrcIndexCb;
	IVDPixmapGen *mpSrcCr;
	uint32 mSrcIndexCr;
};

class VDPixmapGen_B8x3_To_B8G8_R8G8 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *srcCr, uint32 srcindexCr, IVDPixmapGen *srcY, uint32 srcindexY, IVDPixmapGen *srcCb, uint32 srcindexCb) {
		mpSrcY = srcY;
		mSrcIndexY = srcindexY;
		mpSrcCb = srcCb;
		mSrcIndexCb = srcindexCb;
		mpSrcCr = srcCr;
		mSrcIndexCr = srcindexCr;
		mWidth = srcY->GetWidth(srcindexY);
		mHeight = srcY->GetHeight(srcindexY);

		srcY->AddWindowRequest(0, 0);
		srcCb->AddWindowRequest(0, 0);
		srcCr->AddWindowRequest(0, 0);
	}

	void Start() override {
		mpSrcY->Start();
		mpSrcCb->Start();
		mpSrcCr->Start();

		StartWindow(((mWidth + 1) & ~1) * 2);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo unused;
		mpSrcY->TransformPixmapInfo(src,unused);
		mpSrcCb->TransformPixmapInfo(src,unused);
		mpSrcCr->TransformPixmapInfo(src,unused);
	}

	uint32 GetType(uint32 output) const override {
		return (mpSrcY->GetType(mSrcIndexY) & ~kVDPixType_Mask) | kVDPixType_G8B8_G8R8;
	}

	IVDPixmapGen* dump_src(int index) override {
		if(index==0) return mpSrcY;
		if(index==1) return mpSrcCb;
		if(index==2) return mpSrcCr;
		return 0; 
	}

	const char* dump_name() override { return "B8x3_To_B8G8_R8G8"; }

protected:
	void Compute(void *dst0, sint32 y) override {
		uint8 * VDRESTRICT dst = (uint8 *)dst0;
		const uint8 *VDRESTRICT srcY = (const uint8 *)mpSrcY->GetRow(y, mSrcIndexY);
		const uint8 *VDRESTRICT srcCb = (const uint8 *)mpSrcCb->GetRow(y, mSrcIndexCb);
		const uint8 *VDRESTRICT srcCr = (const uint8 *)mpSrcCr->GetRow(y, mSrcIndexCr);

		sint32 w2 = mWidth >> 1;
		for(sint32 x=0; x<w2; ++x) {
			uint8 cb = srcCb[0];
			uint8 y1 = srcY[0];
			uint8 cr = srcCr[0];
			uint8 y2 = srcY[1];

			dst[0] = cb;
			dst[1] = y1;
			dst[2] = cr;
			dst[3] = y2;
			dst += 4;
			srcY += 2;
			++srcCb;
			++srcCr;
		}

		if (mWidth & 1) {
			uint8 cb = srcCb[0];
			uint8 y1 = srcY[0];
			uint8 cr = srcCr[0];

			dst[0] = cb;
			dst[1] = y1;
			dst[2] = cr;
			dst[3] = y1;
		}
	}

	IVDPixmapGen *mpSrcY;
	uint32 mSrcIndexY;
	IVDPixmapGen *mpSrcCb;
	uint32 mSrcIndexCb;
	IVDPixmapGen *mpSrcCr;
	uint32 mSrcIndexCr;
};

class VDPixmapGen_B8x3_To_X8R8G8B8 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *srcCr, uint32 srcindexCr, IVDPixmapGen *srcY, uint32 srcindexY, IVDPixmapGen *srcCb, uint32 srcindexCb) {
		mpSrcY = srcY;
		mSrcIndexY = srcindexY;
		mpSrcCb = srcCb;
		mSrcIndexCb = srcindexCb;
		mpSrcCr = srcCr;
		mSrcIndexCr = srcindexCr;
		mWidth = srcY->GetWidth(srcindexY);
		mHeight = srcY->GetHeight(srcindexY);

		srcY->AddWindowRequest(0, 0);
		srcCb->AddWindowRequest(0, 0);
		srcCr->AddWindowRequest(0, 0);
	}

	void Start() override {
		mpSrcY->Start();
		mpSrcCb->Start();
		mpSrcCr->Start();

		StartWindow(mWidth * 4);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo unused;
		mpSrcY->TransformPixmapInfo(src,unused);
		mpSrcCb->TransformPixmapInfo(src,unused);
		mpSrcCr->TransformPixmapInfo(src,unused);
	}

	uint32 GetType(uint32 output) const override {
		return (mpSrcY->GetType(mSrcIndexY) & ~kVDPixType_Mask) | kVDPixType_8888;
	}

	IVDPixmapGen* dump_src(int index) override {
		if(index==0) return mpSrcCr;
		if(index==1) return mpSrcY;
		if(index==2) return mpSrcCb;
		return 0; 
	}

	const char* dump_name() override { return "B8x3_To_X8R8G8B8"; }

protected:
	void Compute(void *dst0, sint32 y) override {
		uint8 *dst = (uint8 *)dst0;
		const uint8 *srcY = (const uint8 *)mpSrcY->GetRow(y, mSrcIndexY);
		const uint8 *srcCb = (const uint8 *)mpSrcCb->GetRow(y, mSrcIndexCb);
		const uint8 *srcCr = (const uint8 *)mpSrcCr->GetRow(y, mSrcIndexCr);

		for(sint32 x=0; x<mWidth; ++x) {
			uint8 y = *srcY++;
			uint8 cb = *srcCb++;
			uint8 cr = *srcCr++;

			dst[0] = cb;
			dst[1] = y;
			dst[2] = cr;
			dst[3] = 255;
			dst += 4;
		}
	}

	IVDPixmapGen *mpSrcY;
	uint32 mSrcIndexY;
	IVDPixmapGen *mpSrcCb;
	uint32 mSrcIndexCb;
	IVDPixmapGen *mpSrcCr;
	uint32 mSrcIndexCr;
};

class VDPixmapGen_B8x4_To_A8R8G8B8 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *srcR, uint32 srcindexR, IVDPixmapGen *srcG, uint32 srcindexG, IVDPixmapGen *srcB, uint32 srcindexB, IVDPixmapGen *srcA, uint32 srcindexA) {
		mpSrcR = srcR;
		mSrcIndexR = srcindexR;
		mpSrcG = srcG;
		mSrcIndexG = srcindexG;
		mpSrcB = srcB;
		mSrcIndexB = srcindexB;
		mpSrcA = srcA;
		mSrcIndexA = srcindexA;
		mWidth = srcR->GetWidth(srcindexR);
		mHeight = srcR->GetHeight(srcindexR);

		srcR->AddWindowRequest(0, 0);
		srcG->AddWindowRequest(0, 0);
		srcB->AddWindowRequest(0, 0);
		srcA->AddWindowRequest(0, 0);
	}

	void Start() override {
		mpSrcR->Start();
		mpSrcG->Start();
		mpSrcB->Start();
		mpSrcA->Start();

		StartWindow(mWidth * 4);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo unused;
		mpSrcR->TransformPixmapInfo(src,unused);
		mpSrcG->TransformPixmapInfo(src,unused);
		mpSrcB->TransformPixmapInfo(src,unused);
		FilterModPixmapInfo buf;
		mpSrcA->TransformPixmapInfo(src,buf);
		dst.copy_alpha(buf);
	}

	uint32 GetType(uint32 output) const override {
		return (mpSrcR->GetType(mSrcIndexR) & ~kVDPixType_Mask) | kVDPixType_8888;
	}

	IVDPixmapGen* dump_src(int index) override {
		if(index==0) return mpSrcR;
		if(index==1) return mpSrcG;
		if(index==2) return mpSrcB;
		if(index==3) return mpSrcA;
		return 0; 
	}

	const char* dump_name() override { return "B8x4_To_A8R8G8B8"; }

protected:
	void Compute(void *dst0, sint32 y) override {
		uint8 *dst = (uint8 *)dst0;
		const uint8 *srcR = (const uint8 *)mpSrcR->GetRow(y, mSrcIndexR);
		const uint8 *srcG = (const uint8 *)mpSrcG->GetRow(y, mSrcIndexG);
		const uint8 *srcB = (const uint8 *)mpSrcB->GetRow(y, mSrcIndexB);
		const uint8 *srcA = (const uint8 *)mpSrcA->GetRow(y, mSrcIndexA);

		for(sint32 x=0; x<mWidth; ++x) {
			uint8 r = *srcR++;
			uint8 g = *srcG++;
			uint8 b = *srcB++;
			uint8 a = *srcA++;

			dst[0] = b;
			dst[1] = g;
			dst[2] = r;
			dst[3] = a;
			dst += 4;
		}
	}

	IVDPixmapGen *mpSrcR;
	uint32 mSrcIndexR;
	IVDPixmapGen *mpSrcG;
	uint32 mSrcIndexG;
	IVDPixmapGen *mpSrcB;
	uint32 mSrcIndexB;
	IVDPixmapGen *mpSrcA;
	uint32 mSrcIndexA;
};

class VDPixmapGen_B16x3_To_Y416 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *srcCr, uint32 srcindexCr, IVDPixmapGen *srcY, uint32 srcindexY, IVDPixmapGen *srcCb, uint32 srcindexCb) {
		mpSrcY = srcY;
		mSrcIndexY = srcindexY;
		mpSrcCb = srcCb;
		mSrcIndexCb = srcindexCb;
		mpSrcCr = srcCr;
		mSrcIndexCr = srcindexCr;
		mWidth = srcY->GetWidth(srcindexY);
		mHeight = srcY->GetHeight(srcindexY);

		srcY->AddWindowRequest(0, 0);
		srcCb->AddWindowRequest(0, 0);
		srcCr->AddWindowRequest(0, 0);
	}

	void Start() override {
		mpSrcY->Start();
		mpSrcCb->Start();
		mpSrcCr->Start();

		StartWindow(mWidth * 8);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo unused;
		mpSrcY->TransformPixmapInfo(src,dst);
		mpSrcCb->TransformPixmapInfo(src,unused);
		mpSrcCr->TransformPixmapInfo(src,unused);
		dst.alpha_type = 0;
	}

	uint32 GetType(uint32 output) const override {
		return (mpSrcY->GetType(mSrcIndexY) & ~kVDPixType_Mask) | kVDPixType_16x4_LE;
	}

	IVDPixmapGen* dump_src(int index) override {
		if(index==0) return mpSrcCr;
		if(index==1) return mpSrcY;
		if(index==2) return mpSrcCb;
		return 0; 
	}

	const char* dump_name() override { return "B16x3_To_Y416"; }

protected:
	void Compute(void *dst0, sint32 y) override {
		uint16 *dst = (uint16 *)dst0;
		const uint16 *srcY = (const uint16 *)mpSrcY->GetRow(y, mSrcIndexY);
		const uint16 *srcCb = (const uint16 *)mpSrcCb->GetRow(y, mSrcIndexCb);
		const uint16 *srcCr = (const uint16 *)mpSrcCr->GetRow(y, mSrcIndexCr);

		for(sint32 x=0; x<mWidth; ++x) {
			uint16 y = *srcY++;
			uint16 cb = *srcCb++;
			uint16 cr = *srcCr++;

			dst[0] = cb;
			dst[1] = y;
			dst[2] = cr;
			dst[3] = 0;
			dst += 4;
		}
	}

	IVDPixmapGen *mpSrcY;
	uint32 mSrcIndexY;
	IVDPixmapGen *mpSrcCb;
	uint32 mSrcIndexCb;
	IVDPixmapGen *mpSrcCr;
	uint32 mSrcIndexCr;
};

class VDPixmapGen_B16x3_To_RGB64 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *srcR, uint32 srcindexR, IVDPixmapGen *srcG, uint32 srcindexG, IVDPixmapGen *srcB, uint32 srcindexB) {
		mpSrcR = srcR;
		mSrcIndexR = srcindexR;
		mpSrcG = srcG;
		mSrcIndexG = srcindexG;
		mpSrcB = srcB;
		mSrcIndexB = srcindexB;
		mWidth = srcR->GetWidth(srcindexR);
		mHeight = srcR->GetHeight(srcindexR);

		srcR->AddWindowRequest(0, 0);
		srcG->AddWindowRequest(0, 0);
		srcB->AddWindowRequest(0, 0);
	}

	void Start() override {
		mpSrcR->Start();
		mpSrcG->Start();
		mpSrcB->Start();

		StartWindow(mWidth * 8);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo buf;
		mpSrcR->TransformPixmapInfo(src,dst);
		mpSrcG->TransformPixmapInfo(src,buf); dst.ref_g = buf.ref_r;
		mpSrcB->TransformPixmapInfo(src,buf); dst.ref_b = buf.ref_r;
		dst.alpha_type = 0;
	}

	uint32 GetType(uint32 output) const override {
		return (mpSrcR->GetType(mSrcIndexR) & ~kVDPixType_Mask) | kVDPixType_16x4_LE;
	}

	IVDPixmapGen* dump_src(int index) override {
		if(index==0) return mpSrcR;
		if(index==1) return mpSrcG;
		if(index==2) return mpSrcB;
		return 0; 
	}

	const char* dump_name() override { return "B16x3_To_RGB64"; }

protected:
	void Compute(void *dst0, sint32 y) override {
		uint16 *dst = (uint16 *)dst0;
		const uint16 *srcR = (const uint16 *)mpSrcR->GetRow(y, mSrcIndexR);
		const uint16 *srcG = (const uint16 *)mpSrcG->GetRow(y, mSrcIndexG);
		const uint16 *srcB = (const uint16 *)mpSrcB->GetRow(y, mSrcIndexB);

		for(sint32 x=0; x<mWidth; ++x) {
			uint16 r = *srcR++;
			uint16 g = *srcG++;
			uint16 b = *srcB++;

			dst[0] = b;
			dst[1] = g;
			dst[2] = r;
			dst[3] = 0;
			dst += 4;
		}
	}

	IVDPixmapGen *mpSrcR;
	uint32 mSrcIndexR;
	IVDPixmapGen *mpSrcG;
	uint32 mSrcIndexG;
	IVDPixmapGen *mpSrcB;
	uint32 mSrcIndexB;
};

class VDPixmapGen_B16x4_To_RGB64 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *srcR, uint32 srcindexR, IVDPixmapGen *srcG, uint32 srcindexG, IVDPixmapGen *srcB, uint32 srcindexB, IVDPixmapGen *srcA, uint32 srcindexA) {
		mpSrcR = srcR;
		mSrcIndexR = srcindexR;
		mpSrcG = srcG;
		mSrcIndexG = srcindexG;
		mpSrcB = srcB;
		mSrcIndexB = srcindexB;
		mpSrcA = srcA;
		mSrcIndexA = srcindexA;
		mWidth = srcR->GetWidth(srcindexR);
		mHeight = srcR->GetHeight(srcindexR);

		srcR->AddWindowRequest(0, 0);
		srcG->AddWindowRequest(0, 0);
		srcB->AddWindowRequest(0, 0);
		srcA->AddWindowRequest(0, 0);
	}

	void Start() override {
		mpSrcR->Start();
		mpSrcG->Start();
		mpSrcB->Start();
		mpSrcA->Start();

		StartWindow(mWidth * 8);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo buf;
		mpSrcR->TransformPixmapInfo(src,dst);
		mpSrcG->TransformPixmapInfo(src,buf); dst.ref_g = buf.ref_r;
		mpSrcB->TransformPixmapInfo(src,buf); dst.ref_b = buf.ref_r;
		mpSrcA->TransformPixmapInfo(src,buf); dst.ref_a = buf.ref_a;
		dst.copy_alpha(buf);
	}

	uint32 GetType(uint32 output) const override {
		return (mpSrcR->GetType(mSrcIndexR) & ~kVDPixType_Mask) | kVDPixType_16x4_LE;
	}

	IVDPixmapGen* dump_src(int index) override {
		if(index==0) return mpSrcR;
		if(index==1) return mpSrcG;
		if(index==2) return mpSrcB;
		if(index==3) return mpSrcA;
		return 0; 
	}

	const char* dump_name() override { return "B16x4_To_RGB64"; }

protected:
	void Compute(void *dst0, sint32 y) override {
		uint16 *dst = (uint16 *)dst0;
		const uint16 *srcR = (const uint16 *)mpSrcR->GetRow(y, mSrcIndexR);
		const uint16 *srcG = (const uint16 *)mpSrcG->GetRow(y, mSrcIndexG);
		const uint16 *srcB = (const uint16 *)mpSrcB->GetRow(y, mSrcIndexB);
		const uint16 *srcA = (const uint16 *)mpSrcA->GetRow(y, mSrcIndexA);

		for(sint32 x=0; x<mWidth; ++x) {
			uint16 r = *srcR++;
			uint16 g = *srcG++;
			uint16 b = *srcB++;
			uint16 a = *srcA++;

			dst[0] = b;
			dst[1] = g;
			dst[2] = r;
			dst[3] = a;
			dst += 4;
		}
	}

	IVDPixmapGen *mpSrcR;
	uint32 mSrcIndexR;
	IVDPixmapGen *mpSrcG;
	uint32 mSrcIndexG;
	IVDPixmapGen *mpSrcB;
	uint32 mSrcIndexB;
	IVDPixmapGen *mpSrcA;
	uint32 mSrcIndexA;
};

class VDPixmapGen_X8R8G8B8_To_A8R8G8B8 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *src, uint32 srcindex, IVDPixmapGen *srcA, uint32 srcindexA) {
		mpSrc = src;
		mSrcIndex = srcindex;
		mpSrcA = srcA;
		mSrcIndexA = srcindexA;
		mWidth = src->GetWidth(srcindex);
		mHeight = src->GetHeight(srcindex);

		src->AddWindowRequest(0, 0);
		srcA->AddWindowRequest(0, 0);
	}

	void Start() override {
		mpSrc->Start();
		mpSrcA->Start();

		StartWindow(mWidth * 4);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo buf;
		mpSrc->TransformPixmapInfo(src,dst);
		mpSrcA->TransformPixmapInfo(src,buf);
		dst.copy_alpha(src);
	}

	uint32 GetType(uint32 output) const override {
		return mpSrc->GetType(mSrcIndex);
	}

	IVDPixmapGen* dump_src(int index) override {
		if(index==0) return mpSrc;
		if(index==1) return mpSrcA;
		return 0; 
	}

	const char* dump_name() override { return "X8R8G8B8_To_A8R8G8B8"; }

protected:
	void Compute(void *dst0, sint32 y) override {
		uint8 *dst = (uint8 *)dst0;
		const uint8 *src = (const uint8 *)mpSrc->GetRow(y, mSrcIndex);
		const uint8 *srcA = (const uint8 *)mpSrcA->GetRow(y, mSrcIndexA);

		for(sint32 x=0; x<mWidth; ++x) {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst[3] = srcA[0];
			src += 4;
			srcA++;
			dst += 4;
		}
	}

	IVDPixmapGen *mpSrc;
	uint32 mSrcIndex;
	IVDPixmapGen *mpSrcA;
	uint32 mSrcIndexA;
};

class VDPixmapGen_X16R16G16B16_To_A16R16G16B16 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *src, uint32 srcindex, IVDPixmapGen *srcA, uint32 srcindexA) {
		mpSrc = src;
		mSrcIndex = srcindex;
		mpSrcA = srcA;
		mSrcIndexA = srcindexA;
		mWidth = src->GetWidth(srcindex);
		mHeight = src->GetHeight(srcindex);

		src->AddWindowRequest(0, 0);
		srcA->AddWindowRequest(0, 0);
	}

	void Start() override {
		mpSrc->Start();
		mpSrcA->Start();

		StartWindow(mWidth * 8);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo buf;
		mpSrc->TransformPixmapInfo(src,dst);
		mpSrcA->TransformPixmapInfo(src,buf);
		dst.copy_alpha(src);
		dst.ref_a = buf.ref_a;
	}

	uint32 GetType(uint32 output) const override {
		return mpSrc->GetType(mSrcIndex);
	}

	IVDPixmapGen* dump_src(int index) override {
		if(index==0) return mpSrc;
		if(index==1) return mpSrcA;
		return 0; 
	}

	const char* dump_name() override { return "X16R16G16B16_To_A16R16G16B16"; }

protected:
	void Compute(void *dst0, sint32 y) override {
		uint16 *dst = (uint16 *)dst0;
		const uint16 *src = (const uint16 *)mpSrc->GetRow(y, mSrcIndex);
		const uint16 *srcA = (const uint16 *)mpSrcA->GetRow(y, mSrcIndexA);

		for(sint32 x=0; x<mWidth; ++x) {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst[3] = srcA[0];
			src += 4;
			srcA++;
			dst += 4;
		}
	}

	IVDPixmapGen *mpSrc;
	uint32 mSrcIndex;
	IVDPixmapGen *mpSrcA;
	uint32 mSrcIndexA;
};

class VDPixmapGen_B16x2_To_B16R16 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *srcCb, uint32 srcindexCb, IVDPixmapGen *srcCr, uint32 srcindexCr) {
		mpSrcCb = srcCb;
		mSrcIndexCb = srcindexCb;
		mpSrcCr = srcCr;
		mSrcIndexCr = srcindexCr;
		mWidth = srcCb->GetWidth(srcindexCb);
		mHeight = srcCb->GetHeight(srcindexCb);

		srcCb->AddWindowRequest(0, 0);
		srcCr->AddWindowRequest(0, 0);
	}

	void Start() override {
		mpSrcCb->Start();
		mpSrcCr->Start();

		StartWindow(mWidth * 4);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo unused;
		mpSrcCb->TransformPixmapInfo(src,dst);
		mpSrcCr->TransformPixmapInfo(src,unused);
	}

	uint32 GetType(uint32 output) const override {
		return (mpSrcCb->GetType(mSrcIndexCb) & ~kVDPixType_Mask) | kVDPixType_16x2_LE;
	}

	IVDPixmapGen* dump_src(int index) override {
		if(index==0) return mpSrcCb;
		if(index==1) return mpSrcCr;
		return 0; 
	}

	const char* dump_name() override { return "B16x2_To_B16R16"; }

protected:
	void Compute(void *dst0, sint32 y) override {
		uint16 *dst = (uint16 *)dst0;
		const uint16 *srcCb = (const uint16 *)mpSrcCb->GetRow(y, mSrcIndexCb);
		const uint16 *srcCr = (const uint16 *)mpSrcCr->GetRow(y, mSrcIndexCr);

		for(sint32 x=0; x<mWidth; ++x) {
			uint16 cb = *srcCb++;
			uint16 cr = *srcCr++;

			dst[0] = cb;
			dst[1] = cr;
			dst += 2;
		}
	}

	IVDPixmapGen *mpSrcCb;
	uint32 mSrcIndexCb;
	IVDPixmapGen *mpSrcCr;
	uint32 mSrcIndexCr;
};

#endif
