AC_PREREQ(2.59)

dnl Sébastien Boisvert
dnl autoconf script for Ray

AC_INIT(Ray,1.0.0)

if test x"${CXXFLAGS+set}" = xset; then
  # the user set CXXFLAGS; don't override it.
  flags_were_set=true
else
  flags_were_set=false
  CXXFLAGS=""
fi 

AC_PROG_RANLIB

AC_LANG([C++])
AC_PROG_CXX(mpic++)
AC_CHECK_HEADERS(iostream)
AC_CHECK_HEADERS(string)
AC_CHECK_HEADERS(vector)
AC_CHECK_HEADERS(map)
AC_CHECK_HEADERS(fstream)
AC_CHECK_HEADERS(sstream)
AC_CHECK_HEADERS(set)
AC_CHECK_HEADERS(algorithm)
AC_CHECK_HEADERS(cstring)
AC_CHECK_HEADERS(cstdlib)
AC_CHECK_HEADERS(stack)
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T



AC_CHECK_HEADERS(sys/time.h)
AC_CHECK_HEADERS(time.h)
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(stdint.h)
AC_CHECK_HEADERS(mpi.h)
AC_CHECK_HEADERS(string.h)
AC_CHECK_HEADERS(stdio.h)
AC_CHECK_HEADERS(stdlib.h)
AC_CHECK_HEADERS(assert.h)

AC_CHECK_LIB(z, inflate,
        [AC_CHECK_HEADERS([zlib.h],
         AM_CONDITIONAL(HAVE_ZLIB, true)
         AC_DEFINE(HAVE_ZLIB,1,[Have zlib]),
         AM_CONDITIONAL(HAVE_ZLIB, false))],
         AM_CONDITIONAL(HAVE_ZLIB, false))

AC_CHECK_LIB(bz2, BZ2_decompress,
        [AC_CHECK_HEADERS([bzlib.h],
          AM_CONDITIONAL(HAVE_BZ2, true)
          AC_DEFINE(HAVE_LIBBZ2,1,[Have libbz2]),
         AM_CONDITIONAL(HAVE_BZ2, false))],
         AM_CONDITIONAL(HAVE_BZ2, false))

AC_CONFIG_HEADERS([code/config.h])
AC_CONFIG_FILES([Makefile
		code/Makefile])
AM_INIT_AUTOMAKE
AC_OUTPUT
