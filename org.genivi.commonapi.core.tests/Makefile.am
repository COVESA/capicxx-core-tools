ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = \
		${COMMONAPI_CFLAGS} \
		${GMOCK_CPPFLAGS} \
		-Isrc-gen
AM_CXXFLAGS = \
		${GMOCK_CXXFLAGS}
AM_LDFLAGS = \
		${GMOCK_LDFLAGS}
LDADD = \
		${COMMONAPI_LIBS} \
		${GMOCK_LIBS}

# ------------------------------------------------------------------------------
check_PROGRAMS = \
		test-derived-types \
		test-interface-proxy \
		test-interface-stub \
		test-predefined-types

TESTS = ${check_PROGRAMS}
bin_PROGRAMS = ${check_PROGRAMS}

test_derived_types_SOURCES = \
		src-gen/commonapi/tests/DerivedTypeCollection.h \
		src/test-derived-types.cpp

test_interface_proxy_SOURCES = \
		src-gen/commonapi/tests/DerivedTypeCollection.h \
		src-gen/commonapi/tests/DerivedTypeCollection.cpp \
		src-gen/commonapi/tests/TestInterfaceProxy.h \
		src/test-interface-proxy.h \
		src/test-interface-proxy.cpp

test_interface_stub_SOURCES = \
		src-gen/commonapi/tests/DerivedTypeCollection.h \
		src-gen/commonapi/tests/DerivedTypeCollection.cpp \
		src-gen/commonapi/tests/TestInterfaceStub.h \
		src-gen/commonapi/tests/TestInterfaceStubDefault.h \
		src-gen/commonapi/tests/TestInterfaceStubDefault.cpp \
		src/test-interface-stub.cpp

test_predefined_types_SOURCES = \
		src-gen/commonapi/tests/PredefinedTypeCollection.h \
		src/test-predefined-types.cpp

# ------------------------------------------------------------------------------
MAINTAINERCLEANFILES = \
		Makefile.in \
		aclocal.m4 \
		configure

clean-local:
	-rm -rf src-gen

maintainer-clean-local:
	-rm -rf build-aux
	-rm -f config.h.in*
	-rm -f m4/libtool*.m4
	-rm -f m4/lt*.m4
