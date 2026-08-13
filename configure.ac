AC_INIT([ssdeep],[2.14.2],[floss_ssdeep@irq.a4lg.com])
AC_PREREQ([2.71])
AM_INIT_AUTOMAKE

dnl Version info
PACKAGE_VERSION_PRERELEASE=true
PACKAGE_YEAR=2026
AC_SUBST([PACKAGE_YEAR])

dnl Parse version info
ssdeep_parse_version () {
    t=$(echo "$PACKAGE_VERSION" | sed 's/^\([0-9]\+\(\.[0-9]\+\)\+\).*/\1/')
    PACKAGE_VERSION_MAJOR=${t%%.*}
    t=${t#*.}
    PACKAGE_VERSION_MINOR=${t%%.*}
    PACKAGE_VERSION_REV=0
    PACKAGE_VERSION_SUBREV=0
    case "$t" in
    *.*)
        t=${t#*.}
        PACKAGE_VERSION_REV=${t%%.*}
        case "$t" in
        *.*)
            t=${t#*.}
            PACKAGE_VERSION_SUBREV=${t%%.*}
            ;;
        esac
        ;;
    esac
}
ssdeep_parse_version
PACKAGE_VERSION_WINRES=${PACKAGE_VERSION_MAJOR},${PACKAGE_VERSION_MINOR},${PACKAGE_VERSION_REV},${PACKAGE_VERSION_SUBREV}
PACKAGE_VERSION_WINRES_PREREL=''
AS_IF([$PACKAGE_VERSION_PRERELEASE], [PACKAGE_VERSION_WINRES_PREREL=' (prerelease)'])
PACKAGE_VERSION_MAN_PREREL=''
PACKAGE_FILEFLAG_WINRES_PREREL=0
AS_IF([$PACKAGE_VERSION_PRERELEASE], [
    PACKAGE_VERSION_MAN_PREREL=' \- TO BE DETERMINED'
dnl VS_FF_PRERELEASE == 2
    PACKAGE_FILEFLAG_WINRES_PREREL=2
])
AC_SUBST([PACKAGE_VERSION_WINRES])
AC_SUBST([PACKAGE_VERSION_WINRES_PREREL])
AC_SUBST([PACKAGE_VERSION_MAN_PREREL])
AC_SUBST([PACKAGE_FILEFLAG_WINRES_PREREL])

AC_CONFIG_FILES([Makefile ssdeep.1])
AC_CONFIG_HEADERS([config.h])

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX([23], [], [optional])
AX_CXX_COMPILE_STDCXX([20], [], [optional])
AX_CXX_COMPILE_STDCXX([11], [], [optional])

LT_INIT([win32-dll])
LT_LANG([Windows Resource])
AC_PROG_INSTALL
AC_CHECK_TOOL([WINDRES], [windres], [false])

AC_CONFIG_MACRO_DIR([m4])

dnl Detect OS and environment
ssdeep_os_windows=false
AS_CASE([$host],
[*-*-*linux*], [dnl
    AC_DEFINE([__LINUX__],1,[Linux operating system functions])
],
[*-*-mingw*], [dnl
    ssdeep_os_windows=true
    CPPFLAGS="-DUNICODE -D_UNICODE $CPPFLAGS"
])
AM_CONDITIONAL([WIN_WITH_WINDRES], [$ssdeep_os_windows && test "$WINDRES" != false])
AS_IF([$ssdeep_os_windows && test "$WINDRES" != false], [AC_CONFIG_FILES([fuzzy-win32res.rc ssdeep-win32res.rc])])

dnl --enable-libonly
AC_ARG_ENABLE([libonly],
[AS_HELP_STRING([--enable-libonly], [builds only libfuzzy (not ssdeep)])],,
[enable_libonly=no])
AM_CONDITIONAL([LIBONLY], [test "x$enable_libonly" = xyes])


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
AC_CHECK_HEADERS([stdbit.h])

AC_CHECK_HEADERS([fcntl.h sys/types.h sys/ioctl.h sys/param.h wchar.h unistd.h sys/stat.h sys/disk.h])

AC_CHECK_HEADER([inttypes.h],,AC_MSG_ERROR([You must have inttypes.h or some other C99 equivalent]),)

# Bit-parallel string processing
AC_ARG_ENABLE([bitparallel-string-ops],
[AS_HELP_STRING([--disable-bitparallel-string-ops], [disable using bit-parallel string operations (which would work best at 64-bit environment)])],,
[enable_bitparallel_string_ops=yes])
AS_IF([test "x$enable_bitparallel_string_ops" = xno],[
AC_DEFINE([FUZZY_DISABLE_POSITION_ARRAY], [1], [Define to 1 if the user chose to disable bit-parallel string operations.])
],)

# These includes are required on FreeBSD
AC_CHECK_HEADERS([sys/mount.h], [], [], [
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
])

AC_FUNC_FSEEKO

AC_OUTPUT
