cmake_minimum_required(VERSION 3.10)

project(redhat)

# Designated initializers are supported in MSVC only since C++20.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

link_libraries(ws2_32)

add_library(redhat-lib
    BinaryStream.cpp
    CCharacter.cpp
    CRC_32.cpp
    character.cpp
    checkpoint.cpp
    client.cpp
    config.cpp
    hat2.cpp
    kill_stats.cpp
    lgn.cpp
    listener.cpp
    login.cpp
    merge_items.cpp
    packet.cpp
    serialize.cpp
    server.cpp
    session.cpp
    shelf.cpp
    sha1.cpp
    socket.cpp
    sql.cpp
    status.cpp
    update_character.cpp
    utils.cpp
    version.cpp
)

# MySQL library and dirent.h.
target_include_directories(redhat-lib PUBLIC include mysql/include)
target_link_libraries(redhat-lib PUBLIC ${CMAKE_SOURCE_DIR}/mysql/lib/libmysql.lib)

add_executable(redhat
    redhat.cpp
)
target_link_libraries(redhat redhat-lib)

add_executable(redhat-test
    test/shelf_test.cpp 
    test/kill_stats_test.cpp
    test/login_test.cpp
    test/merge_items_test.cpp
    test/test.cpp
    test/UnitTest++/AssertException.cpp
    test/UnitTest++/AssertException.h
    test/UnitTest++/CheckMacros.h
    test/UnitTest++/Checks.cpp
    test/UnitTest++/Checks.h
    test/UnitTest++/CompositeTestReporter.cpp
    test/UnitTest++/CompositeTestReporter.h
    test/UnitTest++/Config.h
    test/UnitTest++/CurrentTest.cpp
    test/UnitTest++/CurrentTest.h
    test/UnitTest++/DeferredTestReporter.cpp
    test/UnitTest++/DeferredTestReporter.h
    test/UnitTest++/DeferredTestResult.cpp
    test/UnitTest++/DeferredTestResult.h
    test/UnitTest++/ExceptionMacros.h
    test/UnitTest++/ExecuteTest.h
    test/UnitTest++/HelperMacros.h
    test/UnitTest++/MemoryOutStream.cpp
    test/UnitTest++/MemoryOutStream.h
    test/UnitTest++/ReportAssert.cpp
    test/UnitTest++/ReportAssert.h
    test/UnitTest++/ReportAssertImpl.h
    test/UnitTest++/RequireMacros.h
    test/UnitTest++/RequiredCheckException.cpp
    test/UnitTest++/RequiredCheckException.h
    test/UnitTest++/RequiredCheckTestReporter.cpp
    test/UnitTest++/RequiredCheckTestReporter.h
    test/UnitTest++/Test.cpp
    test/UnitTest++/Test.h
    test/UnitTest++/TestDetails.cpp
    test/UnitTest++/TestDetails.h
    test/UnitTest++/TestList.cpp
    test/UnitTest++/TestList.h
    test/UnitTest++/TestMacros.h
    test/UnitTest++/TestReporter.cpp
    test/UnitTest++/TestReporter.h
    test/UnitTest++/TestReporterStdout.cpp
    test/UnitTest++/TestReporterStdout.h
    test/UnitTest++/TestResults.cpp
    test/UnitTest++/TestResults.h
    test/UnitTest++/TestRunner.cpp
    test/UnitTest++/TestRunner.h
    test/UnitTest++/TestSuite.h
    test/UnitTest++/ThrowingTestReporter.cpp
    test/UnitTest++/ThrowingTestReporter.h
    test/UnitTest++/TimeConstraint.cpp
    test/UnitTest++/TimeConstraint.h
    test/UnitTest++/TimeHelpers.h
    test/UnitTest++/UnitTest++.h
    test/UnitTest++/UnitTestPP.h
    test/UnitTest++/Win32/TimeHelpers.cpp
    test/UnitTest++/Win32/TimeHelpers.h
    test/UnitTest++/XmlTestReporter.cpp
    test/UnitTest++/XmlTestReporter.h
)
target_link_libraries(redhat-test redhat-lib)

target_include_directories(redhat-test PUBLIC test/UnitTest++)

# For std::min.
target_compile_definitions(redhat-lib PUBLIC -DNOMINMAX -D_WINSOCK_DEPRECATED_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS)

# Static link.
set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")

target_compile_options(redhat-lib PUBLIC /MT)
target_link_options(redhat-lib PUBLIC /NODEFAULTLIB:MSVCRT)

target_compile_options(redhat PUBLIC /MT)
target_link_options(redhat PUBLIC /NODEFAULTLIB:MSVCRT)

target_compile_options(redhat-test PUBLIC /MT)
target_link_options(redhat-test PUBLIC /NODEFAULTLIB:MSVCRT)

# Run the test.
add_custom_command(
    TARGET redhat-test
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND copy mysql\\lib\\libmysql.dll ${RUNTIME_OUTPUT_DIRECTORY}/libmysql.dll
)

add_custom_command(
    TARGET redhat-test
    POST_BUILD
    COMMAND redhat-test
)
