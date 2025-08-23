#ifndef f_VD2_KASUMI_UBERBLIT_V210_H
#define f_VD2_KASUMI_UBERBLIT_V210_H

#include <vd2/system/cpuaccel.h>
#include "uberblit_base.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	32F -> V210
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_32F_To_V210 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *srcR, uint32 srcindexR, IVDPixmapGen *srcG, uint32 srcindexG, IVDPixmapGen *srcB, uint32 srcindexB) {
		mpSrcR = srcR;
		mSrcIndexR = srcindexR;
		mpSrcG = srcG;
		mSrcIndexG = srcindexG;
		mpSrcB = srcB;
		mSrcIndexB = srcindexB;
		mWidth = srcG->GetWidth(srcindexG);
		mHeight = srcG->GetHeight(srcindexG);

		srcR->AddWindowRequest(0, 0);
		srcG->AddWindowRequest(0, 0);
		srcB->AddWindowRequest(0, 0);
	}

	void Start() override {
		mpSrcR->Start();
		mpSrcG->Start();
		mpSrcB->Start();

		int qw = (mWidth + 47) / 48;
		StartWindow(qw * 128);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo unused;
		mpSrcR->TransformPixmapInfo(src,unused);
		mpSrcG->TransformPixmapInfo(src,unused);
		mpSrcB->TransformPixmapInfo(src,unused);
		if (dst.colorRangeMode==vd2::kColorRangeMode_Full)
			max_value = 0x3FF;
		else
			max_value = 0x3FC;
		bias = -128.0f / 255.0f + 512.0f / max_value;
	}

	uint32 GetType(uint32 output) const override;

	IVDPixmapGen* dump_src(int index) override {
		if(index==0) return mpSrcR;
		if(index==1) return mpSrcG;
		if(index==2) return mpSrcB;
		return 0; 
	}

	const char* dump_name() override { return "32F_To_V210"; }

protected:
	int max_value;
	float bias;

	void Compute(void *dst0, sint32 y) override;

	IVDPixmapGen *mpSrcR;
	uint32 mSrcIndexR;
	IVDPixmapGen *mpSrcG;
	uint32 mSrcIndexG;
	IVDPixmapGen *mpSrcB;
	uint32 mSrcIndexB;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	V210 -> 32F
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_V210_To_32F : public VDPixmapGenWindowBasedOneSourceSimple {
public:
	void Start() override;
	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		VDPixmapGenWindowBasedOneSourceSimple::TransformPixmapInfo(src,dst);
		if (src.colorRangeMode==vd2::kColorRangeMode_Full)
			max_value = 0x3FF;
		else
			max_value = 0x3FC;
		bias = 128.0f / 255.0f - 512.0f / max_value;
	}

	const void *GetRow(sint32 y, uint32 index) override;

	sint32 GetWidth(int index) const override;
	uint32 GetType(uint32 output) const override;

	const char* dump_name() override { return "V210_To_32F"; }

protected:
	int max_value;
	float bias;

	void Compute(void *dst0, sint32 y) override;
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	V210 -> P16
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_V210_To_P16 : public VDPixmapGenWindowBasedOneSourceSimple {
public:
	void Start() override;
	const void *GetRow(sint32 y, uint32 index) override;

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo unused;
		mpSrc->TransformPixmapInfo(src,unused);
		dst.colorSpaceMode = src.colorSpaceMode;
		dst.colorRangeMode = src.colorRangeMode;
		if (src.colorRangeMode==vd2::kColorRangeMode_Full)
			dst.ref_r = 0x3FF;
		else
			dst.ref_r = 0x3FC;
		dst.ref_g = 0;
		dst.ref_b = 0;
		dst.ref_a = 0;
	}

	sint32 GetWidth(int index) const override;
	uint32 GetType(uint32 output) const override;

	const char* dump_name() override { return "V210_To_P16"; }

protected:
	void Compute(void *dst0, sint32 y) override;
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	YU64 -> P16
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_YU64_To_P16 : public VDPixmapGenWindowBasedOneSourceSimple {
public:
	void Start() override;
	const void *GetRow(sint32 y, uint32 index) override;

	sint32 GetWidth(int index) const override;
	uint32 GetType(uint32 output) const override;

	const char* dump_name() override { return "YU64_To_P16"; }

protected:
	void Compute(void *dst0, sint32 y) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	P16 -> YU64
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_P16_To_YU64 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *srcR, uint32 srcindexR, IVDPixmapGen *srcG, uint32 srcindexG, IVDPixmapGen *srcB, uint32 srcindexB) {
		mpSrcR = srcR;
		mSrcIndexR = srcindexR;
		mpSrcG = srcG;
		mSrcIndexG = srcindexG;
		mpSrcB = srcB;
		mSrcIndexB = srcindexB;
		mWidth = srcG->GetWidth(srcindexG);
		mHeight = srcG->GetHeight(srcindexG);

		srcR->AddWindowRequest(0, 0);
		srcG->AddWindowRequest(0, 0);
		srcB->AddWindowRequest(0, 0);
	}

	void Start() override {
		mpSrcR->Start();
		mpSrcG->Start();
		mpSrcB->Start();

		StartWindow(mWidth * 4);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo unused;
		mpSrcR->TransformPixmapInfo(src,unused);
		mpSrcG->TransformPixmapInfo(src,dst);
		mpSrcB->TransformPixmapInfo(src,unused);
	}

	uint32 GetType(uint32 output) const override;

	IVDPixmapGen* dump_src(int index) override {
		if(index==0) return mpSrcR;
		if(index==1) return mpSrcG;
		if(index==2) return mpSrcB;
		return 0; 
	}

	const char* dump_name() override { return "P16_To_YU64"; }

protected:
	void Compute(void *dst0, sint32 y) override;

	IVDPixmapGen *mpSrcR;
	uint32 mSrcIndexR;
	IVDPixmapGen *mpSrcG;
	uint32 mSrcIndexG;
	IVDPixmapGen *mpSrcB;
	uint32 mSrcIndexB;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	32F -> V410
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_32F_To_V410 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *srcR, uint32 srcindexR, IVDPixmapGen *srcG, uint32 srcindexG, IVDPixmapGen *srcB, uint32 srcindexB) {
		mpSrcR = srcR;
		mSrcIndexR = srcindexR;
		mpSrcG = srcG;
		mSrcIndexG = srcindexG;
		mpSrcB = srcB;
		mSrcIndexB = srcindexB;
		mWidth = srcG->GetWidth(srcindexG);
		mHeight = srcG->GetHeight(srcindexG);

		srcR->AddWindowRequest(0, 0);
		srcG->AddWindowRequest(0, 0);
		srcB->AddWindowRequest(0, 0);
	}

	void Start() override {
		mpSrcR->Start();
		mpSrcG->Start();
		mpSrcB->Start();

		StartWindow(mWidth * 4);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo unused;
		mpSrcR->TransformPixmapInfo(src,unused);
		mpSrcG->TransformPixmapInfo(src,unused);
		mpSrcB->TransformPixmapInfo(src,unused);
		if (dst.colorRangeMode==vd2::kColorRangeMode_Full)
			max_value = 0x3FF;
		else
			max_value = 0x3FC;
		bias = -128.0f / 255.0f + 512.0f / max_value;
	}

	uint32 GetType(uint32 output) const override;

	IVDPixmapGen* dump_src(int index) override {
		if(index==0) return mpSrcR;
		if(index==1) return mpSrcG;
		if(index==2) return mpSrcB;
		return 0; 
	}

	const char* dump_name() override { return "32F_To_V410"; }

protected:
	int max_value;
	float bias;

	void Compute(void *dst0, sint32 y) override;

	IVDPixmapGen *mpSrcR;
	uint32 mSrcIndexR;
	IVDPixmapGen *mpSrcG;
	uint32 mSrcIndexG;
	IVDPixmapGen *mpSrcB;
	uint32 mSrcIndexB;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	V410 -> 32F
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_V410_To_32F : public VDPixmapGenWindowBasedOneSourceSimple {
public:
	void Start() override;
	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		VDPixmapGenWindowBasedOneSourceSimple::TransformPixmapInfo(src,dst);
		if (src.colorRangeMode==vd2::kColorRangeMode_Full)
			max_value = 0x3FF;
		else
			max_value = 0x3FC;
		bias = 128.0f / 255.0f - 512.0f / max_value;
	}

	const void *GetRow(sint32 y, uint32 index) override;

	uint32 GetType(uint32 output) const override;

	const char* dump_name() override { return "V410_To_32F"; }

protected:
	int max_value;
	float bias;

	void Compute(void *dst0, sint32 y) override;
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	V410 -> P16
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_V410_To_P16 : public VDPixmapGenWindowBasedOneSourceSimple {
public:
	void Start() override;
	const void *GetRow(sint32 y, uint32 index) override;

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo unused;
		mpSrc->TransformPixmapInfo(src,unused);
		dst.colorSpaceMode = src.colorSpaceMode;
		dst.colorRangeMode = src.colorRangeMode;
		if (src.colorRangeMode==vd2::kColorRangeMode_Full)
			dst.ref_r = 0x3FF;
		else
			dst.ref_r = 0x3FC;
		dst.ref_g = 0;
		dst.ref_b = 0;
		dst.ref_a = 0;
	}

	uint32 GetType(uint32 output) const override;

	const char* dump_name() override { return "V410_To_P16"; }

protected:
	void Compute(void *dst0, sint32 y) override;
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	32F -> Y410
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_32F_To_Y410 : public VDPixmapGen_32F_To_V410 {
public:
	uint32 GetType(uint32 output) const override;
	const char* dump_name() override { return "32F_To_Y410"; }

protected:
	void Compute(void *dst0, sint32 y) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Y410 -> 32F
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_Y410_To_32F : public VDPixmapGen_V410_To_32F {
public:
	const char* dump_name() override { return "Y410_To_32F"; }
protected:
	void Compute(void *dst0, sint32 y) override;
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Y410 -> P16
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_Y410_To_P16 : public VDPixmapGen_V410_To_P16 {
public:
	const char* dump_name() override { return "Y410_To_P16"; }
protected:
	void Compute(void *dst0, sint32 y) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	V308 -> P8
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_V308_To_P8 : public VDPixmapGenWindowBasedOneSourceSimple {
public:
	void Start() override;
	const void *GetRow(sint32 y, uint32 index) override;

	uint32 GetType(uint32 output) const override;

	const char* dump_name() override { return "V308_To_P8"; }

protected:
	void Compute(void *dst0, sint32 y) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	P8 -> V308
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_P8_To_V308 : public VDPixmapGenWindowBased {
public:
	void Init(IVDPixmapGen *srcR, uint32 srcindexR, IVDPixmapGen *srcG, uint32 srcindexG, IVDPixmapGen *srcB, uint32 srcindexB) {
		mpSrcR = srcR;
		mSrcIndexR = srcindexR;
		mpSrcG = srcG;
		mSrcIndexG = srcindexG;
		mpSrcB = srcB;
		mSrcIndexB = srcindexB;
		mWidth = srcG->GetWidth(srcindexG);
		mHeight = srcG->GetHeight(srcindexG);

		srcR->AddWindowRequest(0, 0);
		srcG->AddWindowRequest(0, 0);
		srcB->AddWindowRequest(0, 0);
	}

	void Start() override {
		mpSrcR->Start();
		mpSrcG->Start();
		mpSrcB->Start();

		StartWindow(mWidth * 3);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo unused;
		mpSrcR->TransformPixmapInfo(src,unused);
		mpSrcG->TransformPixmapInfo(src,unused);
		mpSrcB->TransformPixmapInfo(src,unused);
	}

	uint32 GetType(uint32 output) const override;

	IVDPixmapGen* dump_src(int index) override {
		if(index==0) return mpSrcR;
		if(index==1) return mpSrcG;
		if(index==2) return mpSrcB;
		return 0; 
	}

	const char* dump_name() override { return "P8_To_V308"; }

protected:
	void Compute(void *dst0, sint32 y) override;

	IVDPixmapGen *mpSrcR;
	uint32 mSrcIndexR;
	IVDPixmapGen *mpSrcG;
	uint32 mSrcIndexG;
	IVDPixmapGen *mpSrcB;
	uint32 mSrcIndexB;
};

#endif	// f_VD2_KASUMI_UBERBLIT_V210_H
