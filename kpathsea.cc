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
//    $Id$


#include <config.h>

// Put the entire module inside #if ENABLE_KPATHSEA.
// We _shouldn't_ be compiling this is that's false, but we obviously are,
// if we get here, so make it a no-op.
#ifdef ENABLE_KPATHSEA

#define NULL 0
#include <iostream>		// for cerr

#ifdef HAVE_CSTD_INCLUDE
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#ifdef HAVE_STD_NAMESPACE
using std::cerr;
#endif

#include "kpathsea.h"

// The Kpathsea library interface has to be isolated in this class because the
// kpathsea headers typedef a `string' type, which conflicts with C++ string.
// It would be more appropriate to put this into a namespace, but egcs
// doesn't support that, so fake it by putting them in a class with only
// static methods.
// 
// The headers here use HAVE_ASSERT_H, so that might need to be configured.
// They also test HAVE_PROTOTYPES and STDC_HEADERS, both of which should be 
// defined here, since the facilities they represent should be
// available in C++ (that's `should' as in `ought' rather than as in
// `probably are'...).
//
// We must _avoid_ including c-std.h, since that file carefully
// declares popen and pclose, but does so K&R-style, with no
// arguments.  This seems to have got past most compilers, but gcc,
// apparently in a recent burst of standards enthusiasm, seems to have
// decided to object to this.  KPATHSEA_C_STD_H is the guard
// definition which c-std.h uses to avoid rereading itself; the one
// thing we do need from that file is stdio.h, so we read that
// explicitly.
#define HAVE_PROTOTYPES
#define STDC_HEADERS
#define KPATHSEA_C_STD_H 1
#include <stdio.h>
extern "C" {
#include <kpathsea/progname.h>
#include <kpathsea/debug.h>
#include <kpathsea/proginit.h>
#include <kpathsea/tex-glyph.h>
}
#undef STDC_HEADERS
#undef HAVE_PROTOTYPES


bool kpathsea::initialised_ = false;
verbosities kpathsea::verbosity_ = normal;

void kpathsea::init (const char *program_name, const int basedpi)
{
    if (initialised_ && verbosity_ > quiet)
	cerr << "Warning: kpathsea doubly initialised.  Ignored\n";
#ifdef DEFAULT_TEXMFCNF
    // if the TEXMFCNF variable isn't set in the environment, set it to this
    char *texmfcnf = getenv ("TEXMFCNF");
    if (texmfcnf == NULL)
    {
	if (setenv ("TEXMFCNF", DEFAULT_TEXMFCNF, 1) && verbosity_ > quiet)
	    cerr << "Warning: couldn't set TEXMFCNF\n";
    }
    else
	cerr << "TEXMFCNF=" << texmfcnf << '\n';
#endif

#ifdef FAKE_PROGNAME
    // Lie about the program name, so we can influence the definitions of
    // the SELFAUTO{LOC,DIR,PARENT} variables.
    //
    // kpse_set_program_name uses the value of program_invocation_name if
    // that's defined, and defines it, and initialises it to program_name,
    // if it's not.  The only way we can reliably intervene here is by
    // setting that variable ourselves.  Unfortunately, this means that we're
    // lying about this for the remainder of this program, but we can't do
    // better than this without independently discovering for ourselves if 
    // program_invocation_name is set in the libc library.  We can't get
    // round this by defining the variables by hand ourselves, since 
    // kpse_set_program_name does elaborate following of links and other
    // consistency checks, which we do not want to duplicate here.
    //
    // In any case, this is supposed to be not much more than a hack to avoid
    // being bitten by arguably misconfigured texmf.cnf files, so don't lose
    // sleep over it.
    program_name = program_invocation_name = FAKE_PROGNAME;
#endif

    kpse_set_program_name (program_name, "dvi2bitmap");

    //kpse_init_prog ("TEX", basedpi, "localfont", "cmr10");
    kpse_init_prog ("TEX", basedpi, NULL, NULL);
    initialised_ = true;
}

const char *kpathsea::version_string (void)
{
    extern char *kpathsea_version_string;
    return kpathsea_version_string;
}

const char *kpathsea::find (const char *fontname, int resolution)
{
    if (! initialised_ && verbosity_ > silent)
    {
	cerr << "Error: kpathsea not initialised\n";
	return 0;
    }

    kpse_glyph_file_type glyph_info;
    char *fname;
    fname = kpse_find_pk (fontname, resolution, &glyph_info);

    if (verbosity_ > normal)
	if (fname)
	    cerr << "KPSE found file " << fname << '\n';
	else
	    cerr << "KPSE can't find file " << fontname
		 << " res=" << resolution << '\n';

    return fname;
}

void kpathsea::verbosity (const verbosities level)
{
    verbosity_ = level;
    if (level > debug)
	kpathsea_debug = ~0;	// all debugging
    else
	kpathsea_debug = 0;	// none
}



#endif // if ENABLE_KPATHSEA
