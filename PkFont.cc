// part of dvi2bitmap
// $Id$

#include "dvi2bitmap.h"
#include "InputByteStream.h"
#include "PkFont.h"

PkFont::PkFont(unsigned int c,
	       unsigned int s,
	       unsigned int d,
	       string name)
{
    name_ = "/var/lib/texmf/pk/ljfour/public/cm/cmr6.600pk"; // temp
    ibs_ = new InputByteStream (name_, true);

    for (int i=0; i<nglyphs_; i++)
	glyphs_[i] = 0;
    read_font (*ibs_);
};

PkFont::~PkFont()
{
    delete ibs_;
}

void PkFont::read_font (InputByteStream& ibs)
{
    // read the preamble, and check that the requested parameters match
    Byte preamble_opcode;
    if ((preamble_opcode = ibs.getByte()) != 247)
	throw DviError ("PK file doesn't start with preamble");
    if ((id_ = ibs.getByte()) != 89)
	throw DviError ("PK file has wrong ID byte");

    int comment_length = ibs.getByte();
    comment_ = "";
    for (;comment_length > 0; comment_length--)
	comment_ += static_cast<char>(ibs.getByte());
    ds_   = ibs.getUIU(4);
    cs_   = ibs.getUIU(4);
    hppp_ = ibs.getUIU(4);
    vppp_ = ibs.getUIU(4);


    cout << "PK file " << name_ << " '" << comment_ << "\'\n"
	 << "ds="  << ds_
	 << " cs=" << cs_
	 << " hppp=" << hppp_
	 << " vppp=" << vppp_
	 << '\n';

    // Now scan through the file, reporting opcodes and character definitions
    bool end_of_scan = false;
    while (! end_of_scan)
    {
	Byte opcode = ibs.getByte();
	if (opcode <= 239)	// character definition
	{
	    Byte dyn_f = opcode >> 4;
	    bool black_count = opcode & 8;
	    bool two_byte = opcode & 4;
	    Byte pl_prefix = opcode & 3;
	    unsigned int packet_length;

	    unsigned int g_cc, g_tfmwidth, g_dm, g_dx, g_dy, g_w, g_h;
	    int g_hoff, g_voff;
	    unsigned int g_hoffu, g_voffu;
	    if (two_byte)
		if (pl_prefix == 3) // long-form character preamble
		{
		    packet_length = ibs.getUIU(4);
		    g_cc       = ibs.getByte();
		    g_tfmwidth = ibs.getUIU(4);
		    g_dx       = ibs.getUIU(4);
		    g_dy       = ibs.getUIU(4);
		    g_w        = ibs.getUIU(4);
		    g_h        = ibs.getUIU(4);
		    g_hoffu    = ibs.getUIU(4);
		    g_voffu    = ibs.getUIU(4);

		    if (g_cc < 0 || g_cc >= nglyphs_)
			throw DviError
			    ("PK file has out-of-range character code");

		    packet_length -= 7*4;
		    glyph_data_[g_cc].pos = ibs.pos();
		    glyph_data_[g_cc].len = packet_length;
		    glyphs_[g_cc] = new PkGlyph (dyn_f,
						 g_cc, g_tfmwidth, g_dx, g_dy, 
						 g_w, g_h, g_hoffu, g_voffu);
		    ibs.skip (packet_length);
		}
		else		// extended short form character preamble
		{
		    packet_length = pl_prefix;
		    packet_length <<= 16;
		    packet_length += ibs.getUIU(2);
		    g_cc       = ibs.getByte();
		    g_tfmwidth = ibs.getUIU(3);
		    g_dm       = ibs.getUIU(2);
		    g_w        = ibs.getUIU(2);
		    g_h        = ibs.getUIU(2);
		    g_hoff     = ibs.getSIS(2);
		    g_voff     = ibs.getSIS(2);

		    if (g_cc < 0 || g_cc >= nglyphs_)
			throw DviError
			    ("PK file has out-of-range character code");

		    packet_length -= 3 + 5*2;
		    glyph_data_[g_cc].pos = ibs.pos();
		    glyph_data_[g_cc].len = packet_length;
		    glyphs_[g_cc] = new PkGlyph (dyn_f,
						 g_cc, g_tfmwidth, g_dm,
						 g_w, g_h, g_hoff, g_voff);
		    ibs.skip (packet_length);
		}
	    else		// short form character preamble
	    {
		packet_length = pl_prefix;
		packet_length <<= 8;
		packet_length += ibs.getByte();
		g_cc       = ibs.getByte();
		g_tfmwidth = ibs.getUIU(3);
		g_dm       = ibs.getUIU(1);
		g_w        = ibs.getUIU(1);
		g_h        = ibs.getUIU(1);
		g_hoff     = ibs.getSIS(1);
		g_voff     = ibs.getSIS(1);

		if (g_cc < 0 || g_cc >= nglyphs_)
		    throw DviError
			("PK file has out-of-range character code");

		packet_length -= 8;
		glyph_data_[g_cc].pos = ibs.pos();
		glyph_data_[g_cc].len = packet_length;
		glyphs_[g_cc] = new PkGlyph (dyn_f,
					     g_cc, g_tfmwidth, g_dm,
					     g_w, g_h, g_hoff, g_voff);
		ibs.skip(packet_length);
	    }
	    cout << "Char " << g_cc
		 << " tfm=" << g_tfmwidth
		 << " w="   << g_w
		 << " h="   << g_h
		 << " off=(" << g_hoff << ',' << g_voff
		 << ")\n\tat " << glyph_data_[g_cc].pos
		 << '(' << packet_length << ")\n";
	}
	else			// opcode is command
	{
	    int lenb = 0;
	    switch (opcode)
	    {
	      case 243:		// pk_xxx4
		lenb++;
	      case 242:		// pk_xxx3
		lenb++;
	      case 241:		// pk_xxx2
		lenb++;
	      case 240:		// pk_xxx1
		lenb++;
		{
		    string special = "";
		    for (unsigned int special_len = ibs.getUIU(lenb);
			 special_len > 0;
			 special_len--)
			special += static_cast<char>(ibs.getByte());
		    cout << "Special \'" << special << "\'\n";
		}
		break;
	      case 244:		// pk_yyy
		for (int i=0; i<4; i++) (void) ibs.getByte();
		break;
	      case 245:		// pk_post
		end_of_scan = true;
		break;
	      case 256:		// pk_no_op
		break;
	      case 257:		// pk_pre
		throw DviError ("Found PK preamble in body of file");
		break;
	      default:
		throw DviError ("Found unexpected opcode in PK file");
		break;
	    }
	}
    }
}

PkGlyph *PkFont::glyph (unsigned int n)
const
{
    if (n < 0 || n >= nglyphs_)
	throw DviError ("glyph number out of range");

    // Check that the glyph has its bitmap first. If not, have it create
    // it, in which case we pass it the rasterinfo from the PK file.
    if (glyphs_[n]->bitmap() == 0)
	glyphs_[n]->construct_bitmap (ibs_->getBlock(glyph_data_[n].pos,
						     glyph_data_[n].len));
    return glyphs_[n];
}
	

unsigned int PkGlyph::unpackpk (PKDecodeState& s)
{
    unsigned int res = 0;
    
    Byte n = nybble(s);
    if (n == 0)
    {
	// it's a 'large' number
	int nn = 1;
	while ((n = nybble(s)) == 0)
	    nn++;
	res = n;
	for (nn--; nn>0; nn--)
	    res = res<<4 + nybble(s);
	res = res - (13-dyn_f_)*16 - dyn_f_ + 15;
    }
    else if (n <= dyn_f_)
	res = n;
    else if (n < 14)
	res = (n-dyn_f_-1)*16+(dyn_f_+1) + nybble(s);
    else
    {
	// it's a repeatcount
	if (s.repeatcount != 0)
	    throw DviError ("found double repeatcount in unpackpk");
	if (n == 15)
	    s.repeatcount = 1;
	else
	    s.repeatcount = unpackpk (s);
	res = unpackpk(s);	// get the following runcount
    }
    return res;
}

// Construct a bitmap from the provided rasterinfo, which has come from
// the PK file.  Place the resulting bitmap in bitmap_
void PkGlyph::construct_bitmap (const Byte *rasterinfo)
{
    Byte runcount;
    // highnybble_ , rasterp_ and repeatcount_ are class variables
    // shared between construct_bitmap, nybble and unpackpk
    struct PKDecodeState pkds;
    pkds.highnybble = false;
    pkds.rasterp = rasterinfo;
    pkds.repeatcount = 0;

    bitmap_ = new Byte[w_ * h_];

    Byte pixelcolour = 1;	// start off black

    if (dyn_f_ == 14)
    {
	// rasterinfo is a pure bitmap - no decoding necessary
	unsigned int nbits_req = w_*h_;
	Byte *p = bitmap_;
	Byte b;

	while (nbits_req >= 8)
	{
	    for (int i=7, b=*rasterinfo; i>=0; i--, b>>=1)
		p[i] = b&1;
	    p += 8;
	    rasterinfo++;
	    nbits_req -= 8;
	}
	if (nbits_req > 0)
	{
	    // get the last few bits
	    b >>= (8-nbits_req);
	    for (int i=nbits_req-1; i>=0; i--, b>>=1)
		p[i] = b&1;
	}
    }
    else
    {
	// decode the rasterinfo
	Byte *rowp = bitmap_;
	Byte *rowstart = bitmap_;
	unsigned int nrow = 0;
	unsigned int ncol = 0;

	while (nrow < h_)
	{
	    runcount = unpackpk(pkds);
	    for (; runcount>0; runcount--)
	    {
		*rowp++ = pixelcolour;
		ncol++;
		if (ncol == w_)	// end of row
		{
		    ncol = 0;
		    nrow++;
		    if (pkds.repeatcount > 0)
		    {
			Byte *rowend=rowp;
			for (; pkds.repeatcount>0; pkds.repeatcount--)
			{
			    Byte *pt=rowstart;
			    while (pt < rowend)
				*rowp++ = *pt++;
			    nrow++;
			}
		    }
		    rowstart = rowp;
		}
	    }
	    pixelcolour = (pixelcolour==0 ? 1 : 0);
	}
    }
}
