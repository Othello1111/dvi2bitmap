// Part of dvi2bitmap.
// Copyright 1999, 2000 Council for the Central Laboratory of the Research Councils.
// See file LICENCE for conditions.
//
// $Id$

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dvi2bitmap.h"
#include "InputByteStream.h"

#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>		// for strerror
#if NO_CSTD_INCLUDE
#include <stdio.h>
#include <errno.h>
#else
#include <cstdio>
#include <cerrno>
using std::sprintf;
#endif

// Static debug switch
verbosities InputByteStream::verbosity_ = normal;

// Open the requested file.  If preload is true, then open the file and
// read it entire into memory, since the client will be seeking a lot.
// If the file can't be opened, then try adding tryext to the end of
// it.  If this succeeds in opening the file, then modify the filename.
InputByteStream::InputByteStream (string& s, bool preload, string tryext)
    : eof_(true), preloaded_(preload)
{
    fname_ = s;
    fd_ = open (fname_.c_str(), O_RDONLY);
    if (fd_ < 0 && tryext.length() > 0)
    {
	fname_ += tryext;
	fd_ = open (fname_.c_str(), O_RDONLY);
	if (fd_ >= 0)
	    s = fname_;		// modify the input filename
    }
    if (fd_ < 0)
    {
	string errstr = strerror (errno);
	throw InputByteStreamError ("can\'t open file " + s + " to read ("
				    +errstr+")");
    }

    eof_ = false;

    struct stat S;
    if (fstat (fd_, &S))
    {
	string errstr = strerror (errno);
	throw InputByteStreamError ("Can't stat open file ("
				    +errstr+")");
    }
    filesize_ = S.st_size;

    if (preload)
    {
	buflen_ = filesize_;
	buf_ = new Byte[buflen_];
	size_t bufcontents = read (fd_, buf_, buflen_);
	if (bufcontents != buflen_)
	{
	    string errstr = strerror (errno);
	    throw InputByteStreamError ("Couldn't preload file "+fname_
					+" ("+errstr+")");
	}
	eob_ = buf_ + bufcontents;

	close (fd_);
	fd_ = -1;

	if (verbosity_ > normal)
	    cerr << "InputByteStream: preloaded " << fname_
		 << ", " << filesize_ << " bytes\n";
    }
    else
    {
	buflen_ = 512;
	buf_ = new Byte[buflen_];
	eob_ = buf_;	// nothing read in yet - eob at start

	if (verbosity_ > normal)
	    cerr << "InputByteStream: name=" << fname_
		 << " filesize=" << filesize_
		 << " buflen=" << buflen_ << '\n';
    }
    p_ = buf_;
}

InputByteStream::~InputByteStream ()
{
    if (fd_ >= 0)
	close (fd_);
    delete[] buf_;
}

Byte InputByteStream::getByte(int n)
{
    if (eof_)
	return 0;

    if (p_ < buf_)
	throw DviBug ("InputByteStream:"+fname_+": pointer before buffer start");
    if (p_ > eob_)
	throw DviBug ("InputByteStream:"+fname_+": pointer beyond EOF");

    if (p_ == eob_)
    {
	if (preloaded_)		// end of buffer means end of file
	    eof_ = true;
	else
	{
	    read_buf_();
	    p_ = buf_;
	}
    }
    Byte result = eof_ ? static_cast<Byte>(0) : *p_;
    p_ += n;
    return result;
}

const Byte *InputByteStream::getBlock (int pos, unsigned int length)
{
    if (eof_)
	return 0;

    Byte *blockp;

    if (pos < 0)		// retrieve last `length' bytes from
				// end of file
    {
	if (preloaded_)
	{
	    blockp = buf_ + buflen_ - length;
	    if (blockp < buf_)
		throw DviBug
		  ("InputByteStream::getBlock:"+fname_+": requested more than bufferful");
	}
	else
	{
	    if (length > buflen_)
		throw InputByteStreamError ("getBlock:"+fname_+": length too large");
	    if (lseek (fd_, filesize_-length, SEEK_SET) < 0)
	    {
		string errstr = strerror (errno);
		throw InputByteStreamError ("getBlock:"+fname_+
					    ": can\'t seek to EOF ("
					    +errstr+")");
	    }
	    read_buf_();
	    blockp = buf_;
	}
    }
    else
    {
	if (preloaded_)
	{
	    blockp = buf_ + pos;
	    if (blockp < buf_)
		throw DviBug
		    ("InputByteStream::getBlock:"+fname_+": pointer before buffer start");
	    if (blockp > eob_)
	    {
		char buf[100];
		sprintf (buf,
		      "InputByteStream::getBlock:%s: pointer beyond EOF (%d,%d)",
			 fname_.c_str(), pos, length);
		throw DviBug (buf);
	    }
	    if (blockp+length > eob_)
	    {
		char buf[100];
		sprintf (buf,
		"InputByteStream::getBlock:%s: requested block too large (%d,%d)",
			 fname_.c_str(), pos,length);
		throw DviBug (buf);
	    }
	}
	else
	    throw DviBug
		("InputByteStream::getBlock:"+fname_+": implemented only for preloaded");
    }

    return blockp;
}

bool InputByteStream::eof()
{
    return eof_;
}

void InputByteStream::seek (unsigned int pos)
{
    if (preloaded_)
    {
	if (pos > buflen_)	// pos unsigned, so can't be negative
	    throw DviBug ("InputByteStream::seek:"+fname_+": out of range");
	p_ = buf_ + pos;
    }
    else
    {
	if (lseek (fd_, pos, SEEK_SET) < 0)
	{
	    string errstr = strerror(errno);
	    throw DviBug ("InputByteStream::seek:"+fname_+": can\'t seek ("
			  +errstr+")");
	}
	read_buf_();
	p_ = buf_;
    }
}
int InputByteStream::pos ()
{
    if (!preloaded_)
	throw DviBug ("InputByteStream:"+fname_+": Can't get pos in non-preloaded file");
    return static_cast<int>(p_ - buf_);
}
void InputByteStream::skip (unsigned int skipsize)
{
    if (!preloaded_)
	throw DviBug ("InputByteStream:"+fname_+": Can't skip in non-preloaded file");
    p_ += skipsize;
}

void InputByteStream::read_buf_ ()
{
    ssize_t bufcontents = read (fd_, buf_, buflen_);
    if (bufcontents < 0)
    {
	string errmsg = strerror(errno);
	throw DviBug ("InputByteStream::read_buf_:"+fname_+": read error ("
		      +errmsg+")");
    }
    eof_ = (bufcontents == 0);
    eob_ = buf_ + bufcontents;
}
/*
#if (sizeof(unsigned int) != 4)
// The code here is intended to deal with the case where (un)signed
// ints are 4 bytes long.  It's actually simpler on machines where
// ints are longer, because we wouldn't have to do the two-stage
// subtraction (below) when n==4.  I can't test this, however, so
// simply cause an error here.
#error "InputByteStream.cc assumes sizeof(unsigned int)==4"
#endif
*/

// Assumes: DVI file is big-endian - ie, MSB first (I can't find this
// explicitly stated in the spec); DVI integers are 2-complement
// (explicit in the spec).
// I _believe_ that the following code is independent of the host
// machine's bytesex and integer representation (2's-complement or
// not), but....
signed int InputByteStream::getSIS(int n)
{
    if (n<0 || n>4)
	throw DviBug("InputByteStream:"+fname_+": bad argument to getSIS");
    unsigned int t = getByte();
    unsigned int pow2 = 128;	// 2^7-1   is largest one-byte signed int
				// 2^7=128 is most negative signed int
				// 2^8-1   is -1
    for (int i=n-1; i>0; i--)
    {
	pow2 *= 256;
	t *= 256;
	t += getByte();
    }
    signed int result;
    if (t < pow2)
	result = t;
    else
	// pow2 <= t < 2*pow2
	// t is actually a negative number - subtract off 2*pow2
	if (n < 4)
	{
	    // t and 2*pow2 will both fit into a signed int
	    // so explicitly convert them to that _first_.
	    // I'm not sure if the result of subtracting an unsigned from 
	    // a smaller unsigned (ie, whether it's a signed negative number,
	    // or, say, zero) is implementation-dependent or not.
	    result = static_cast<signed int>(t)
		- static_cast<signed int>(2*pow2);
	    //result -= 2*pow2;
	}
	else
	{
	    // n==4: t won't fit into a signed int, and 2*pow2 would
	    // overflow, so do the subtraction in two stages.  A
	    // careless optimiser could mess this up.
	    t -= pow2;		// now it'll fit
	    // 0 <= t < pow2
	    result = pow2 - t;	// a positive result in a signed variable
	    result = -result;
	}
    return result;
}

signed int InputByteStream::getSIU(int n)
{
    // disallow n==4 - there are no unsigned 4-byte quantities in the DVI file
    if (n<0 || n>3)
	throw DviBug("InputByteStream:"+fname_+": bad argument to getSIU");
    unsigned int t = 0;
    for (; n>0; n--)
    {
	t *= 256;
	t += getByte();
    }
    return static_cast<signed int>(t);
}

unsigned int InputByteStream::getUIU(int n)
{
    if (n<0 || n>4)
	throw DviBug("InputByteStream:"+fname_+": bad argument to getUIU");
    unsigned int t = 0;
    for (; n>0; n--)
    {
	t *= 256;
	t += getByte();
    }
    return t;
}

unsigned int InputByteStream::getUIU(int n, const Byte *p)
{
    if (n<0 || n>4)
	throw DviBug("InputByteStream: bad argument to getUIU(int,Byte*)");
    unsigned int t = 0;
    for (const Byte *b=p; n>0; n--, b++)
	t = t*256 + static_cast<unsigned int>(*b);
    return t;
}

