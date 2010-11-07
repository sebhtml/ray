AC_INIT(Ray,0.1.0)

if test x"${CXXFLAGS+set}" = xset; then
  # the user set CXXFLAGS; don't override it.
  flags_were_set=true
else
  flags_were_set=false
  CXXFLAGS=""
fi 

AC_LANG([C++])
AC_PROG_CXX(mpic++)
AC_CHECK_HEADERS(iostream)
AC_CHECK_HEADERS(vector)
AC_CHECK_HEADERS(map)
AC_CHECK_HEADERS(zlib.h)
AC_CONFIG_HEADERS([code/config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([code/Makefile])
AM_INIT_AUTOMAKE
AC_OUTPUT
