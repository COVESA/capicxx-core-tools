ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = \
        -I$(top_srcdir)/src \
        -I$(includedir) \
        ${GLUE_CFLAGS} \
        ${COMMONAPI_CFLAGS} \
        ${GTEST_CPPFLAGS}

bin_PROGRAMS = \
        verification

verification_SOURCES = \
            src/VerificationTestStub.h \
            src/VerificationTestStub.cpp \
            src/MainLoopTest.cpp

verification_CPPFLAGS = ${AM_CPPFLAGS}
verification_LDADD = ${COMMONAPI_LIBS} ${GLUE_LIBS} ${GTEST_LIBS}
verification_LDFLAGS = ${GLUE_LDFLAGS}