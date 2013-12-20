AC_PREREQ(2.59)
AC_INIT(org.genivi.commonapi.core.verification, 1.0)
AC_USE_SYSTEM_EXTENSIONS
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([build-aux/config.h])

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([foreign])

AC_ARG_WITH([gluecode-path], [path to gluecode containing directories src-gen and .libs], [
    GLUE_PATH=${withval}
], [])

AC_ARG_WITH([gluecode-so-name], [name of gluecode .so file within directory .libs], [
    GLUE_SHARED=true
    GLUE_SO=${withval}
    GLUE_LIBS=${GLUE_PATH}/.libs/${GLUE_SO}
], [])

AC_ARG_WITH([binding-a], [full path to binding .a file if gluecode is linked statically], [
    BINDING_A=${withval}
], [])

AC_ARG_WITH([binding-so], [full path to binding .so file if gluecode is linked statically], [
    BINDING_SO=${withval}
], [])

AC_ARG_WITH([binding-extra], [full path to additional .so files required by statically linked binding], [
    BINDING_EXTRA=${withval}
], [])

AS_IF([test "${BINDING_A}" != ""], [
    BINDING="--whole-archive,${BINDING_A},--no-whole-archive"
]
, [
    BINDING=${BINDING_SO}
])

AC_ARG_WITH([gluecode-a-name], [name of gluecode .a file within direcotry .libs], [
    GLUE_SHARED=true
    GLUE_A=${withval}
    GLUE_LDFLAGS="-Wl,--whole-archive,${GLUE_PATH}/.libs/${GLUE_A},--no-whole-archive,${BINDING},${BINDING_EXTRA}"
], [])

GLUE_CFLAGS=-I${GLUE_PATH}/src-gen

AC_SUBST(GLUE_CFLAGS)
AC_SUBST(GLUE_LIBS)
AC_SUBST(GLUE_PATH)
AC_SUBST(GLUE_SO)
AC_SUBST(GLUE_LDFLAGS)
AC_SUBST(BINDING)

AC_PROG_CXX
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11([noext])

LT_PREREQ(2.2)
LT_INIT

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(COMMONAPI, [CommonAPI >= 0.7])

GTEST_MIN_VERSION="1.6.0"
GTEST_URL="http://code.google.com/p/googletest"

AC_ARG_VAR([GTEST_CONFIG], [The exact path of Google Test's 'gtest-config' script.])
AC_ARG_VAR([GTEST_CPPFLAGS], [C-like preprocessor flags for Google Test.])
AC_ARG_VAR([GTEST_CXXFLAGS], [C++ compile flags for Google Test.])
AC_ARG_VAR([GTEST_LDFLAGS], [Linker path and option flags for Google Test.])
AC_ARG_VAR([GTEST_LIBS], [Library linking flags for Google Test.])
AC_ARG_VAR([GTEST_VERSION], [The available version of Google Test.])

AS_IF([test -f "${GTEST_CONFIG}"],
      [AS_IF([${GTEST_CONFIG} --min-version=${GTEST_MIN_VERSION}],
             [],
             [AC_MSG_ERROR([dnl
                            Found Google Test Version ${GTEST_VERSION}. However ${GTEST_MIN_VERSION} is
                            required. Please refer to ${GTEST_URL} for a more recent version.])])
       [GTEST_CPPFLAGS=`${GTEST_CONFIG} --cppflags`]
       [GTEST_CXXFLAGS=`${GTEST_CONFIG} --cxxflags`]
       [GTEST_LDFLAGS=`${GTEST_CONFIG} --ldflags`]
       [GTEST_LIBS=`${GTEST_CONFIG} --libs`]
       [GTEST_VERSION=`${GTEST_CONFIG} --version`]
      ]
      []
)

AC_MSG_RESULT([
        $PACKAGE_NAME v$VERSION

        enable docs:       ${ENABLE_DOCS}

        COMMONAPI_CFLAGS:  ${COMMONAPI_CFLAGS}
        COMMONAPI_LIBS:    ${COMMONAPI_LIBS}

        BINDING:           ${BINDING}

        GLUE_CFLAGS:  ${GLUE_CFLAGS}
        GLUE_LIBS:    ${GLUE_LIBS}
        GLUE_SO:	  ${GLUE_SO}
        GLUE_A:	      ${GLUE_A}
        GLUE_PATH:    ${GLUE_PATH}

        GTEST_CONFIG:      ${GTEST_CONFIG}
        GTEST_CPPFLAGS:    ${GTEST_CPPFLAGS}
        GTEST_CXXFLAGS:    ${GTEST_CXXFLAGS}
        GTEST_LDFLAGS:     ${GTEST_LDFLAGS}
        GTEST_LIBS:        ${GTEST_LIBS}
        GTEST_VERSION:     ${GTEST_VERSION}

        prefix:            ${prefix}
        CXXFLAGS:          ${CXXFLAGS}
        LDFLAGS:           ${LDFLAGS}
])

AC_CONFIG_FILES(Makefile)
AC_OUTPUT