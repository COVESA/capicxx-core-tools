ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = \
        -I$(top_srcdir)/src \
        -I$(includedir) \
        -I${GLUE_PATH}/src-gen/ \
        ${GLUE_CFLAGS} \
        ${COMMONAPI_CFLAGS} \
        ${GTEST_CPPFLAGS}

check_PROGRAMS = \
        verification

TESTS =	${check_PROGRAMS}

verification_SOURCES = \
            src/VerificationTestStub.h \
            src/VerificationTestStub.cpp \
            src/MainLoopTest.cpp

verification_CPPFLAGS = ${AM_CPPFLAGS}
verification_LDADD = ${COMMONAPI_LIBS} ${GLUE_LIBS} ${GTEST_LIBS} ${BINDING}
verification_LDFLAGS = ${GLUE_LDFLAGS}