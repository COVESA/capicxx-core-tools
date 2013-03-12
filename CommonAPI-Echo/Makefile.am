ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = LICENSE

# ------------------------------------------------------------------------------
MAINTAINERCLEANFILES = \
		Makefile.in \
		aclocal.m4 \
		configure

maintainer-clean-local:
	-rm -rf build-aux
	-rm -f config.h.in*
	-rm -f m4/libtool*.m4
	-rm -f m4/lt*.m4
	-rm -rf src-gen

AM_CPPFLAGS = \
		${COMMONAPI_DBUS_CFLAGS} \
		-I$(top_srcdir)/src-gen \
		-D_GNU_SOURCE \
		-D_GLIBCXX_USE_NANOSLEEP

AM_LDFLAGS = ${COMMONAPI_DBUS_LIBS}

# ------------------------------------------------------------------------------
lib_LTLIBRARIES = \
		libcommonapi-tests-Echo.la \
		libcommonapi-tests-EchoProxy.la \
		libcommonapi-tests-EchoStubDefault.la

libcommonapi_tests_Echo_la_SOURCES = \
		src-gen/CommonAPI/tests/Echo.h \
		src-gen/CommonAPI/tests/Echo.cpp

libcommonapi_tests_EchoProxy_la_SOURCES = \
		src-gen/CommonAPI/tests/EchoProxyBase.h \
		src-gen/CommonAPI/tests/EchoProxy.h \
		src-gen/CommonAPI/tests/EchoDBusProxy.h \
		src-gen/CommonAPI/tests/EchoDBusProxy.cpp
libcommonapi_tests_EchoProxy_la_LIBADD = \
		libcommonapi-tests-Echo.la

libcommonapi_tests_EchoStubDefault_la_SOURCES = \
		src-gen/CommonAPI/tests/EchoStub.h \
		src-gen/CommonAPI/tests/EchoStubDefault.h \
		src-gen/CommonAPI/tests/EchoStubDefault.cpp \
		src-gen/CommonAPI/tests/EchoDBusStubAdapter.h \
		src-gen/CommonAPI/tests/EchoDBusStubAdapter.cpp
libcommonapi_tests_EchoStubDefault_la_LIBADD = \
		libcommonapi-tests-Echo.la

# ------------------------------------------------------------------------------
bin_PROGRAMS = \
		EchoClient \
		EchoService

EchoClient_SOURCES = \
		src/BenchmarkStats.h \
		src/BenchmarkStats.cpp \
		src/Benchmark.h \
		src/Benchmark.cpp \
		src/EchoClient.cpp
EchoClient_LDADD = libcommonapi-tests-EchoProxy.la

EchoService_SOURCES = \
		src/EchoService.cpp
EchoService_LDADD = \
		libcommonapi-tests-EchoStubDefault.la