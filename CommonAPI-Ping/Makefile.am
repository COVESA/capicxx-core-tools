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
		libcommonapi-tests-Ping.la \
		libcommonapi-tests-PingProxy.la \
		libcommonapi-tests-PingStubDefault.la

libcommonapi_tests_Ping_la_SOURCES = \
		src-gen/CommonAPI/tests/Ping.h \
		src-gen/CommonAPI/tests/Ping.cpp

libcommonapi_tests_PingProxy_la_SOURCES = \
		src-gen/CommonAPI/tests/PingProxyBase.h \
		src-gen/CommonAPI/tests/PingProxy.h \
		src-gen/CommonAPI/tests/PingDBusProxy.h \
		src-gen/CommonAPI/tests/PingDBusProxy.cpp
libcommonapi_tests_PingProxy_la_LIBADD = \
		libcommonapi-tests-Ping.la

libcommonapi_tests_PingStubDefault_la_SOURCES = \
		src-gen/CommonAPI/tests/PingStub.h \
		src-gen/CommonAPI/tests/PingStubDefault.h \
		src-gen/CommonAPI/tests/PingStubDefault.cpp \
		src-gen/CommonAPI/tests/PingDBusStubAdapter.h \
		src-gen/CommonAPI/tests/PingDBusStubAdapter.cpp
libcommonapi_tests_PingStubDefault_la_LIBADD = \
		libcommonapi-tests-Ping.la

# ------------------------------------------------------------------------------
bin_PROGRAMS = \
		PingClient \
		PingService

PingClient_SOURCES = \
		src/BenchmarkStats.h \
		src/BenchmarkStats.cpp \
		src/Benchmark.h \
		src/Benchmark.cpp \
		src/PingClient.cpp
PingClient_LDADD = libcommonapi-tests-PingProxy.la

PingService_SOURCES = \
		src/PingService.cpp
PingService_LDADD = \
		libcommonapi-tests-PingStubDefault.la