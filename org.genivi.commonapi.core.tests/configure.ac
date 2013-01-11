AC_PREREQ(2.61)
AC_INIT([Common API C++ Tests],
        [1.0],
        [],
        [commonapi-tests])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([build-aux/config.h])
AC_CONFIG_FILES([Makefile])

AC_CANONICAL_SYSTEM

dnl silent-rules parallel-tests
AM_INIT_AUTOMAKE([foreign 1.11 subdir-objects -Wall -Werror])
dnl AM_SILENT_RULES
AM_MAINTAINER_MODE

AC_PROG_CXX
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11([noext])

LT_PREREQ(2.2)
LT_INIT

GMOCK_MIN_VERSION="1.6.0"
GMOCK_URL="http://code.google.com/p/googlemock"

AC_ARG_WITH([gmock],
            [AS_HELP_STRING([--with-gmock],
                            [Specifies how to find the gmock package. The gmock
                            built or installed at that prefix will be used.])],
            [],
            [with_gmock=yes])

AC_ARG_VAR([GMOCK_CONFIG], [The exact path of Google Mock's 'gmock-config' script.])
AC_ARG_VAR([GMOCK_CPPFLAGS], [C-like preprocessor flags for Google Mock.])
AC_ARG_VAR([GMOCK_CXXFLAGS], [C++ compile flags for Google Mock.])
AC_ARG_VAR([GMOCK_LDFLAGS], [Linker path and option flags for Google Mock.])
AC_ARG_VAR([GMOCK_LIBS], [Library linking flags for Google Mock.])
AC_ARG_VAR([GMOCK_VERSION], [The version of Google Mock available.])

AS_IF([test "x${with_gmock}" != "xyes"],
      [AS_IF([test -x "${with_gmock}/scripts/gmock-config"],
             [GMOCK_CONFIG="${with_gmock}/scripts/gmock-config"],
             [GMOCK_CONFIG="${with_gmock}/bin/gmock-config"])])

AS_IF([test -f "${GMOCK_CONFIG}"],
      [],
      [AC_MSG_ERROR([dnl
Unable to locate Google Mock, please provide the path to an installed version
by using the --with-gmock parameter or GMOCK_CONFIG environment variable.
Refer to ${GMOCK_URL} for more information.])])

GMOCK_CPPFLAGS=`${GMOCK_CONFIG} --cppflags`
GMOCK_CXXFLAGS=`${GMOCK_CONFIG} --cxxflags`
GMOCK_LDFLAGS=`${GMOCK_CONFIG} --ldflags`
GMOCK_LIBS=`${GMOCK_CONFIG} --libs`
GMOCK_VERSION=`${GMOCK_CONFIG} --version`

AS_IF([${GMOCK_CONFIG} --min-version=${GMOCK_MIN_VERSION}],
      [],
      [AC_MSG_ERROR([dnl
Found Google Mock Version ${GMOCK_VERSION}. However ${GMOCK_MIN_VERSION} is
required. Please refer to ${GMOCK_URL} for a more recent version.])])

PKG_CHECK_MODULES(COMMONAPI, [CommonAPI >= 0.7])

AC_MSG_RESULT([
        $PACKAGE_NAME v$VERSION

        GMOCK_CONFIG:       ${GMOCK_CONFIG}
        GMOCK_CPPFLAGS:     ${GMOCK_CPPFLAGS}
        GMOCK_CXXFLAGS:     ${GMOCK_CXXFLAGS}
        GMOCK_LDFLAGS:      ${GMOCK_LDFLAGS}
        GMOCK_LIBS:         ${GMOCK_LIBS}
        GMOCK_VERSION:      ${GMOCK_VERSION}

		COMMONAPI_CFLAGS:  ${COMMONAPI_CFLAGS}
		COMMONAPI_LIBS:    ${COMMONAPI_LIBS}

        prefix:             ${prefix}
        CXXFLAGS:           ${CXXFLAGS}
        LDFLAGS:            ${LDFLAGS}
])

AC_OUTPUT

