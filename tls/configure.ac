#!/bin/bash -norc
dnl	This file is an input file used by the GNU "autoconf" program to
dnl	generate the file "configure", which is run during Tcl installation
dnl	to configure the system for the local environment.
dnl 
dnl This file contains code to generate "tls" using either the
dnl OpenSSL libraries or libraries from the commercial BSAFE SSL-C
dnl product from RSA Security.  In the United States, it is necessary
dnl to use the RSA BSAFE libraries for any product developed for
dnl commercial use. Licensing information for BSAFE SSL-C may be
dnl obtained from RSA Data Scurity Inc., San Mateo, California, USA.
dnl Their home page on the web is "www.rsasecurity.com". 

#-----------------------------------------------------------------------
# Sample configure.ac for Tcl Extensions.  The only places you should
# need to modify this file are marked by the string __CHANGE__
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# __CHANGE__
# Set your package name and version numbers here.
#
# This initializes the environment with PACKAGE_NAME and PACKAGE_VERSION
# set as provided.  These will also be added as -D defs in your Makefile
# so you can encode the package version directly into the source files.
# This will also define a special symbol for Windows (BUILD_<PACKAGE_NAME>
# so that we create the export library with the dll.
#-----------------------------------------------------------------------

AC_INIT([tls], [1.6.7])

#--------------------------------------------------------------------
# Call TEA_INIT as the first TEA_ macro to set up initial vars.
# This will define a ${TEA_PLATFORM} variable == "unix" or "windows"
# as well as PKG_LIB_FILE and PKG_STUB_LIB_FILE.
#--------------------------------------------------------------------

TEA_INIT([3.10])

AC_CONFIG_AUX_DIR(tclconfig)

#--------------------------------------------------------------------
# Load the tclConfig.sh file
#--------------------------------------------------------------------

TEA_PATH_TCLCONFIG
TEA_LOAD_TCLCONFIG

#--------------------------------------------------------------------
# Load the tkConfig.sh file if necessary (Tk extension)
#--------------------------------------------------------------------

#TEA_PATH_TKCONFIG
#TEA_LOAD_TKCONFIG

#-----------------------------------------------------------------------
# Handle the --prefix=... option by defaulting to what Tcl gave.
# Must be called after TEA_LOAD_TCLCONFIG and before TEA_SETUP_COMPILER.
#-----------------------------------------------------------------------

TEA_PREFIX

#-----------------------------------------------------------------------
# Standard compiler checks.
# This sets up CC by using the CC env var, or looks for gcc otherwise.
# This also calls AC_PROG_CC and a few others to create the basic setup
# necessary to compile executables.
#-----------------------------------------------------------------------

TEA_SETUP_COMPILER

#-----------------------------------------------------------------------
# __CHANGE__
# Specify the C source files to compile in TEA_ADD_SOURCES,
# public headers that need to be installed in TEA_ADD_HEADERS,
# stub library C source files to compile in TEA_ADD_STUB_SOURCES,
# and runtime Tcl library files in TEA_ADD_TCL_SOURCES.
# This defines PKG(_STUB)_SOURCES, PKG(_STUB)_OBJECTS, PKG_HEADERS
# and PKG_TCL_SOURCES.
#-----------------------------------------------------------------------

TEA_ADD_SOURCES([tls.c tlsIO.c tlsBIO.c tlsX509.c fixstrtod.c])
TEA_ADD_HEADERS([tls.h])
TEA_ADD_INCLUDES([])
TEA_ADD_LIBS([])
TEA_ADD_CFLAGS([])
TEA_ADD_STUB_SOURCES([])
TEA_ADD_TCL_SOURCES([tls.tcl])

#--------------------------------------------------------------------
# __CHANGE__
#
# You can add more files to clean if your extension creates any extra
# files by extending CLEANFILES.
# Add pkgIndex.tcl if it is generated in the Makefile instead of ./configure
# and change Makefile.in to move it from CONFIG_CLEAN_FILES to BINARIES var.
#
# A few miscellaneous platform-specific items:
# TEA_ADD_* any platform specific compiler/build info here.
#--------------------------------------------------------------------

#CLEANFILES="$CLEANFILES pkgIndex.tcl"
if test "${TEA_PLATFORM}" = "windows" ; then
    # Ensure no empty if clauses
    :
    #TEA_ADD_SOURCES([win/winFile.c])
    #TEA_ADD_INCLUDES([-I\"$(${CYGPATH} ${srcdir}/win)\"])
    #    AC_DEFINE(BUILD_tls)
    #    AC_DEFINE(WINDOWS)
else
    # Ensure no empty else clauses
    :
    #TEA_ADD_SOURCES([unix/unixFile.c])
    #TEA_ADD_LIBS([-lsuperfly])
fi

#--------------------------------------------------------------------
# __CHANGE__
# Choose which headers you need.  Extension authors should try very
# hard to only rely on the Tcl public header files.  Internal headers
# contain private data structures and are subject to change without
# notice.
# This MUST be called after TEA_LOAD_TCLCONFIG / TEA_LOAD_TKCONFIG
#--------------------------------------------------------------------

TEA_PUBLIC_TCL_HEADERS
#TEA_PRIVATE_TCL_HEADERS

#TEA_PUBLIC_TK_HEADERS
#TEA_PRIVATE_TK_HEADERS
#TEA_PATH_X

#--------------------------------------------------------------------
# Check whether --enable-threads or --disable-threads was given.
# This auto-enables if Tcl was compiled threaded.
#--------------------------------------------------------------------

TEA_ENABLE_THREADS

#--------------------------------------------------------------------
# The statement below defines a collection of symbols related to
# building as a shared library instead of a static library.
#--------------------------------------------------------------------

TEA_ENABLE_SHARED

#--------------------------------------------------------------------
# This macro figures out what flags to use with the compiler/linker
# when building shared/static debug/optimized objects.  This information
# can be taken from the tclConfig.sh file, but this figures it all out.
#--------------------------------------------------------------------

TEA_CONFIG_CFLAGS

#--------------------------------------------------------------------
# Set the default compiler switches based on the --enable-symbols option.
#--------------------------------------------------------------------

TEA_ENABLE_SYMBOLS

#--------------------------------------------------------------------
# If the variable OPENSSL is set, we will build with the OpenSSL
# libraries.  If it is not set, then we will use RSA BSAFE SSL-C
# libraries instead of the default OpenSSL libaries.
#--------------------------------------------------------------------

OPENSSL="1"

TLS_CHECK_SSL

#--------------------------------------------------------------------
# Determine if we should use the patented encryption code
#--------------------------------------------------------------------

AC_ARG_ENABLE(patents, [  --enable-patents        Use patented code.  Default is enabled], PATENTS=${enableval}, PATENTS=yes)  

if test ${PATENTS} = no; then
    AC_DEFINE([NO_PATENTS])
fi

#--------------------------------------------------------------------
# This macro generates a line to use when building a library.  It
# depends on values set by the TEA_ENABLE_SHARED, TEA_ENABLE_SYMBOLS,
# and TEA_LOAD_TCLCONFIG macros above.
#--------------------------------------------------------------------

TEA_MAKE_LIB

#--------------------------------------------------------------------
# Shared libraries and static libraries have different names.
# Also, windows libraries and unix libraries have different names.
# For the OpenSSL version, I chose to use the same library names that
# OpenSSL uses as its default names.
#--------------------------------------------------------------------

if test "${TEA_PLATFORM}" = "windows" ; then
    if test "$GCC" = "yes"; then
	TEA_ADD_LIBS([-L${SSL_LIB_DIR_NATIVE}])
    else
	TEA_ADD_LIBS([-libpath:${SSL_LIB_DIR_NATIVE}])
    fi
    if test -n "${OPENSSL}"; then
        TEA_ADD_LIBS([ssleay32.lib libeay32.lib])
    else
        TEA_ADD_LIBS([sslc32.lib])
    fi
else
    # Subst runtime dir here, use -R and -L where necessary. [Bug 1742859]
    LIB_RUNTIME_DIR=${SSL_LIB_DIR}
    eval "LD_SEARCH_FLAGS=\"${LD_SEARCH_FLAGS}\""
    if test -n "${OPENSSL}"; then
	TEA_ADD_LIBS([${LD_SEARCH_FLAGS} -L${SSL_LIB_DIR} -lssl -lcrypto ${GCCPATH} ${GCCLIB}])
    else
	TEA_ADD_LIBS([${LD_SEARCH_FLAGS} -L${SSL_LIB_DIR} -lsslc])
    fi
fi

#--------------------------------------------------------------------
# Determine the name of the tclsh and/or wish executables in the
# Tcl and Tk build directories or the location they were installed
# into. These paths are used to support running test cases only,
# the Makefile should not be making use of these paths to generate
# a pkgIndex.tcl file or anything else at extension build time.
#--------------------------------------------------------------------

TEA_PROG_TCLSH
#TEA_PROG_WISH

#--------------------------------------------------------------------
# These are for tlsConfig.sh
#--------------------------------------------------------------------

TEA_EXPORT_CONFIG([tls])

#--------------------------------------------------------------------
# Finally, substitute all of the various values into the Makefile.
# You may alternatively have a special pkgIndex.tcl.in or other files
# which require substituting th AC variables in.  Include these here.
#--------------------------------------------------------------------

AC_OUTPUT([Makefile pkgIndex.tcl tlsConfig.sh])
