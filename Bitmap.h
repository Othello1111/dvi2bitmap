// Part of dvi2bitmap.
// Copyright 1999, 2000 Council for the Central Laboratory of the Research Councils.
// See file LICENCE for conditions.
//
// $Id$

#ifndef BITMAP_HEADER_READ
#define BITMAP_HEADER_READ 1

#include "Byte.h"
#include "DviError.h"
#include "verbosity.h"

class BitmapError : public DviError {
 public:
    BitmapError(string s) : DviError(s) { };
};

class Bitmap {
 public:
    Bitmap (const int width, const int height, const int bpp=1);
    ~Bitmap();

    // make sure Left..Bottom are 0..3 (I should use an iterator, I know...)
    enum Margin { Left=0, Right=1, Top=2, Bottom=3, All};

    void paint (const int x, const int y, const int w, const int h,
		const Byte* b);
    void rule (const int x, const int y, const int w, const int h);
    void strut (const int x, const int y,
		const int l, const int r,
		const int t, const int b);
    void write (const string filename, const string format);
    void freeze ();
    void crop ();
    static void cropDefault (Margin spec, int pixels, bool absolute=false);
    void crop (Margin spec, int pixels, bool absolute=false);
    void blur ();
    void setTransparent(const bool sw) { transparent_ = sw; }
    typedef struct BitmapColour_s {
	Byte red, green, blue;
    } BitmapColour;
    void setRGB (const bool fg, const BitmapColour*);
    static void setDefaultRGB (const bool fg, const BitmapColour*);
    void scaleDown (const int factor);
    // If the bounding-box variables have their initial impossible values,
    // then either nothing has been written to the bitmap, or it was
    // all out of bounds.
    bool empty () const
	{ return (bbL > W || bbR < 0 || bbT > H || bbB < 0); }
    // Does the bitmap overlap its canvas?  This can only be true before a
    // call to freeze(), since that normalizes the bounding box variables.
    bool overlaps () const
	{ if (frozen_)
	    throw BitmapError
		("Bitmap::overlaps called after freeze() when it is always true");
	else
	    return (bbL < 0 || bbR > W || bbT < 0 || bbB > H); }
    int *boundingBox ()
	{ BB[0]=bbL; BB[1]=bbT; BB[2]=bbR; BB[3]=bbB; return &BB[0]; }
    static void verbosity (const verbosities level) { verbosity_ = level; }
    static void logBitmapInfo (bool b) { logBitmapInfo_ = b; };

 private:
    // pointer to bitmap.  Pixel (x,y) is at B[y*W + x];
    Byte *B;
    // width and height of bitmap
    int W, H;
    // bounding box - 
    // bbL and bbT are the leftmost and topmost blackened pixels,
    // bbR and bbB are one more than the rightmost and lowest blackened pixels.
    // Until a call to freeze(), bb? may go outside the canvas (ie, may
    // be negative or greater than W or H); afterwards they are bounded by
    // 0..W and 0..H.  (they were distinct variables until
    // Bitmap.cc 1.13 and Bitmap.h 1.11).
    int bbL, bbR, bbT, bbB;
    int BB[4];			// holds return values for boundingBox()
    bool frozen_;
    // cropX is the value of bbX when the crop() method was called
    int cropL, cropR, cropT, cropB;
    bool cropped_;
    // When cropping, set margins.  Indexed by enumerator Bitmap::Margin.
    static int  cropMarginDefault[4];
    int  cropMargin[4];
    static bool cropMarginAbsDefault[4];
    bool cropMarginAbs[4];
    bool transparent_;		// make bg transparent if poss.
    //Byte fg_red_, fg_green_, fg_blue_, bg_red_, bg_green_, bg_blue_;
    BitmapColour fg_, bg_;
    bool customRGB_;		// have custom colours been set?
    //static Byte def_fg_red_, def_fg_green_, def_fg_blue_,
    //def_bg_red_, def_bg_green_, def_bg_blue_;
    static BitmapColour def_fg_, def_bg_;
    static bool def_customRGB_;
    int bpp_;			// bits-per-pixel
    Byte max_colour_;		// ==> max colour index (must fit into
				// a Byte)
    static verbosities verbosity_;

    static bool logBitmapInfo_;
};

#endif //#ifndef BITMAP_HEADER_READ
