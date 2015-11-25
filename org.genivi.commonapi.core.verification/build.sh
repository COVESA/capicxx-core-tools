export GTEST_ROOT=$HOME/Projects/commonapi/helper/gtest-1.7.0/

rm -rf build
rm -rf src-gen
mkdir build
cd build

cmake \
-DCommonAPI_DIR=$(readlink -f ../../../ascgit017.CommonAPI/build) \
-DCOMMONAPI_TOOL_GENERATOR=$(readlink -f ../../../ascgit017.CommonAPI-Tools/org.genivi.commonapi.core.cli.product/target/products/org.genivi.commonapi.core.cli.product/linux/gtk/x86/commonapi-generator-linux-x86) \
-DCMAKE_GLUECODE_SOMEIP_NAME=SomeIPGlue \
-DSomeIPGlue_DIR=$(readlink -f ../../../ascgit017.CommonAPI-SomeIP-Tools/org.genivi.commonapi.someip.verification/build) \
-DCMAKE_GLUECODE_DBUS_NAME=DBusGlue \
-DDBusGlue_DIR=$(readlink -f ../../../ascgit017.CommonAPI-D-Bus-Tools/org.genivi.commonapi.dbus.verification/build) \
..

make