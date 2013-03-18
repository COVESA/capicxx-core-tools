AC_PREREQ(2.61)
AC_INIT([GENIVI Common API Ping Tool],
        [1.0],
        [],
        [CommonAPI-Ping])

AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([build-aux/config.h])
AC_CONFIG_FILES([Makefile])

AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE([foreign 1.11 silent-rules subdir-objects color-tests -Wall -Werror])
AM_SILENT_RULES
AM_MAINTAINER_MODE

AC_PROG_CXX
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11([noext])

LT_PREREQ(2.2)
LT_INIT

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(COMMONAPI_DBUS, [CommonAPI-DBus >= 2.0])

AC_MSG_RESULT([
        $PACKAGE_NAME v$VERSION

        COMMONAPI_DBUS_CFLAGS:  ${COMMONAPI_DBUS_CFLAGS}
        COMMONAPI_DBUS_LIBS:    ${COMMONAPI_DBUS_LIBS}

        prefix:            ${prefix}
        CXXFLAGS:          ${CXXFLAGS}
        LDFLAGS:           ${LDFLAGS}
])

AC_OUTPUT