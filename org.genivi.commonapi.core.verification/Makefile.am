ACLOCAL_AMFLAGS = -I m4

GLUE_LDFLAGS_FOR_A="-Wl,--whole-archive,${GLUECODE_A},${BINDING_A},--no-whole-archive,${BINDING_EXTRA}"
GLUE_LDFLAGS_FOR_SO="-Wl,--no-as-needed,${GLUECODE_SO},${BINDING_SO},--as-needed,${BINDING_EXTRA}"

AM_CPPFLAGS = \
        -I$(top_srcdir)/src \
        -I./src-gen \
        -I$(includedir) \
        ${COMMONAPI_CFLAGS} \
        ${GTEST_CPPFLAGS}

check_PROGRAMS = \
        DynamicLoadingVerification \
        MainLoopVerification

generated = \
        src-gen/commonapi/tests/DerivedTypeCollection.cpp \
        src-gen/commonapi/tests/TestInterfaceStubDefault.cpp

TESTS = ${check_PROGRAMS}

DynamicLoadingVerification_SOURCES = \
        src/utils/VerificationTestStub.cpp \
        src/DynamicLoadingVerification.cpp

MainLoopVerification_SOURCES = \
        src/utils/VerificationTestStub.cpp \
        src/MainLoopVerification.cpp

DynamicLoadingVerification_CPPFLAGS = ${AM_CPPFLAGS}
DynamicLoadingVerification_LDADD = ${COMMONAPI_LIBS} ${GTEST_LIBS}
DynamicLoadingVerification_LDFLAGS = ${GLUE_LDFLAGS_FOR_SO}

MainLoopVerification_CPPFLAGS = ${AM_CPPFLAGS}
MainLoopVerification_LDADD = ${COMMONAPI_LIBS} ${BINDING_A} ${GTEST_LIBS}
MainLoopVerification_LDFLAGS = ${GLUE_LDFLAGS_FOR_A}
