// Part of dvi2bitmap.
// Copyright 1999, 2000, 2001 Council for the Central Laboratory of the Research Councils.
// See file LICENCE for conditions.

static const char RCSID[] =
	"$Id$";

// FIXME: at several points in the option processing below, I've noted
// sensible changes to the behaviour.  These, and the corresponding
// documentation, should be changed come the next minor version update
// (but not between bugfix releases).  At the same time, check that
// the documentation (SGML and roff), and the usage message at the
// bottom of this file, are consistent with the new options.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dvi2bitmap.h"
#include <vector>
#include <iostream>
#include <string>

#if HAVE_CSTD_INCLUDE
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cctype>
using std::exit;
#else
#include <stdio.h>		// for vsprintf
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#endif

#include "DviFile.h"
#include "PkFont.h"
#include "Bitmap.h"
#include "BitmapImage.h"
#include "verbosity.h"
#include "PageRange.h"
#include "Util.h"
#include "stringstream.h"
#include "version.h"

#if ENABLE_KPATHSEA
#include "kpathsea.h"
#endif
#if ENABLE_PNG
#include "PNGBitmap.h"		// for PNGBitmap::libpng_version
#endif

#define DVI2BITMAPURL "http://www.astro.gla.ac.uk/users/norman/star/dvi2bitmap/"

typedef vector<string> string_list;

// bitmap_info keeps together all the detailed information about the
// bitmap to be written.
struct bitmap_info {
    bitmap_info()
	: blur_bitmap(false), crop_bitmap(true),
	  make_transparent(true), bitmap_scale_factor(1),
	  ofile_pattern(""), ofile_name(""), ofile_type("") { }
    bool blur_bitmap;
    bool crop_bitmap;
    bool make_transparent;
    int bitmap_scale_factor;
    string ofile_pattern;
    string ofile_name;
    string ofile_type;
};

void process_dvi_file (DviFile *, bitmap_info&, int resolution,
		       const PkFont *fallback_font, PageRange&);
bool process_special (DviFile *, string specialString,
		      Bitmap*, bitmap_info&);
string_list& tokenise_string (string s);
string get_ofn_pattern (string dviname);
bool parseRGB (Bitmap::BitmapColour&, char*);
int doRegression (ostream& o);
void Usage (void);
char *progname;

verbosities verbosity = normal;
int bitmapH = -1;
int bitmapW = -1;

// Make resolution global -- 
// several functions in here might reasonably want to see this.
//int resolution = 72;		// in pixels-per-inch
//int oneInch = 72;		// one inch, including magnification
int resolution = PkFont::dpiBase();// in pixels-per-inch
int oneInch = resolution;

#define FONT_CMDS 4
#define FONT_INCFOUND 2
#define FONT_SHOW 1

int main (int argc, char **argv)
{
    string dviname;
    double magmag = 1.0;	// magnification of file magnification factor
    unsigned int show_font_list = 0;
    bitmap_info bm;
    // Usual processing is process_dvi.  preamble_only exits after
    // checking the preamble (and thus checking fonts).  options_only
    // merely processes the options and then exits.
    enum { process_dvi, preamble_only, options_only } processing_ =process_dvi;
    bool all_fonts_present = true;
    bool no_font_present = true;
    PageRange PR;


#define MM oneInch * 0.03937
    struct {
	char *name; double w; double h;
    } papersizes[] = {
	// Do these calculations in terms of the unit oneInch
	// Defn of inch: 1m=39.37in => 1mm=0.03937in
	{ (char*)"a4",		210 * MM,	297 * MM 	}, // 210x297mm
	{ (char*)"a4l",		297 * MM,	210 * MM,	}, // 297x210mm
	{ (char*)"a5",		148 * MM,	210 * MM,	}, // 148x210mm
	{ (char*)"a5l",		210 * MM,	148 * MM,	}, // 210x148mm
	{ (char*)"usletter",	8.5*oneInch,	8.5*oneInch,	}, // 8.5x11in
    };
    int npapersizes = sizeof(papersizes)/sizeof(papersizes[0]);
	

    progname = argv[0];

#if ENABLE_KPATHSEA
    kpathsea::init (progname, resolution);
#endif

    BitmapImage::setInfo (BitmapImage::SOFTWAREVERSION,
			  new string(version_string));
    BitmapImage::setInfo (BitmapImage::FURTHERINFO,
			  new string (DVI2BITMAPURL));

    bool absCrop = false;

    for (argc--, argv++; argc>0; argc--, argv++)
	if (**argv == '-')
	    switch (*++*argv)
	    {
	      case 'b':
		switch (*++*argv)
		{
		  case 'h':
		    argc--, argv++; if (argc <= 0) Usage();
		    bitmapH = atoi (*argv);
		    break;
		  case 'w':
		    argc--, argv++; if (argc <= 0) Usage();
		    bitmapW = atoi (*argv);
		    break;
		  case 'p':
		    argc--, argv++; if (argc <= 0) Usage();
		    // Note that the functionality here will vary
		    // depending on whether the magmag is set before
		    // or after this option, and it'll take no
		    // account of variations of the magnification
		    // within the DVI file.
		    int i;
		    for (i = 0; i<npapersizes; i++)
			if (strcmp (*argv, papersizes[i].name) == 0)
			{
			    bitmapH
				= static_cast<int>(magmag*papersizes[i].h+0.5);
			    bitmapW
				= static_cast<int>(magmag*papersizes[i].w+0.5);
			    break;
			}
		    if (i == npapersizes)
			cerr << "-bs " << *argv
			     << " not recognised.  See -Qp" << endl;
		    else
			if (verbosity > normal)
			    cerr << "Papersize " << *argv
				 << ": H=" << bitmapH
				 << " W=" << bitmapW
				 << endl;
		    break;
		    
		  default:
		    Usage();
		    break;
		}
		break;
	      case 'C':		// absolute crop
		absCrop = true;
		// FALL THROUGH
	      case 'c':		// crop
	      {
		  char c = *++*argv;
		  argc--, argv++; if (argc <= 0) Usage();
		  // get dimension, and convert points to pixels.
		  // Note that the functionality here will vary
		  // depending on whether the magmag is set before
		  // or after this option, and it'll take no
		  // account of variations of the magnification
		  // within the DVI file.
		  int cropmargin = static_cast<int>(magmag*atof(*argv)/72.0*resolution);
		  switch (c)
		  {
		    case 'l':
		      Bitmap::cropDefault(Bitmap::Left,	  cropmargin, absCrop);
		      break;
		    case 'r':
		      Bitmap::cropDefault(Bitmap::Right,  cropmargin, absCrop);
		      break;
		    case 't':
		      Bitmap::cropDefault(Bitmap::Top,	  cropmargin, absCrop);
		      break;
		    case 'b':
		      Bitmap::cropDefault(Bitmap::Bottom, cropmargin, absCrop);
		      break;
		    case '\0':
		      if (absCrop) // don't want this!
			  Usage();
		      Bitmap::cropDefault(Bitmap::All,	  cropmargin, false);
		      break;
		    default:
		      Usage();
		  }
		  break;
	      }
	      case 'f':
		{
		    char sel = *++*argv;
		    argc--, argv++;
		    if (argc <= 0)
			Usage();
		    switch (sel)
		    {
		      case '\0':
		      case 'p':	// -fp set PK font path
			PkFont::setFontPath(*argv);
			break;
		      case 'm':	// -fm set PK font generation mode
			PkFont::setMissingFontMode (*argv);
			break;
		      case 'g':	// -fg switch off font generation
			// FIXME: to be consistent with -P, options
			// -fg and -fG should be the other way
			// around.  Change in next version.
			PkFont::setMakeFonts (false);
			break;
		      case 'G':	// -fG switch on font generation
			PkFont::setMakeFonts (true);
			break;
		      default:
			Usage();
			break;
		    }
		    break;
		}
	      case 'g':		// debugging...
		{
		    verbosities debuglevel = debug;
		    for (++*argv; **argv != '\0'; ++*argv)
			switch (**argv)
			{
			  case 'd': // debug DVI file
			    DviFile::verbosity(debuglevel);
			    break;
			  case 'p': // debug PK file (and kpathsea)
			    PkFont::verbosity(debuglevel);
			    break;
			  case 'r': // debug rasterdata parsing
			    PkRasterdata::verbosity(debuglevel);
			    break;
			  case 'i': // debug input
			    InputByteStream::verbosity(debuglevel);
			    break;
			  case 'b': // debug bitmap
			    Bitmap::verbosity(debuglevel);
			    BitmapImage::verbosity(debuglevel);
			    break;
			  case 'm': // debug main program
			    verbosity = debuglevel;
			    break;
			  case 'u': // debug utility functions
			    Util::verbosity(debuglevel);
			    break;
			  case 'g':
			    if (debuglevel == debug)
				debuglevel = everything;
			    break;
			  default:
			    Usage();
			}
		}
		break;
	      // case 'l': see below
	      case 'm':		// set magnification
		argc--, argv++;
		if (argc <= 0)
		    Usage();
		magmag = atof (*argv);
		break;
	      case 'n':		// don't actually process the DVI file
		switch (*++*argv)
		{
		  case '\0':
		    processing_ = preamble_only;
		    PkFont::setMakeFonts (false);
		    break;
		  case 'n':	// do nothing -- don't even look for a DVI file
		    processing_ = options_only;
		    break;
		  case 'f':
		    // FIXME: remove this option in the next version
		    // (degenerate with -fg) 
		    PkFont::setMakeFonts (false);
		    break;
		  default:
		    Usage();
		    break;
		}
		break;
	      case 'o':		// set output filename pattern
		argc--, argv++;
		if (argc <= 0)
		    Usage();
		bm.ofile_pattern = *argv;
		break;
	      case 'P':		// process the bitmap...
		while (*++*argv != '\0')
		    switch (**argv)
		    {
		      case 'b':	// blur bitmap
			bm.blur_bitmap = true;
			break;
		      case 'B':	// don't
			bm.blur_bitmap = false;
			break;
		      case 't':	// make bitmap transparent
			bm.make_transparent = true;
			break;
		      case 'T':	// don't
			bm.make_transparent = false;
			break;
		      case 'c':	// crop bitmap
			bm.crop_bitmap = true;
			break;
		      case 'C':	// don't
			bm.crop_bitmap = false;
			break;
		      default:
			Usage();
			break;
		    }
		break;
	      case 'l':		// set page range
	      case 'p':
		if (argc <= 1)
		    Usage();
		if (! PR.addSpec (argv[0], argv[1]))
		    Usage();
		argc--, argv++;
		break;
	      case 'q':		// run quietly
		verbosity = quiet;
		if (*++*argv == 'q') // run very quietly
		    verbosity = silent;
		DviFile::verbosity(verbosity);
		PkFont::verbosity(verbosity);
		PkRasterdata::verbosity(verbosity);
		InputByteStream::verbosity(verbosity);
		Bitmap::verbosity(verbosity);
		BitmapImage::verbosity(verbosity);
		Util::verbosity(verbosity);
		break;
	      case 'Q':		// various queries
		while (*++*argv != '\0')
		    switch (**argv)
		    {
		      case 'F':		// show missing fonts
			show_font_list = (FONT_SHOW | FONT_INCFOUND);
			break;

		      case 'f':
			show_font_list = FONT_SHOW;
			break;

		      case 'G':
			show_font_list
			    = (FONT_SHOW | FONT_INCFOUND | FONT_CMDS);
			break;

		      case 'g':
			show_font_list = (FONT_SHOW | FONT_CMDS);
			break;

		      case 't':	// show file types
			{
			    cout << "Qt ";
			    const char *ft
				= BitmapImage::firstBitmapImageFormat();
			    cout << ft;
			    ft = BitmapImage::nextBitmapImageFormat();
			    while (ft != 0)
			    {
				cout << ' ' << ft;
				ft = BitmapImage::nextBitmapImageFormat();
			    }
			    cout << endl;
			    // FIXME: remove the following line at the
			    // next version change.  Change behaviour
			    // so that -Qt does not automatically exit.
			    processing_ = options_only;
			    break;
			}

		      case 'b':	// show bitmap info
			Bitmap::logBitmapInfo (true);
			break;

		      case 'p':	// show `paper sizes'
			cout << "Qp";
			for (int i=0; i<npapersizes; i++)
			    cout << ' ' << papersizes[i].name;
			cout << endl;
			break;

		      default:
			Usage();
			break;
		    }
		break;
	      case 'r':		// set resolution
		argc--, argv++;
		if (argc <= 0)
		    Usage();
		PkFont::setResolution (atoi(*argv));
		resolution = PkFont::dpiBase();
		break;
	      case 'R':		// set colours
		char c;
		c = *++*argv;
		argc--, argv++;
		if (argc <= 0)
		    Usage();
		if (c == 'f' || c == 'b')
		{
		    Bitmap::BitmapColour rgb;
		    if (parseRGB(rgb, *argv))
			Bitmap::setDefaultRGB (c=='f', &rgb);
		    else
			Usage();
		}
		else
		    Usage();
		break;		    
	      case 's':		// scale down
		argc--, argv++;
		if (argc <= 0)
		    Usage();
		bm.bitmap_scale_factor = atoi (*argv);
		break;
	      case 't':		// set output file type
		argc--, argv++;
		if (argc <= 0)
		    Usage();
		bm.ofile_type = *argv;
		if (! BitmapImage::supportedBitmapImage (bm.ofile_type))
		{
		    bm.ofile_type
			= BitmapImage::firstBitmapImageFormat();
		    cerr << "Unsupported image type "
			 << *argv
			 << ": using "
			 << bm.ofile_type
			 << " instead" << endl;
		}
		break;
	      case 'X':		// generate regression-test output
		try
		{
		    doRegression(cout);
		    processing_ = options_only;
		    break;
		}
		catch (DviBug& e)
		{
		    cerr << "Bug exception running regression tests ("
			 << e.problem() << ")" << endl;
		}
		catch (DviError& e)
		{
		    cerr << "Error exception running regression tests ("
			 << e.problem() << ")" << endl;
		}

	      case 'V':		// display version
		cout << version_string << endl << "Options:" << endl;

		cout << "ENABLE_GIF           "
		     << (ENABLE_GIF ? "yes" : "no") << endl;
		cout << "ENABLE_PNG           "
		     << (ENABLE_PNG ? "yes" : "no") << endl;
#if ENABLE_PNG
		cout << "  libpng: "
		     << PNGBitmap::version_string() << endl;
#endif

		cout << "ENABLE_KPATHSEA      "
		     << (ENABLE_KPATHSEA ? "yes" : "no") << endl;
#if ENABLE_KPATHSEA
		cout << "  libkpathsea: "
		     << kpathsea::version_string() << endl;
#endif
#ifdef FONT_SEARCH_STRING
		cout << "FONT_SEARCH_STRING   " << FONT_SEARCH_STRING << endl;
#endif
#ifdef DEFAULT_TEXMFCNF
		cout << "  DEFAULT_TEXMFCNF=" << DEFAULT_TEXMFCNF << endl;
#endif
#ifdef FAKE_PROGNAME
		cout << "  FAKE_PROGNAME=" << FAKE_PROGNAME << endl;
#endif

#if defined(FONT_GEN_TEMPLATE)
		cout << "FONT_GEN_TEMPLATE    " << FONT_GEN_TEMPLATE << endl;
#else
		cout << "Font generation disabled" << endl;
#endif
#ifdef DEFAULT_MFMODE
		cout << "  DEFAULT_MFMODE=" << DEFAULT_MFMODE << endl;
#endif
#ifdef DEFAULT_RESOLUTION
		cout << "  DEFAULT_RESOLUTION=" << DEFAULT_RESOLUTION << endl;
#endif

		cout << RCSID << endl;
		processing_ = options_only; // ...and exit
		break;

	      default:
		Usage();
	    }
	else
	{
	    if (dviname.length() != 0)
		Usage();
	    dviname = *argv;
	}

    if (processing_ == options_only)
	exit (0);

    // Insist we have a DVI file specified.
    if (dviname.length() == 0)
	Usage();

    if (verbosity >= normal)
	// Banner
	cout << "This is " << version_string << endl;

    if (bm.ofile_pattern.length() == 0)
	bm.ofile_pattern = get_ofn_pattern (dviname);
    if (bm.ofile_pattern.length() == 0)
    {
	if (verbosity > silent)
	    cerr << "Error: Can't make output filename pattern from "
		 << dviname << endl;
	exit(1);
    }

    try
    {
	DviFile *dvif = new DviFile(dviname, resolution, magmag);
	if (dvif->eof())
	{
	    if (verbosity > silent)
		cerr << "Error: Can't open file " << dviname
		     << " to read" << endl;
	    exit(1);
	}

	all_fonts_present = true;
	no_font_present = true;
	const PkFont *fallback_font = 0;

	for (PkFont *f = dvif->firstFont();
	     f != 0;
	     f = dvif->nextFont())
	{
	    if (f->loaded())
	    {
		no_font_present = false;
		// Set the fallback font to be the first font named cmr10,
		// or the first font if there are none such.
		if (verbosity > normal)
		    cerr << "dvi2bitmap: loaded font " << f->name() << endl;
		if (fallback_font == 0)
		    fallback_font = f;
		else if (f->name() == "cmr10"
			 && fallback_font->name() != "cmr10")
		{
		    fallback_font = f;
		    if (verbosity > normal)
			cerr << "dvi2bitmap: fallback font now "
			     << fallback_font->name() << endl;
		}
	    }
	    else		// flag at least one missing
		all_fonts_present = false;

	    if (show_font_list & FONT_SHOW)
		if (show_font_list & FONT_CMDS)
		{
		    if ((show_font_list & FONT_INCFOUND) || !f->loaded())
		    {
			// If f->loaded() is true, then we're here
			// because FONT_INCFOUND was set, so indicate
			// this in the output.
			string cmd = f->fontgenCommand();
			if (cmd.length() == 0)
			    throw DviError ("configuration problem: I can't create a font-generation command");
			cout << (f->loaded() ? "QG " : "Qg ")
			     << f->fontgenCommand()
			     << endl;
		    }
		}
		else
		    if ((show_font_list & FONT_INCFOUND) || !f->loaded())
		    {
			// If f->loaded() is true, then we're here
			// because FONT_INCFOUND was set, so indicate
			// this in the output.
			cout << (f->loaded() ? "QF " : "Qf ");

			// write out font name, dpi, base-dpi, mag and MF mode
			cout << f->name() << ' '
			     << f->dpiBase() << ' '
			     << f->dpiScaled() << ' '
			     << f->magnification()
			     << " localfont";
			if (f->loaded())
			{
			    string fn = f->fontFilename();
			    string unk = "unknown";
			    cout << ' ' << (fn.length() > 0 ? fn : unk);
			}
			cout << endl;
		    }
	}


	if (processing_ == process_dvi)
	{
	    if (no_font_present) // give up!
	    {
		if (verbosity > silent)
		    cerr << progname << ": no fonts found!  Giving up" << endl;
	    }
	    else
		process_dvi_file (dvif, bm, resolution, fallback_font, PR);
	}

    }
    catch (DviBug& e)
    {
	if (verbosity > silent)
	    e.print();
    }
    catch (DviError& e)
    {
	if (verbosity > silent)
	    e.print();
    }

    // Exit non-zero if we were just checking the pre- and postambles,
    // and we found some missing fonts.
    // Or put another way: exit zero if (a) we were processing the DVI
    // file normally and we found at least one font, or (b) we were
    // just checking the preamble and we found _all_ the fonts.
    if (no_font_present || (processing_==preamble_only && !all_fonts_present))
	exit (1);
    else
	exit (0);
}

void process_dvi_file (DviFile *dvif, bitmap_info& b, int fileResolution,
		       const PkFont *fallback_font, PageRange& PR)
{
    DviFileEvent *ev;
    const PkFont *curr_font = 0;
    int pagenum = 0;
    Bitmap *bitmap = 0;
    bool end_of_file = false;
    size_t outcount = 0;	// characters written to output current line
    bool initialisedInch = false;
    bool skipPage = false;

    while (! end_of_file)
    {
	if (skipPage)
	    ev = dvif->getEndOfPage();
	else
	    ev = dvif->getEvent();

	if (verbosity > debug)
	    ev->debug();

	if (! initialisedInch)
	{
	    // can't do this any earlier, as it's set in the preamble
	    oneInch = static_cast<int>(fileResolution * dvif->magnification());
	    initialisedInch = true;
	}

	if (DviFilePage *test = dynamic_cast<DviFilePage*>(ev))
	{
	    DviFilePage &page = *test;
	    if (page.isStart)
	    {
		pagenum++;

		// Are we to print this page?
		if (! PR.isSelected(pagenum, page.count))
		    skipPage = true;
		else
		{
		    // Request a big-enough bitmap; this bitmap is the `page'
		    // on which we `print' below.  hSize and vSize are the
		    // width and height of the widest and tallest pages,
		    // as reported by the DVI file; however, the file doesn't
		    // report the offsets of these pages.  Add a
		    // couple of inches to both and hope for the best.
		    bitmap = new Bitmap
			((bitmapW > 0 ? bitmapW : dvif->hSize()+2*oneInch),
			 (bitmapH > 0 ? bitmapH : dvif->vSize()+2*oneInch));
		    if (verbosity > quiet)
		    {
			int last, i;
			// find last non-zero count
			for (last=9; last>=0; last--)
			    if (page.count[last] != 0)
				break;
			
			SSTREAM pageind;
			pageind << '[' << page.count[0];
			for (i=1; i<=last; i++)
			    pageind << '.' << page.count[i];
			pageind << '\0';
			string ostr = pageind.str();
			if (outcount + ostr.length() > 78)
			{
			    cout << endl;
			    outcount = 0;
			}
			cout << ostr;
			outcount += ostr.length();
		    }
		}
	    }
	    else
	    {
		if (skipPage)
		{
		    // nothing to do in this case except reset it
		    skipPage = false;
		    if (bitmap != 0) // just in case
		    {
			delete bitmap;
			bitmap = 0;
		    }
		}
		else
		{
		    if (bitmap == 0)
			throw DviBug ("bitmap uninitialised at page end");
		    else if (bitmap->empty())
		    {
			if (verbosity > quiet)
			    cerr << "Warning: page " << pagenum
				 << " empty: nothing written" << endl;
		    }
		    else
		    {
			if (bitmap->overlaps() && verbosity > quiet)
			{
			    int *bb = bitmap->boundingBox();
			    cerr << "Warning: p." << pagenum
			     << ": bitmap too big: occupies (" << bb[0] << ','
			     << bb[1] << ")...(" << bb[2] << ','
			     << bb[3] << ").  Requested "
			     << (bitmapW > 0 ? bitmapW : dvif->hSize()+oneInch)
			     << 'x'
			     << (bitmapH > 0 ? bitmapH : dvif->vSize()+oneInch)
			     << endl;
			}
			if (b.crop_bitmap)
			    bitmap->crop();
			if (b.blur_bitmap)
			    bitmap->blur();
			if (b.make_transparent)
			    bitmap->setTransparent(true);
			if (b.bitmap_scale_factor != 1)
			    bitmap->scaleDown (b.bitmap_scale_factor);
			const string *fn = dvif->filename();
			if (fn->length() != 0)
			    BitmapImage::setInfo (BitmapImage::INPUTFILENAME,
						  fn);
			if (b.ofile_type.length() == 0)
			{
			    b.ofile_type 
				= BitmapImage::firstBitmapImageFormat();
			    /*
			    cerr << "Warning: unspecified image format.  Selecting default ("
				 << b.ofile_type << ")" << endl;
			    */
			}
			if (b.ofile_name.length() == 0)
			{
			    char fnb[100];
			    sprintf (fnb, b.ofile_pattern.c_str(), pagenum);
			    string output_filename = fnb;
			    bitmap->write (output_filename, b.ofile_type);
			}
			else
			    bitmap->write (b.ofile_name, b.ofile_type);
		    }
		    b.ofile_name = "";

		    delete bitmap;
		    bitmap = 0;

		    if (verbosity > quiet)
		    {
			cout << "] ";
			outcount += 2;
		    }
		}
	    }
	}
	else if (DviFileSetChar *test = dynamic_cast<DviFileSetChar*>(ev))
	{
	    if (curr_font == 0 || bitmap == 0)
		throw DviBug ("font or bitmap not initialised setting char");
	    DviFileSetChar& sc = *test;
	    PkGlyph& glyph = *curr_font->glyph(sc.charno);
	    if (verbosity > normal)
	    {
		cerr << "glyph `" << glyph.characterChar()
		     << "\' (" << glyph.characterCode() << ')';
		if (verbosity > debug)
		    cerr << " size " << glyph.w() << 'x' << glyph.h()
			 << " at position ("
			 << dvif->currH() << ',' << dvif->currV()
			 << ") plus oneInch=" << oneInch;
		cerr << endl;
	    }
	    // calculate glyph positions, taking into account the
	    // offsets for the bitmaps, and the (1in,1in)=(72pt,72pt)
	    // = (resolution px,resolution px) offset of the TeX origin.
	    int x = dvif->currH() + glyph.hoff() + oneInch;
	    int y = dvif->currV() + glyph.voff() + oneInch;
	    bitmap->paint (x, y,
			   glyph.w(), glyph.h(),
			   glyph.bitmap());
	}
	else if (DviFileSetRule *test = dynamic_cast<DviFileSetRule*>(ev))
	{
	    DviFileSetRule& sr = *test;
	    int x = dvif->currH() + oneInch;
	    int y = dvif->currV() + oneInch;
	    bitmap->rule (x,y,sr.w, sr.h);
	}
	else if (DviFileFontChange *test =
		 dynamic_cast<DviFileFontChange*>(ev))
	{
	    DviFileFontChange& fc = *test;
	    const PkFont *f = fc.font;
	    curr_font = (f->loaded() ? f : fallback_font);
	}
	else if (DviFileSpecial* test =
		 dynamic_cast<DviFileSpecial*>(ev))
	{
	    DviFileSpecial& special = *test;
	    if (!process_special (dvif,
				  special.specialString,
				  bitmap, b))
		if (verbosity > quiet)
		    cerr << "Warning: unrecognised special: "
			 << special.specialString
			 << endl;
	}
	else if (DviFilePostamble *post
		 = dynamic_cast<DviFilePostamble*>(ev))
	    end_of_file = true;

	delete ev;
    }

    if (verbosity > quiet)
	cout << endl;
}

// Process the special string, returning true on success.
bool process_special (DviFile *dvif, string specialString,
		      Bitmap* bitmap, bitmap_info& b)
{
    string_list l = tokenise_string (specialString);
    string_list::const_iterator s = l.begin();
    bool stringOK = false;
    bool setDefault = false;
    bool absolute = false;

    if (*s == "dvi2bitmap")	// OK
    {
	stringOK = true;
	s++;

	while (s != l.end() && stringOK)
	{
	    if (*s == "default")
		setDefault = true;
	    else if (*s == "absolute")
		absolute = true;
	    else if (*s == "outputfile")
	    {
		s++;
		if (s == l.end())
		    stringOK = false;
		else
		    if (setDefault)
		    {
			bool seenHash = false;
			b.ofile_pattern = "";
			for (unsigned int i=0; i<s->length(); i++)
			{
			    char c;
			    if ((c=(*s)[i]) == '#')
			    {
				if (! seenHash)
				{
				    b.ofile_pattern += '%';
				    b.ofile_pattern += 'd';
				    seenHash = true;
				}
			    }
			    else
				b.ofile_pattern += c;
			}
			if (!seenHash)
			{
			    b.ofile_pattern += '%';
			    b.ofile_pattern += 'd';
			}
			if (verbosity > normal)
			    cerr << "special: ofile_pattern="
				 << b.ofile_pattern << endl;
		    }
		    else
			b.ofile_name = *s;
	    }
	    else if (*s == "crop")
	    {
		s++;
		if (s == l.end()) { stringOK = false; break; }
		string side_s = *s;
		Bitmap::Margin side = Bitmap::All;
		s++;
		if (s == l.end()) { stringOK = false; break; }
		int dimen = atoi (s->c_str());
		// scale from points to pixels
		double npixels = dimen/72.0    // to inches
		    * oneInch;
		if (absolute)
		{
		    // these dimensions are given w.r.t. an origin one inch
		    // from the left and top of the `paper'.  Add this inch:
		    npixels += oneInch;
		}
		dimen = static_cast<int>(npixels);

		if (side_s == "left")
		    side = Bitmap::Left;
		else if (side_s == "right")
		    side = Bitmap::Right;
		else if (side_s == "top")
		    side = Bitmap::Top;
		else if (side_s == "bottom")
		    side = Bitmap::Bottom;
		else if (side_s == "all")
		    side = Bitmap::All;
		else
		    stringOK = false;

		if (verbosity > normal)
		    cerr << "Crop " << side_s << '=' << dimen
			 << (setDefault ? " default" : "")
			 << (absolute ? " absolute" : "")
			 << endl;

		if (stringOK)
		    if (side == Bitmap::All)
		    {
			if (setDefault)
			    for (int tside=0; tside<4; tside++)
				Bitmap::cropDefault
				    (static_cast<Bitmap::Margin>(tside),
				     dimen, absolute);
			for (int tside=0; tside<4; tside++)
			    bitmap->crop
				(static_cast<Bitmap::Margin>(tside),
				 dimen, absolute);
		    }
		    else
		    {
			if (setDefault)
			    Bitmap::cropDefault (side, dimen, absolute);
			bitmap->crop (side, dimen, absolute);
		    }
	    }
	    else if (*s == "imageformat")
	    {
		s++;
		if (s == l.end())
		    stringOK = false;
		else
		{
		    if (BitmapImage::supportedBitmapImage (*s))
			b.ofile_type = *s;
		    else
		    {
			b.ofile_type = BitmapImage::firstBitmapImageFormat();
			cerr << "Warning: imageformat " << *s
			     << " not supported.  Using " << b.ofile_type
			     << " instead." << endl;
		    }
		    if (!setDefault && verbosity > quiet)
			cerr << "Warning: imageformat special should be prefixed with `default'" << endl;
		}
	    }
	    else if (*s == "foreground" || *s == "background")
	    {
		bool isfg = (*s == "foreground");
		//Byte r, g, b;
		Bitmap::BitmapColour rgb;
		s++;
		if (s == l.end()) { stringOK = false; break; }
		rgb.red   = static_cast<Byte>(strtol (s->c_str(), 0, 0));
		s++;
		if (s == l.end()) { stringOK = false; break; }
		rgb.green = static_cast<Byte>(strtol (s->c_str(), 0, 0));
		s++;
		if (s == l.end()) { stringOK = false; break; }
		rgb.blue  = static_cast<Byte>(strtol (s->c_str(), 0, 0));

		if (stringOK)
		{
		    if (verbosity > normal)
			cerr << "Set "
			     << (setDefault ? "(default) " : "")
			     << (isfg ? "foreground" : "background")
			     << " to "
			     << static_cast<int>(rgb.red) << ','
			     << static_cast<int>(rgb.green) << ','
			     << static_cast<int>(rgb.blue) << endl;
		    if (setDefault)
			Bitmap::setDefaultRGB (isfg, &rgb);
		    else
			bitmap->setRGB (isfg, &rgb);
		}
	    }
	    else if (*s == "strut")
	    {
		int x = dvif->currH() + oneInch;
		int y = dvif->currV() + oneInch;
		int left, right, top, bottom;
		s++;
		if (s == l.end()) { stringOK = false; break; }
		left = dvif->pt2px(atof (s->c_str()));
		s++;
		if (s == l.end()) { stringOK = false; break; }
		right = dvif->pt2px(atof (s->c_str()));
		s++;
		if (s == l.end()) { stringOK = false; break; }
		top = dvif->pt2px(atof (s->c_str()));
		s++;
		if (s == l.end()) { stringOK = false; break; }
		bottom = dvif->pt2px(atof (s->c_str()));
		if (left<0 || right<0 || top<0 || bottom<0)
		{
		    if (verbosity > silent)
			cerr << "Strut must have positive dimensions" << endl;
		    stringOK = false;
		}
		else
		{
		    if (verbosity > normal)
			cerr << "Strut: (" << x << ',' << y
			     << ") (lrtb)=("
			     << left << ',' << right << ','
			     << top  << ',' << bottom << ")" << endl;
		    bitmap->strut (x, y, left, right, top, bottom);
		}
	    }
	    else
		stringOK = false;

	    s++;
	}
    }

    if (!stringOK && verbosity > quiet)
	cerr << "Warning: unrecognised special: " << specialString << endl;

    return stringOK;
}

string get_ofn_pattern (string dviname)
{
    // strip path and extension from filename
    size_t string_index = dviname.rfind(path_separator);
    string dvirootname;
    //if (string_index < 0)
    if (string_index == string::npos)
	dvirootname = dviname;
    else
	dvirootname = dviname.substr(string_index+1);
    string_index = dvirootname.rfind('.');
    if (string_index != string::npos) // there is an extension -- skip it
	dvirootname = dvirootname.substr(0,string_index);

    return dvirootname + "-page%d";
}

// Tokenise string at whitespace.  There's a more C++-ish way of doing
// this, I'm sure....
string_list& tokenise_string (string str)
{
    static bool initialised = false;
    static string_list *l;

    if (verbosity > normal)
	cerr << "tokenise_string: string=<" << str << ">" << endl;

    if (! initialised)
    {
	l = new string_list();
	initialised = true;
    }
    else
	l->clear();

    unsigned int i=0;

    // skip leading whitespace
    while (i < str.length() && isspace(str[i]))
	i++;
    while (i < str.length())
    {
	unsigned int wstart = i;
	while (i < str.length() && !isspace(str[i]))
	    i++;
	string t = str.substr(wstart,i-wstart);
	if (verbosity > normal)
	    cerr << "tokenise:" << t << ":" << endl;
	l->push_back(t);
	while (i < str.length() && isspace(str[i]))
	    i++;
    }
    return *l;
}

bool parseRGB (Bitmap::BitmapColour& rgb, char* s)
{
    char *p;
    rgb.red = static_cast<Byte>(strtol (s, &p, 0));
    if (p == s)			// no digit
	return false;
    if (*p == '\0')		// end of string
	return false;
    s = p;
    while (!isxdigit(*s))
    {
	if (*s == '\0') return false;
	s++;
    }

    rgb.green = static_cast<Byte>(strtol (s, &p, 0));
    if (p == s)			// no digit
	return false;
    if (*p == '\0')		// end of string
	return false;
    s = p;
    while (!isxdigit(*s))
    {
	if (*s == '\0') return false;
	s++;
    }

    rgb.blue = static_cast<Byte>(strtol (s, &p, 0));
    if (p == s)			// no digit
	return false;

    return true;
}


int doRegression (ostream& o)
{
    int rval = 0;
    rval += PkFont::regressionOutput ("PkFont:", o);
    rval += Util::regressionOutput ("Util:", o);
    return rval;
}


void Usage (void)
{
    cerr << "Usage: " << progname << " [-b(h|w) size] [-bp a4|a4l|usletter...]" << endl <<
"        [-[Cc][lrtb] size] [-fp PKpath ] [-fm mfmode] [-fg] [-fG]" << endl <<
"        [-g[dpribmg]] [-l num] [-m magmag ] [-n[n]] [-o outfile-pattern]" << endl <<
"        [-p num] [-pp ranges] [-P[bBtTcC]] [-q[q]] [-Q[FfGgtbp]]" << endl <<
"        [-r resolution] [-R[fb] int,int,int] [-s scale-factor]" << endl <<
"        [-t xbm"
#if ENABLE_GIF
	 << "|gif"
#endif
#if ENABLE_PNG
	 << "|png"
#endif
	 << "] [-V]" << endl <<
"	dvifile" << endl << endl <<
"  -bh, -bw  page height and width in pixels    -bp  bitmap <-- papersize" << endl <<
"  -c   crop margins left, right, top, bottom   -C   crop absolute" << endl <<
"  -fp  set font-path (DVI2BITMAP_PK_PATH)      -fg  switch off (-fG=on) fontgen" << endl <<
"  -fm  Metafont mode (must match -r)           -r   Metafont resolution" << endl <<
"  -m   DVI file magnification                  -s   scale-down of bitmap" << endl <<
"  -o   set output file pattern, including %d   -g   debugging of sections" << endl <<
"  -n   only process DVI file preamble          -nn  only process-options" << endl <<
"  -q   switch off chatter and warnings         -qq  switch off errors, too" << endl <<
"  -R   set foreground/background colour (RGB)  -V   version+features then exit" << endl <<
"  -l num, -p num, -pp ranges  select pages to process (l--p, or ranges pp)" << endl <<
"  -Q   query: f=missing fonts, g=missing font commands (F, G=all fonts)" << endl <<
"       t=supported output types, b=output bitmap names, p=paper sizes in -bp" << endl <<
"  -P   Processing: b=blur bitmap, t=set transparent, c=do cropping (BTC->off)" << endl;
    exit (1);
}
