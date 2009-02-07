
#  This is a Bourne shell script.  In order to be POSIX-compliant, the
#  first line must be blank.

#+
#  Name:
#     mk
#
#  Version:
#     Version for Mk Va library makefile.
#
#  Purpose:
#     Invoke make to build and install the SGML package.
#
#  Type of Module:
#     Shell script.
#
#  Description:
#     This script should normally be used to invoke the make utility
#     to build and install the dvi2bitmap package and to perform other
#     housekeeping tasks.  It invokes the make utility after first
#     defining appropriate environment variables and macros for the
#     computer system in use.  This file also serves to document the
#     systems on which the package is implemented.
#
#     Note that the Makefile which accompanies this mk script is called
#     Makefile.starlink -- it DOES NOT use the standard name `makefile'.
#
#  Invocation:
#     The user of this script should normally first define the SYSTEM
#     environment variable to identify the host computer system (see
#     the "Supported Systems" section).  This script should then be used
#     in the same way as the make utility would be used.  For instance,
#     to build, install and test dvi2bitmap, you might use the following
#     commands:
#
#        ./mk build
#        ./mk install
#        ./mk test
#        ./mk clean
#
#  Supported Systems:
#     The following systems are currently supported and may be
#     identified by defining the SYSTEM environment variable
#     appropriately before invoking this script:
#
#        alpha_OSF1
#           DEC Alpha machines running OSF1
#
#        sun4_Solaris
#           SUN Sparcstations running SunOS 5.x (Solaris)
#
#        ix86_Linux
#           Intel PC running Linux
#
#     In addition, the following systems have been supported at some
#     time in the past.  This script contains the macro definitions 
#     needed for these systems, but it is not known whether Star2HTML
#     will still build or run on these systems:
#
#        mips
#           DECstations running Ultrix
#
#        sun4
#           SUN Sparcstations running SunOS 4.x
#
#     This script will exit without action if the SYSTEM environment
#     variable is not defined.  A warning will be issued if it is
#     defined but is not set to one of the values above.  In this case,
#     no additional environment variables will be defined by this
#     script (any that are pre-defined will be passed on to make
#     unaltered).
#
#  Targets:
#     For details of what targets are provided, see the associated
#     makefile.  The latter will normally provide a default target
#     called "help", which outputs a message describing this script
#     and lists the targets provided.
#
#  Notes on Porting:
#     If your machine or system setup does not appear in this script,
#     then it should be possible to build and install the package by
#     adding a new case to this script with appropriate definitions
#     (probably based on one of the existing implementations).
#
#  make Macros:
#     The following "global" make macros are used in the associated
#     makefile and may be changed by means of appropriate environment
#     variable definitions (in each case the default is shown in
#     parentheses).  Note that these macros are provided to allow
#     external control over the directories in which software is
#     installed, etc., so they should not normally be re-defined within
#     this script.
#
#        STARLINK (/star)
#           Pathname of the root directory beneath which Starlink
#           software is currently installed.  This indicates to
#           the package where to find other Starlink software (include
#           files, libraries, etc.) which it uses.
#
#        PERL (/usr/local/bin/perl)
#           Pathname for Perl 
#
#        INSTALL ($HOME)
#           Pathname of the root directory beneath which the package will
#           be installed for use.  Your home directory will be used by
#           default.  This macro is provided to allow the package to be
#           installed locally for personal use (e.g. during development
#           or testing).  It should be set to the $STARLINK directory if
#           you want to add the package into an already installed set of
#           Starlink software.  You should ensure that the appropriate
#           sub-directories appear on any relevant search paths which
#           your system uses for locating software (e.g. binaries and
#           libraries).
#
#        EXPORT (.)
#           Pathname of the directory into which compressed tar files
#           will be written if the "export" or "export_source" make
#           targets are used to produce an exportable copy of Star2HTML
#           or its source files.  The current working directory (i.e.
#           the source distribution directory) will be used by default.
#
#     The following "local" make macros are used in the associated
#     makefile and should normally be overridden by environment variable
#     definitions within this script.  All the local macros that are
#     used in building a package should overridden even when the value
#     is the same as the default.  This documents which macros are used
#     and ensures that the package will continue to build correctly even
#     if the default values are changed.  Macros that are not used on a
#     particular machine (e.g. BLD_SHR on DECstations) should not be
#     overridden.  In each case the default is shown in parentheses.
#
#        AR_IN (ar -r)
#           The command to use to insert an object (.o) file into an
#           archive (.a) library.  On some systems the variation 'ar r'
#           may be required instead.
#
#        BLD_SHR (:)
#           Command to build a shareable library when given three
#           arguments specifying (1) the name of the library file to be
#           built (2) a list of the object files to be used in the
#           library and (3) a list of any additional libraries against
#           which to link.  By default, it is assumed that shareable
#           libraries are not available, and the default acts as a null
#           command.
#
#        CC (c89)
#           The C compiler command to use.
#
#        CFLAGS (-O)
#           The C compiler options to use.
#
#        FC (fort77)
#           The Fortran compiler command to use.  The package does not
#           require a Fortran compiler at present.
#
#        FFLAGS (-O)
#           The Fortran compiler options to be used.
#
#        LINK (ln)
#           The command required to establish a link to a file.  The
#           default assumes POSIX.2 behavior, which only provides a
#           "hard" link operating within a single file system.  If the
#           host operating system allows "symbolic" links, then this
#           macro might be re-defined as 'ln -s'.  Alternatively, if the
#           use of multiple file systems is essential but not supported
#           by any form of link, then a copy command could be
#           substituted (e.g. 'cp -p'), at some cost in file space.
#
#        SOURCE_VARIANT ($SYSTEM)
#           The name used to distinguish between platform-specific
#           files.  Using the default ($SYSTEM) implies that multiple
#           copies of the same source may exist under different names.
#           For example, the files sub1.f_sun4 and sun1.f_sun4_Solaris
#           may contain identical source code.  If this is to be
#           avoided (to save space), edit this script to set
#           SOURCE_VARIANT to a suitable string - for example setting it
#           to "sun" would allow one copy of the source called
#           sub1.f_sun to be used. To do this, add to each of the
#           per-system sections a statement of the form:
#
#              SOURCE_VARIANT='string'
#
#        RANLIB (:)
#           The command required to "randomise" an object library.  By
#           default, this operation is not performed (the default acts
#           as a null command).  On systems which require it, this
#           should typically be set to 'ranlib'.
#
#        SHARE (.so)
#           The file type suffix to be applied to produce the name of a
#           shareable library file.  By default, the ".so" suffix is
#           applied without a library version number.  For systems which
#           support version numbers on shareable libraries, the macro
#           LIB_VERS is defined within the associated makefile and may
#           be used as part of a definition such as '.so.$(LIB_VERS)'.
#
#        TAR_IN (pax -w -v -x ustar -f)
#           Command to use to insert a file into a .tar archive file.
#           The default uses the POSIX.2 pax command, which is not
#           available on traditional UNIX systems.  These typically use
#           a tar command such as 'tar -cvhf' instead (if symbolic
#           links are supported, then an option to follow these must be
#           included in this command).
#
#        TAR_OUT (pax -r -f)
#           Command to use to extract a file from a .tar archive file.
#           The default uses the POSIX.2 pax command, which is not
#           available on traditional UNIX systems.  These typically use
#           a tar command such as 'tar -xf' instead.
#
#  Implementation Deficiencies:
#     -  The implementation of shareable libraries on the alpha_OSF1
#        system is still preliminary.
#
#  Copyright:
#     Copyright (C) 1995 Rutherford Appleton Laboratory
#
#  Authors:
#     RFWS: R.F.Warren-Smith (Starlink, RAL)
#     PMA: P.M.Allan (Starlink, RAL)
#     PTW: P.T.Wallace (Starlink, RAL)
#     BLY: M.J.Bly (Starlink, RAL)
#     AJC: A.J.Chipperfield (Starlink, RAL)
#     NG: Norman Gray (Starlink, Glasgow)
#     {enter_new_authors_here}
#
#  History:
#     4-JAN-1993 (RFWS):
#        Original version.
#     15-MAR-1993 (RFWS):
#        Adapted for use on sun4_Solaris and alpha_OSF1.
#     30-JUN-1993 (PMA):
#        Added definitions of CC and CFLAGS for all machines.
#     13-OCT-1993 (PMA):
#        Added comments about definitions of CC and CFLAGS for all machines.
#     15-NOV-1993 (PTW):
#        Cosmetics.
#     2-MAY-1995 (BLY):
#        Modified RANLIB default to : (colon).
#        Modified BLD_SHR default to : (colon).
#     16-JUN-1995 (BLY):
#        Designated Mk IVb version.
#     21-JUN-1995 (AJC):
#        Remove references to /star in CFLAGS.
#     11-Sep-1995 (AJC):
#        Version 5a - Added Linux
#     29-SEP-1995 (AJC):
#        Modified for Star2HTML
#     12-NOV-1997 (AJC):
#        Set PERL /usr/bin/perl for Linux
#     02-MAY-1999 (NG):
#        Adapted from Star2HTML mk file.
#     {enter_further_changes_here}
#
#  Bugs:
#     {note_any_bugs_here}
#
#-

#  Export "local" definitions to the environment for use by make.
      export PERL
      export AR_IN
      export BLD_SHR
      export CC
      export CXX
      export CFLAGS
      export FC
      export FFLAGS
      export LINK
      export SOURCE_VARIANT
      export RANLIB
      export SHARE
      export TAR_IN
      export TAR_OUT
      export TAR_OUT_PIPE

#  Check that the SYSTEM environment variable is defined.
      if test "$SYSTEM" = ""; then
         echo "mk: Please define the environment variable SYSTEM to identify"
         echo "    your computer system (the prologue in the mk script file"
         echo "    contains more information if you require it)."

#  If OK, test for each recognised system.
      else
         case "$SYSTEM" in

#  DEC Alpha:
#  =========
#  DEC Alpha machines running OSF1.
#  -------------------------------
            alpha_OSF1)
               AR_IN='ar -r'
#               BLD_SHR=\
#'f() ld -shared -update_registry $(INSTALL)/share/so_locations -o $$1 $$2 $$3 \
#-lfor -lFutil -lUfor -lm -lots -lc; f'
               CC='cc'
	       CXX='cxx'
               CFLAGS='-I$(STAR_INC) -O -std1'
               FC='f77'
               FFLAGS='-O'
               LINK='ln -s'
               RANLIB='ranlib'
#               SHARE='.so'
               TAR_IN='tar -cvhf'
               TAR_OUT='tar -xf'
	       # -xBpf untested on this platform
               TAR_OUT_PIPE='tar -xBpf'
               echo "mk: Environment variables defined for $SYSTEM system"
               ;;

#  DECstations:
#  ===========
#  DECstations running Ultrix.
#  --------------------------
            mips)
               AR_IN='ar -r'
               CC='c89'
               CFLAGS='-I$(STAR_INC) -O'
               FC='f77'
               FFLAGS='-O'
               LINK='ln -s'
               RANLIB='ranlib'
               TAR_IN='tar -cvhf'
               TAR_OUT='tar -xf'
	       # -xBpf untested on this platform
               TAR_OUT_PIPE='tar -xBpf'
               echo "mk: Environment variables defined for $SYSTEM system"
               ;;

#  SUN4 systems:
#  ============
#  SUN Sparcstations running SunOS 4.x.
#  -----------------------------------
            sun4)
               AR_IN='ar r'
               BLD_SHR='f() ld -assert pure-text -o $$1 $$2; f'
               CC='gcc'
               CFLAGS='-I$(STAR_INC) -O -fPIC'
               FC='f77'
               FFLAGS='-O -PIC'
               LINK='ln -s'
               RANLIB='ranlib'
               SHARE='.so.$(LIB_VERS)'
               TAR_IN='tar -cvhf'
               TAR_OUT='tar -xf'
	       # -xBpf untested on this platform
               TAR_OUT_PIPE='tar -xBpf'
               echo "mk: Environment variables defined for $SYSTEM system"
               ;;

#  SUN Sparcstations running SunOS 5.x (Solaris).
#  ---------------------------------------------
            sun4_Solaris)
               AR_IN='ar -r'
               BLD_SHR='f() ld -G -z text -o $$1 $$2; f'
               CC='cc'
               CXX='CC'
               CFLAGS='-I$(STAR_INC) -O -K PIC'
               FC='f77'
               FFLAGS='-O -PIC'
               LINK='ln -s'
               SHARE='.so'
               TAR_IN='tar -cvhf'
               TAR_OUT='tar -xf'
               TAR_OUT_PIPE='tar -xBpf'
               echo "mk: Environment variables defined for $SYSTEM system"
               ;;

#  PC systems:
#  ==========
#  Intel PC running Linux.
#  ----------------------
            ix86_Linux)
               PERL='/usr/bin/perl'
               AR_IN='ar r'
	       CC='gcc'
               CXX='g++'
               CFLAGS='-O'
               FC='fort77'
	       FFLAGS='-O'
               LINK='ln -s'
               RANLIB='ranlib'
               TAR_IN='tar -chf'
               TAR_OUT='tar -xf'
               TAR_OUT_PIPE='tar -xBpf'
               echo "mk: Environment variables defined for $SYSTEM system"
               ;;

#  Issue a warning if SYSTEM is not recognised.
            *)
               SOURCE_VARIANT='unknown'
               echo "mk: WARNING: value of SYSTEM = $SYSTEM not recognised..."
               echo "             ...assuming default system characteristics"
               echo '             ...setting SOURCE_VARIANT to "unknown"'
               ;;
         esac

#  Invoke make with the appropriate environment variables set to override
#  default macros defined in the makefile.
         echo make -e -f Makefile.starlink $*
         make -e -f Makefile.starlink $*
      fi

#  End of script.
