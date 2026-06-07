# CMake generated Testfile for 
# Source directory: C:/Users/Noxi-PC/open.mp-sdk/WorldSync
# Build directory: C:/Users/Noxi-PC/open.mp-sdk/WorldSync/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[WorldSyncTests]=] "C:/Users/Noxi-PC/open.mp-sdk/WorldSync/build/Debug/WorldSyncTests.exe")
  set_tests_properties([=[WorldSyncTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Noxi-PC/open.mp-sdk/WorldSync/CMakeLists.txt;32;add_test;C:/Users/Noxi-PC/open.mp-sdk/WorldSync/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[WorldSyncTests]=] "C:/Users/Noxi-PC/open.mp-sdk/WorldSync/build/Release/WorldSyncTests.exe")
  set_tests_properties([=[WorldSyncTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Noxi-PC/open.mp-sdk/WorldSync/CMakeLists.txt;32;add_test;C:/Users/Noxi-PC/open.mp-sdk/WorldSync/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[WorldSyncTests]=] "C:/Users/Noxi-PC/open.mp-sdk/WorldSync/build/MinSizeRel/WorldSyncTests.exe")
  set_tests_properties([=[WorldSyncTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Noxi-PC/open.mp-sdk/WorldSync/CMakeLists.txt;32;add_test;C:/Users/Noxi-PC/open.mp-sdk/WorldSync/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[WorldSyncTests]=] "C:/Users/Noxi-PC/open.mp-sdk/WorldSync/build/RelWithDebInfo/WorldSyncTests.exe")
  set_tests_properties([=[WorldSyncTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Noxi-PC/open.mp-sdk/WorldSync/CMakeLists.txt;32;add_test;C:/Users/Noxi-PC/open.mp-sdk/WorldSync/CMakeLists.txt;0;")
else()
  add_test([=[WorldSyncTests]=] NOT_AVAILABLE)
endif()
subdirs("sdk")
