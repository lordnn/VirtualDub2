#ifndef f_VD2_KASUMI_UBERBLIT_FILL_H
#define f_VD2_KASUMI_UBERBLIT_FILL_H

#include "uberblit.h"
#include "uberblit_base.h"

class VDPixmapGenFill8 : public IVDPixmapGen {
public:
	void Init(uint8 fill, uint32 bpr, sint32 width, sint32 height, uint32 type) {
		mRow.resize(bpr, fill);
		mWidth = width;
		mHeight = height;
		mType = type;
	}

	void AddWindowRequest(int minY, int maxY)  override {
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst)  override {
	}

	void Start()  override {
	}

	sint32 GetWidth(int) const  override {
		return mWidth;
	}

	sint32 GetHeight(int) const  override {
		return mHeight;
	}

	bool IsStateful() const  override {
		return false;
	}

	const void *GetRow(sint32 y, uint32 output)  override {
		return mRow.data();
	}

	void ProcessRow(void *dst, sint32 y)  override {
		if (!mRow.empty())
			memset(dst, mRow[0], mRow.size());
	}

	uint32 GetType(uint32 index) const  override {
		return mType;
	}

	IVDPixmapGen* dump_src(int index) override { return 0; }
	const char* dump_name() override { return "Fill8"; }

protected:
	sint32		mWidth;
	sint32		mHeight;
	uint32		mType;

	vdfastvector<uint8> mRow;
};

class VDPixmapGenFillF : public IVDPixmapGen {
public:
	void Init(float fill, uint32 bpr, sint32 width, sint32 height, uint32 type) {
		mRow.resize(bpr, fill);
		mWidth = width;
		mHeight = height;
		mType = type;
	}

	void AddWindowRequest(int minY, int maxY)  override {
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
	}

	void Start() override {
	}

	sint32 GetWidth(int) const override {
		return mWidth;
	}

	sint32 GetHeight(int) const override {
		return mHeight;
	}

	bool IsStateful() const override {
		return false;
	}

	const void *GetRow(sint32 y, uint32 output) override {
		return mRow.data();
	}

	void ProcessRow(void *dst, sint32 y) override {
		if (!mRow.empty()) {
			for(int i=0; i<(int)mRow.size(); i++) ((float*)dst)[i] = mRow[0];
		}
	}

	uint32 GetType(uint32 index) const override {
		return mType;
	}

	IVDPixmapGen* dump_src(int index) override { return 0; }
	const char* dump_name() override { return "FillF"; }

protected:
	sint32		mWidth;
	sint32		mHeight;
	uint32		mType;

	vdfastvector<float> mRow;
};

class VDPixmapGenFill16c : public IVDPixmapGen {
public:
	void Init(uint32 bpr, sint32 width, sint32 height, uint32 type) {
		mRow.resize(bpr, 0);
		mWidth = width;
		mHeight = height;
		mType = type;
	}

	void AddWindowRequest(int minY, int maxY) override {
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		dst.copy_dynamic(src);
		uint16 c = vd2::chroma_neutral(src.ref_r);
		std::fill(mRow.begin(), mRow.end(), c);
	}

	void Start() override {
	}

	sint32 GetWidth(int) const override {
		return mWidth;
	}

	sint32 GetHeight(int) const override {
		return mHeight;
	}

	bool IsStateful() const override {
		return false;
	}

	const void *GetRow(sint32 y, uint32 output) override {
		return mRow.data();
	}

	void ProcessRow(void *dst, sint32 y) override {
		if (!mRow.empty()) {
			for(int i=0; i<(int)mRow.size(); i++) ((uint16*)dst)[i] = mRow[0];
		}
	}

	uint32 GetType(uint32 index) const override {
		return mType;
	}

	IVDPixmapGen* dump_src(int index) override { return 0; }
	const char* dump_name() override { return "Fill16c"; }

protected:
	sint32		mWidth;
	sint32		mHeight;
	uint32		mType;

	vdfastvector<uint16> mRow;
};

class VDPixmapGen_dup_r16 : public VDPixmapGenWindowBasedOneSourceSimple {
public:

	void Start() override {
		StartWindow(mWidth * 2);
	}

	void Compute(void *dst0, sint32 y) override {}

	const void *GetRow(sint32 y, uint32 output) override {
		return mpSrc->GetRow(y, output);
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		mpSrc->TransformPixmapInfo(src,dst);
		dst.ref_g = dst.ref_r;
		dst.ref_b = dst.ref_r;
	}

	uint32 GetType(uint32 output) const override {
		return mpSrc->GetType(mSrcIndex);
	}

	const char* dump_name(){ return "dup_r16"; }
};

#endif
