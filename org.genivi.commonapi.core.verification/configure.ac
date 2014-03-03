AC_PREREQ(2.59)
AC_INIT(org.genivi.commonapi.core.verification, 1.0)
AC_USE_SYSTEM_EXTENSIONS
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([build-aux/config.h])
AC_CONFIG_FILES([
        Makefile
        DynamicLoadingVerification.conf
        src/DynamicLoadingVerification.cpp
        CommunicationVerification.conf
        src/CommunicationVerification.cpp
        ])

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([foreign])


### Binding definitions ###

# Well known binding name
AC_MSG_CHECKING(Well known name of the binding that is to be tested)
AC_ARG_WITH([binding-name], [Well known name of the binding that is to be tested], [
    BINDING_NAME=${withval}
    AC_MSG_RESULT(${BINDING_NAME})
],[
    AC_MSG_RESULT(no)
    AC_MSG_ERROR([A binding that shall be verified needs to be specified! Use '--with-binding=' to do so.])
])


# Path to binding libraries
AC_ARG_WITH([binding], [full path to binding library, including name, excluding file ending. The .a and .so files with the given fqn will be used if present and when needed.], [
    BINDING_FQN=${withval}

    AC_MSG_CHECKING(.a binding library)
    AS_IF([test -f "${BINDING_FQN}.a"], [
              BINDING_A="${BINDING_FQN}.a"
              AC_MSG_RESULT(yes)
          ], [
              AC_MSG_RESULT(no)
          ]
    )

    AC_MSG_CHECKING(.so binding library)
    AS_IF([test -f "${BINDING_FQN}.so"], [
              BINDING_SO="${BINDING_FQN}.so"
              AC_MSG_RESULT(yes)
          ], [
              AC_MSG_RESULT(no)
          ]
    )
], [AC_MSG_ERROR([A binding that shall be verified needs to be specified! Use '--with-binding=' to do so.])])


# Additional libraries required by the binding
AC_ARG_WITH([binding-extra], [full path to additional library required by statically linked binding], [
    AC_MSG_CHECKING(required additional libraries for the binding)
    AS_IF([test -f "${withval}"], [
              BINDING_EXTRA="${withval}"
              AC_MSG_RESULT(yes)
          ], [
              AC_MSG_RESULT(no)
          ]
    )
],[])


AM_CONDITIONAL(STATICALLY_LINKED_BINDING, [test -f "${GTEST_CONFIG}" && ${TESTS_CAN_BE_EXECUTED}])
AM_CONDITIONAL(DYNAMICALLY_LINKED_BINDING, [test -f "${GTEST_CONFIG}" && ${TESTS_CAN_BE_EXECUTED}])


#Libraries containing binding specific gluecode
AC_ARG_WITH([gluecode], [full path to gluecode libraries, including name, excluding file ending. The .a and .so files with the given name will be used if present and when needed.], [
    GLUECODE_FQN=${withval}

    AC_MSG_CHECKING(.a gluecode library)
    AS_IF([test -f "${GLUECODE_FQN}.a"], [
              GLUECODE_A="${GLUECODE_FQN}.a"
              AC_MSG_RESULT(yes)
          ], [
              AC_MSG_RESULT(no)
          ]
    )

    AC_MSG_CHECKING(.so gluecode library)
    AS_IF([test -f "${GLUECODE_FQN}.so"], [
              GLUECODE_SO="${GLUECODE_FQN}.so"
              AC_MSG_RESULT(yes)
          ], [
              AC_MSG_RESULT(no)
          ]
    )
], [AC_MSG_WARN([No path to binding specific gluecode library specified. Is this correct? If not, do so with '--with-gluecode='.])])


#Substitutions
AC_SUBST(BINDING_NAME)
AC_SUBST(BINDING_SO)
AC_SUBST(BINDING_A)
AC_SUBST(BINDING_EXTRA)
AC_SUBST(GLUECODE_SO)
AC_SUBST(GLUECODE_A)


#Configurations
AC_PROG_CXX
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11([noext])

LT_PREREQ(2.2)
LT_INIT


#Dependencies
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(COMMONAPI, [CommonAPI >= 2.0])

GTEST_MIN_VERSION="1.6.0"
GTEST_URL="http://code.google.com/p/googletest"

AC_ARG_VAR([GTEST_CONFIG], [The exact path of Google Test's 'gtest-config' script.])
AC_ARG_VAR([GTEST_CPPFLAGS], [C-like preprocessor flags for Google Test.])
AC_ARG_VAR([GTEST_CXXFLAGS], [C++ compile flags for Google Test.])
AC_ARG_VAR([GTEST_LDFLAGS], [Linker path and option flags for Google Test.])
AC_ARG_VAR([GTEST_LIBS], [Library linking flags for Google Test.])
AC_ARG_VAR([GTEST_VERSION], [The available version of Google Test.])

AS_IF([test -f "${GTEST_CONFIG}"], [
           AS_IF([${GTEST_CONFIG} --min-version=${GTEST_MIN_VERSION}],
                 [],
                 [AC_MSG_ERROR([Found Google Test Version ${GTEST_VERSION}. However ${GTEST_MIN_VERSION} is
                                required. Please refer to ${GTEST_URL} for a more recent version.])])
           [GTEST_CPPFLAGS=`${GTEST_CONFIG} --cppflags`]
           [GTEST_CXXFLAGS=`${GTEST_CONFIG} --cxxflags`]
           [GTEST_LDFLAGS=`${GTEST_CONFIG} --ldflags`]
           [GTEST_LIBS=`${GTEST_CONFIG} --libs`]
           [GTEST_VERSION=`${GTEST_CONFIG} --version`]
      ], [
           AC_MSG_ERROR([Google Test could not be found! Please provide an environment variable 'GTEST_CONFIG'
                         pointing to the location of Google Test's 'gtest-config' script])
      ]
)


#Result
AC_MSG_RESULT([
        $PACKAGE_NAME v$VERSION

        COMMONAPI_CFLAGS:  ${COMMONAPI_CFLAGS}
        COMMONAPI_LIBS:    ${COMMONAPI_LIBS}

        BINDING_FQN:       ${BINDING_FQN}
        BINDING_SO:        ${BINDING_SO}
        BINDING_A:         ${BINDING_A}
        BINDING_EXTRA:     ${BINDING_EXTRA}

        GLUECODE_FQN:      ${GLUECODE_FQN}
        GLUECODE_SO:       ${GLUECODE_SO}
        GLUECODE_A:        ${GLUECODE_A}

        GTEST_CONFIG:      ${GTEST_CONFIG}
        GTEST_CPPFLAGS:    ${GTEST_CPPFLAGS}
        GTEST_CXXFLAGS:    ${GTEST_CXXFLAGS}
        GTEST_LDFLAGS:     ${GTEST_LDFLAGS}
        GTEST_LIBS:        ${GTEST_LIBS}
        GTEST_VERSION:     ${GTEST_VERSION}

        CXXFLAGS:          ${CXXFLAGS}
        LDFLAGS:           ${LDFLAGS}
])

AC_OUTPUT