// Part of dvi2bitmap.
// Copyright 1999, 2000, 2001, Council for the Central Laboratory of the Research Councils.
// See file LICENCE for conditions.
//
// Various utility functions
//
// $Id$

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>

#if HAVE_CSTD_INCLUDE
#include <cstdio>
#include <cctype>
#else
#include <stdio.h>
#include <ctype.h>
#endif

#if HAVE_STD_NAMESPACE
using std::ostream;
using std::ends;
using std::endl;
using std::cerr;
#endif

#include "Util.h"
#include "stringstream.h"

namespace Util
{
    verbosities verbosity_ = normal;
}


// Open a pipe with the given command, and read from it until EOF.
// Return this output as a single string.  Throw DviError on errors.
// If there's no way of doing this on a particular platform, throw DviError.
string Util::runCommandPipe (string cmd)
    throw (DviError)
{
    string response;

#if HAVE_POPEN
    FILE *PIN = popen (cmd.c_str(), "r");

    if (PIN == NULL)
	throw DviError ("Can't open pipe");

    SSTREAM resp;

    int ich;
    bool firstline = true;
    while ((ich = fgetc (PIN)) != EOF)
    {
	if (firstline)
	{
	    const char ch = static_cast<char>(ich);
	    if (ch == '\n' || ch == '\r') // end of line
		firstline = false;
	    else
		resp << ch;
	}
	// else just keep reading to the end of the file
    }

    int exitval = pclose(PIN);
    if (exitval == -1)
    {
	// Couldn't exit cleanly
	if (verbosity_ >= normal)
	    cerr << "runCommandPipe: command <" << cmd
		 << "> didn't exit cleanly" << endl;
    }
    else if ((exitval & 0xff) == 0)
	// Get wait4(2) exit status -- low-order 8 bits
	response = SS_STRING(resp);
    else
    {
	if (verbosity_ >= normal)
	    cerr << "runCommandPipe: command <"
		 << cmd << "> apparently failed (" << exitval << ')'
		 << endl;
	response = "";
    }

#else

    // No popen-line function available.  We probably shouldn't have got here.
    throw DviError ("Can't run subcommands on this platform");

#endif /* HAVE_POPEN */

    return response;
}

void Util::verbosity (const verbosities level)
{
    verbosity_ = level;

    return;
}