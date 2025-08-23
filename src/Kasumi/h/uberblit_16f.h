#ifndef f_VD2_KASUMI_UBERBLIT_16F_H
#define f_VD2_KASUMI_UBERBLIT_16F_H

#include <vd2/system/cpuaccel.h>
#include "uberblit_base.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	32F -> 16F
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_32F_To_16F : public VDPixmapGenWindowBasedOneSourceSimple {
public:
	void Start() override;

	uint32 GetType(uint32 output) const override;

	const char* dump_name() override { return "32F_To_16F"; }

protected:
	void Compute(void *dst0, sint32 y) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	16F -> 32F
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_16F_To_32F : public VDPixmapGenWindowBasedOneSourceSimple {
public:
	void Start() override;

	uint32 GetType(uint32 output) const override;

	const char* dump_name() override { return "16F_To_32F"; }

protected:
	void Compute(void *dst0, sint32 y) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	32F -> 16
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_32F_To_16 : public VDPixmapGenWindowBasedOneSourceSimple {
public:

	VDPixmapGen_32F_To_16(bool chroma){ isChroma = chroma; }

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo unused;
		mpSrc->TransformPixmapInfo(src,unused);
		if (dst.colorRangeMode==vd2::kColorRangeMode_Full)
			dst.ref_r = 0xFFFF;
		else
			dst.ref_r = 0xFF00;
		m = float(dst.ref_r);
		bias = 0;
		if (isChroma) bias = 0x8000 - 128.0f / 255.0f * m;
	}

	void Start() override;

	uint32 GetType(uint32 output) const override;

	const char* dump_name() override { return "32F_To_16"; }

protected:
	bool isChroma;
	float m;
	float bias;

	void Compute(void *dst0, sint32 y) override;
};

class VDPixmapGen_32F_To_r16 : public VDPixmapGen_32F_To_16 {
public:

  VDPixmapGen_32F_To_r16():VDPixmapGen_32F_To_16(false){}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo buf;
		mpSrc->TransformPixmapInfo(src,buf);
		dst.ref_r = 0xFFFF;
		dst.ref_g = 0xFFFF;
		dst.ref_b = 0xFFFF;
		dst.ref_a = 0xFFFF;
		dst.copy_alpha(buf);
		m = float(dst.ref_r);
		bias = 0;
	}

	const char* dump_name() override { return "32F_To_r16"; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	16 -> 32F
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_16_To_32F : public VDPixmapGenWindowBasedOneSourceSimple {
public:

	VDPixmapGen_16_To_32F(bool chroma){ isChroma = chroma; }

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo buf;
		mpSrc->TransformPixmapInfo(src,buf);
		ref = buf.ref_r;
		m = float(1.0/buf.ref_r);
		bias = 0;
		int mref = vd2::chroma_neutral(ref);
		if (isChroma) bias = -mref*m + 128.0f / 255.0f;
		invalid = false;
	}

	void Start() override;

	uint32 GetType(uint32 output) const override;

	const char* dump_name() override { return "16_To_32F"; }

protected:
	bool isChroma;
	bool invalid;
	int ref;
	float m;
	float bias;

	void Compute(void *dst0, sint32 y) override;
};

class VDPixmapGen_A16_To_A32F : public VDPixmapGen_16_To_32F {
public:
  VDPixmapGen_A16_To_A32F():VDPixmapGen_16_To_32F(false){}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo buf;
		mpSrc->TransformPixmapInfo(src,buf);
		ref = buf.ref_a;
		m = float(1.0/buf.ref_a);
		bias = 0;
		dst.copy_alpha(buf);
		invalid = !dst.alpha_type;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//	16 -> 8
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_8_To_16 : public VDPixmapGenWindowBasedOneSourceAlign8to16 {
public:

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo buf;
		mpSrc->TransformPixmapInfo(src,buf);
		dst.copy_frame(buf);
		dst.ref_r = 0xFF00;
		invalid = false;
	}

	uint32 GetType(uint32 output) const override {
		return (mpSrc->GetType(mSrcIndex) & ~kVDPixType_Mask) | kVDPixType_16_LE;
	}

	const char* dump_name() override { return "8_To_16"; }

protected:
	bool invalid;

	int ComputeSpan(uint16* dst, const uint8* src, int n) override;
};

class VDPixmapGen_A8_To_A16 : public VDPixmapGen_8_To_16 {
public:
	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo buf;
		mpSrc->TransformPixmapInfo(src,buf);
		dst.copy_alpha(buf);
		dst.ref_a = 0xFFFF;
		invalid = !dst.alpha_type;
	}

	int ComputeSpan(uint16* dst, const uint8* src, int n) override;
};

class VDPixmapGen_R8_To_R16 : public VDPixmapGen_A8_To_A16 {
public:
	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		FilterModPixmapInfo buf;
		mpSrc->TransformPixmapInfo(src,buf);
		dst.copy_frame(buf);
		dst.ref_r = 0xFFFF;
		dst.ref_g = 0xFFFF;
		dst.ref_b = 0xFFFF;
		invalid = false;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_16_To_8 : public VDPixmapGenWindowBasedOneSourceAlign16to8 {
public:
	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override;
	void Start() override;

	uint32 GetType(uint32 output) const override;

	const char* dump_name() override { return "16_To_8"; }

protected:
	int ref;
	uint32 m;
	bool invalid;

	int ComputeSpan(uint8* dst, const uint16* src, int n) override;
};

class VDPixmapGen_A16_To_A8 : public VDPixmapGen_16_To_8 {
public:
	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class VDPixmapGen_Y16_Normalize : public VDPixmapGenWindowBasedOneSourceAlign16 {
public:

	VDPixmapGen_Y16_Normalize(bool chroma=false){ max_value = 0xFFFF; round = 1; isChroma = chroma; }

	uint32 max_value;
	uint16 round;

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override;

	const char* dump_name() override { return "Y16_Normalize"; }

protected:
	int ref;
	uint32 m;
	uint16 s;
	int bias;
	bool isChroma;
	bool do_normalize;

	int ComputeSpan(uint16* dst, const uint16* src, int n) override;
	void ComputeNormalize(uint16* dst, const uint16* src, int n);
	void ComputeNormalizeBias(uint16* dst, const uint16* src, int n);
	void ComputeMask(uint16* dst, const uint16* src, int n);
};

class VDPixmapGen_A16_Normalize : public VDPixmapGen_Y16_Normalize {
public:

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override;

	const char* dump_name() override { return "A16_Normalize"; }

protected:
	uint32 a_mask;

	int ComputeSpan(uint16* dst, const uint16* src, int n) override;
	void ComputeWipeAlpha(uint16* dst, int n);
};

class VDPixmapGen_A8_Normalize : public VDPixmapGenWindowBasedOneSourceSimple {
public:

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		mpSrc->TransformPixmapInfo(src,dst);

		a_mask = 0;
		if(dst.alpha_type==FilterModPixmapInfo::kAlphaInvalid)
			a_mask = 255;
	}

	void Start() override {
		StartWindow(mWidth);
	}

	const char* dump_name() override { return "A8_Normalize"; }

protected:
	uint32 a_mask;

	void Compute(void *dst0, sint32 y) override;
	void ComputeWipeAlpha(void *dst0, sint32 y);
};

class ExtraGen_YUV_Normalize : public IVDPixmapExtraGen {
public:
	uint32 max_value;
	uint32 max_a_value;
	uint16 round;

	ExtraGen_YUV_Normalize(){ max_value=0xFFFF; max_a_value=0xFFFF; round=1; }
	void Create(VDPixmapUberBlitterGenerator& gen, const VDPixmapLayout& dst) override;
};

class ExtraGen_RGB_Normalize : public IVDPixmapExtraGen {
public:
	uint32 max_value;
	uint32 max_a_value;

	ExtraGen_RGB_Normalize(){ max_value=0xFFFF; max_a_value=0xFFFF; }
	void Create(VDPixmapUberBlitterGenerator& gen, const VDPixmapLayout& dst) override;
};

class ExtraGen_A8_Normalize : public IVDPixmapExtraGen {
public:
	void Create(VDPixmapUberBlitterGenerator& gen, const VDPixmapLayout& dst) override;
};

/*
bool inline VDPixmap_YUV_IsNormalized(const FilterModPixmapInfo& info, uint32 max_value=0xFFFF) {
	if (info.ref_r!=max_value)
		return false;
	return true;
}

void VDPixmap_YUV_Normalize(VDPixmap& dst, const VDPixmap& src, uint32 max_value=0xFFFF);
*/

#endif
