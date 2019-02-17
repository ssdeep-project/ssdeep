AC_INIT([ssdeep],[2.14.2],[floss_ssdeep@irq.a4lg.com])
AM_INIT_AUTOMAKE

AC_CONFIG_FILES([Makefile])
AM_CONFIG_HEADER([config.h])

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CXX
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AC_PROG_INSTALL

AC_CONFIG_MACRO_DIR([m4])

case $host in
  *-*-*linux*-*) AC_DEFINE([__LINUX__],1,[Linux operating system functions]) ;;
  *-*-mingw32)   CPPFLAGS="-DUNICODE -D_UNICODE $CPPFLAGS"
esac


# Bring additional directories where things might be found into our
# search path. I don't know why autoconf doesn't do this by default
AC_ARG_ENABLE([auto-search],
[AS_HELP_STRING([--disable-auto-search], [disable searching additional include and library directories])],,
[enable_auto_search=yes])
with_auto_headers_default=$enable_auto_search
with_auto_libs_default=$enable_auto_search
AC_ARG_WITH([auto-headers],
[AS_HELP_STRING([--without-auto-headers], [disable searching additional include directories])],,
[with_auto_headers=$with_auto_headers_default])
AC_ARG_WITH([auto-libs],
[AS_HELP_STRING([--without-auto-libs], [disable searching additional library directories])],,
[with_auto_libs=$with_auto_libs_default])
for spfx in /usr/local /opt/local /sw ; do
    if test "x$with_auto_headers" = xyes; then
        AC_MSG_CHECKING([for ${spfx}/include])
        if test -d ${spfx}/include; then
            CPPFLAGS="-I${spfx}/include $CPPFLAGS"
            AC_MSG_RESULT([yes])
        else
            AC_MSG_RESULT([no])
        fi
    fi
    if test "x$with_auto_libs" = xyes; then
        AC_MSG_CHECKING([for ${spfx}/lib])
        if test -d ${spfx}/lib; then
            LDFLAGS="-L${spfx}/lib $LDFLAGS"
            AC_MSG_RESULT([yes])
        else
            AC_MSG_RESULT([no])
        fi
    fi
done

AC_C_BIGENDIAN
AC_SYS_LARGEFILE

AC_CHECK_HEADERS([libgen.h])
AC_CHECK_HEADERS([dirent.h])
AC_CHECK_HEADERS([inttypes.h])

AC_CHECK_HEADERS([fcntl.h sys/types.h sys/ioctl.h sys/param.h wchar.h unistd.h sys/stat.h sys/disk.h])

AC_CHECK_HEADER([inttypes.h],,AC_MSG_ERROR([You must have inttypes.h or some other C99 equivalent]),)

# Bit-parallel string processing
AC_ARG_ENABLE([bitparallel-string-ops],
[AS_HELP_STRING([--disable-bitparallel-string-ops], [disable using bit-parallel string operations (which would work best at 64-bit environment)])],,
[enable_bitparallel_string_ops=yes])
AS_IF([test "x$enable_bitparallel_string_ops" = xno],[
AC_DEFINE([SSDEEP_DISABLE_POSITION_ARRAY], [1], [Define to 1 if the user chose to disable bit-parallel string operations.])
],)

# These includes are required on FreeBSD
AC_CHECK_HEADERS([sys/mount.h],[],[],
[#ifdef HAVE_SYS_TYPES_H
 # include <sys/types.h>
 #endif
 #ifdef HAVE_SYS_PARAM_H
 # include <sys/param.h>
 #endif])

AC_FUNC_FSEEKO

AC_OUTPUT


