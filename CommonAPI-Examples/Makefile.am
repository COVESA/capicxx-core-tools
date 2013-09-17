ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = LICENSE
MOSTLYCLEANFILES =

AM_CPPFLAGS = \
        -include $(top_builddir)/build-aux/config.h \
        -I$(top_srcdir)/src \
        -I$(top_srcdir)/src/test \
        ${COMMONAPI_CFLAGS} \
        ${COMMONAPI_DBUS_CFLAGS} \
        -Isrc-gen

AM_LDFLAGS = \
        ${COMMONAPI_LIBS} \
        ${COMMONAPI_DBUS_LIBS}

# ------------------------------------------------------------------------------
bin_PROGRAMS = \
	helloworld-proxy \
	helloworld-stub

helloworld_proxy_SOURCES = \
	src/helloworld-proxy.cpp \
	src-gen/commonapi/examples/HelloWorldInterface.h \
	src-gen/commonapi/examples/HelloWorldInterfaceProxyBase.h \
	src-gen/commonapi/examples/HelloWorldInterfaceProxy.h \
	src-gen/commonapi/examples/HelloWorldInterfaceDBusProxy.h \
	src-gen/commonapi/examples/HelloWorldInterfaceDBusProxy.cpp \
	src-gen/commonapi/examples/HelloWorldLeaf.h \
	src-gen/commonapi/examples/HelloWorldLeafProxyBase.h \
	src-gen/commonapi/examples/HelloWorldLeafProxy.h \
	src-gen/commonapi/examples/HelloWorldLeafDBusProxy.h \
	src-gen/commonapi/examples/HelloWorldLeafDBusProxy.cpp

helloworld_stub_SOURCES = \
	src/helloworld-stub.cpp \
	src-gen/commonapi/examples/HelloWorldInterface.h \
	src-gen/commonapi/examples/HelloWorldInterfaceStub.h \
	src-gen/commonapi/examples/HelloWorldInterfaceStubDefault.h \
	src-gen/commonapi/examples/HelloWorldInterfaceStubDefault.cpp \
	src-gen/commonapi/examples/HelloWorldInterfaceDBusStubAdapter.h \
	src-gen/commonapi/examples/HelloWorldInterfaceDBusStubAdapter.cpp \
	src-gen/commonapi/examples/HelloWorldLeaf.h \
	src-gen/commonapi/examples/HelloWorldLeafStub.h \
	src-gen/commonapi/examples/HelloWorldLeafStubDefault.h \
	src-gen/commonapi/examples/HelloWorldLeafStubDefault.cpp \
	src-gen/commonapi/examples/HelloWorldLeafDBusStubAdapter.h \
	src-gen/commonapi/examples/HelloWorldLeafDBusStubAdapter.cpp

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
