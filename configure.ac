dnl Process this file with autoconf to produce a configure script

# Initialize autotools
AC_PREREQ(2.61)
AC_INIT([vktor], [0.1], [shahar@prematureoptimization.org])
AC_CONFIG_SRCDIR([lib/vktor.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([string.h stdarg.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_CHECK_FUNCS([strerror])

# Debug build convenience option
dnl enable various debugging options 
AC_ARG_ENABLE([debug], 
  [AS_HELP_STRING([--enable-debug], 
    [Enable various debugging build options @<:@default=no@:>@])],
  [
    if test "x$enableval" = "xno"; then
      enable_debug="no"
	else
	  enable_debug="yes"
	fi
  ],
  [
  	enable_debug="no"
  ])  
AM_CONDITIONAL(ENABLE_DEBUG, test "x$enable_debug" = "xyes")  
if test "x$enable_debug" = "xyes"; then
  AC_DEFINE_UNQUOTED(ENABLE_DEBUG, [1], [Enable various debugging options])
  VKTOR_CFLAGS+="-Wall -g"
else 
  AC_DEFINE_UNQUOTED(NODEBUG, [1], [Disable assertions and other debugging options])
fi

AC_SUBST(VKTOR_CFLAGS)

# Initialize doxygen support
DX_DOXYGEN_FEATURE(ON)
DX_DOT_FEATURE(HIDDEN)
DX_MAN_FEATURE(ON)
DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(HIDDEN)
DX_RTF_FEATURE(HIDDEN)
DX_XML_FEATURE(HIDDEN)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(HIDDEN)
DX_INIT_DOXYGEN($PACKAGE_NAME, [doxygen.cfg], [docs])

AC_CONFIG_FILES([Makefile 
                 lib/Makefile
				 test/Makefile])

AC_CONFIG_COMMANDS([default],[[ echo timestamp > stamp-h ]],[[]])

AC_OUTPUT

echo "
  $PACKAGE_NAME version $PACKAGE_VERSION
  Prefix.........: $prefix
  Debug Build....: $enable_debug
  C Compiler.....: $CC $CFLAGS $VKTOR_CFLAGS $CPPFLAGS
  Linker.........: $LD $LDFLAGS $LIBS
  Doxygen........: ${DX_DOXYGEN:-NONE}
"
