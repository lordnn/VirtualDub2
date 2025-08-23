#ifndef f_VD2_KASUMI_UBERBLIT_INPUT_H
#define f_VD2_KASUMI_UBERBLIT_INPUT_H

#include "uberblit.h"
#include "uberblit_base.h"

class IVDPixmapGenSrc {
public:
	virtual void SetSource(const void *src, ptrdiff_t pitch, const uint32 *palette) = 0;
	virtual void SetPlane(int i) = 0;
};

class VDPixmapGenSrc : public IVDPixmapGen, public IVDPixmapGenSrc {
public:
	void Init(sint32 width, sint32 height, uint32 type, uint32 bpr) {
		mWidth = width;
		mHeight = height;
		mType = type;
		mBpr = bpr;
		mPlane = -1;
	}

	void SetSource(const void *src, ptrdiff_t pitch, const uint32 *palette) override {
		mpSrc = src;
		mPitch = pitch;
	}

	void SetPlane(int i) override {
		mPlane = i;
	}

	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		dst.copy_dynamic(src);
	}

	void AddWindowRequest(int minY, int maxY) override {
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
		if (y < 0)
			y = 0;
		else if (y >= mHeight)
			y = mHeight - 1;
		return vdptroffset(mpSrc, mPitch*y);
	}

	void ProcessRow(void *dst, sint32 y) override {
		memcpy(dst, GetRow(y, 0), mBpr);
	}

	uint32 GetType(uint32 index) const override {
		return mType;
	}

	const char* dump_name() override { return "src"; }
	IVDPixmapGen* dump_src(int index) override { return 0; }

protected:
	const void *mpSrc;
	ptrdiff_t	mPitch;
	size_t		mBpr;
	sint32		mWidth;
	sint32		mHeight;
	uint32		mType;
	int		mPlane;
};

class VDPixmapGenSrcAlpha: public VDPixmapGenSrc {
public:
	void TransformPixmapInfo(const FilterModPixmapInfo& src, FilterModPixmapInfo& dst) override {
		dst.copy_alpha(src);
		dst.ref_a = src.ref_a;
	}
};

#endif
