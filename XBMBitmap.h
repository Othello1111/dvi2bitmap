//    This file is part of dvi2bitmap.
//    Copyright 1999--2002, Council for the Central Laboratory of the Research Councils
//    
//    This program is part of the Starlink Software Distribution: see
//    http://www.starlink.ac.uk 
//
//    dvi2bitmap is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    dvi2bitmap is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with dvi2bitmap; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    The General Public License is distributed along with this
//    program in the file LICENCE.
//
//    Author: Norman Gray <norman@astro.gla.ac.uk>
//    $Id: XBMBitmap.h,v 1.7 2003/02/16 21:48:06 norman Exp $


#ifndef XBMBITMAP_HEADER_READ
#define XBMBITMAP_HEADER_READ 1

#include "BitmapImage.h"

class XBMBitmap : public BitmapImage {
 public:
    XBMBitmap (const int w, const int h);
    ~XBMBitmap();
    void setBitmap (const Byte *b);
    void setBitmapRow (const Byte *B);
    void setTransparent (const bool) { };
    void write (const string filename);
    string fileExtension() const { return "xbm"; }
};

#endif // #ifndef XBMBITMAP_HEADER_READ
