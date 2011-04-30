AC_PREREQ(2.59)
AC_INIT(Ray,1.3.1, denovoassembler-users@lists.sourceforge.net, ray, http://denovoassembler.sf.net)

AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR(code/MessageProcessor.cpp)
AC_CONFIG_HEADER([config.h])

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

AC_CHECK_HEADERS(sys/time.h)
AC_CHECK_HEADERS(time.h)
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(stdint.h)
AC_CHECK_HEADERS(mpi.h)
AC_CHECK_HEADERS(string.h)
AC_CHECK_HEADERS(stdio.h)
AC_CHECK_HEADERS(stdlib.h)
AC_CHECK_HEADERS(assert.h)
AC_CHECK_HEADERS(stdio.h)
AC_CHECK_HEADERS(stdlib.h)
AC_CHECK_HEADERS(sys/types.h)
AC_CHECK_HEADERS(sys/stat.h)
AC_CHECK_HEADERS(fcntl.h)
AC_CHECK_HEADERS(sys/mman.h)

AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

AC_CHECK_LIB(z, inflate,
        [AC_CHECK_HEADERS([zlib.h],
         AM_CONDITIONAL(HAVE_ZLIB, true)
         AC_DEFINE(HAVE_ZLIB,1,[Have zlib]),
         AM_CONDITIONAL(HAVE_ZLIB, false))],
         AM_CONDITIONAL(HAVE_ZLIB, false))

AC_CHECK_LIB(bz2, BZ2_decompress,
        [AC_CHECK_HEADERS([bzlib.h],
          AM_CONDITIONAL(HAVE_LIBBZ2, true)
          AC_DEFINE(HAVE_LIBBZ2,1,[Have libbz2]),
         AM_CONDITIONAL(HAVE_LIBBZ2, false))],
         AM_CONDITIONAL(HAVE_LIBBZ2, false))

AC_CHECK_LIB(rt, clock_gettime,
        [AC_CHECK_HEADERS([time.h],
          AM_CONDITIONAL(HAVE_CLOCK_GETTIME, true)
          AC_DEFINE(HAVE_CLOCK_GETTIME,1,[Have rt]),
         AM_CONDITIONAL(HAVE_CLOCK_GETTIME, false))],
         AM_CONDITIONAL(HAVE_CLOCK_GETTIME, false))


AC_CONFIG_FILES([code/Makefile
Makefile])

AC_OUTPUT
