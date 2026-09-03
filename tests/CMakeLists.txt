cmake_minimum_required(VERSION 3.13)
project(ESPressioEventObserverTests LANGUAGES CXX)

find_package(Threads REQUIRED)

option(ESPRESSIO_ENABLE_SANITIZERS "Enable address and undefined-behavior sanitizers" OFF)

add_executable(espressio_event_observer_tests test_event_observer.cpp)
add_executable(espressio_event_reference_tests test_event_references.cpp)
add_executable(espressio_event_dispatch_context_tests test_event_dispatch_context.cpp)
add_executable(espressio_event_broker_nonblocking_tests test_event_broker_nonblocking.cpp)
add_executable(espressio_event_dispatch_completion_tests test_event_dispatch_completion.cpp)
add_executable(espressio_event_transport_packet_ownership_tests test_event_transport_packet_ownership.cpp)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_executable(espressio_event_observer_no_rtti_tests test_event_observer.cpp)
    target_compile_options(espressio_event_observer_no_rtti_tests PRIVATE -fno-rtti)
endif()

target_compile_features(espressio_event_dispatch_context_tests PRIVATE cxx_std_17)
target_compile_features(espressio_event_observer_tests PRIVATE cxx_std_17)
target_compile_features(espressio_event_reference_tests PRIVATE cxx_std_17)
target_compile_features(espressio_event_broker_nonblocking_tests PRIVATE cxx_std_17)
target_compile_features(espressio_event_dispatch_completion_tests PRIVATE cxx_std_17)
target_compile_features(espressio_event_transport_packet_ownership_tests PRIVATE cxx_std_17)
target_link_libraries(espressio_event_reference_tests PRIVATE Threads::Threads)

set(EVENT_HOST_TEST_INCLUDE_DIRS
    stubs
    ../src
    ../../ESPressio-System/src
    ../../ESPressio-Primitive/src
    ../../ESPressio-Observable/src
    ../../ESPressio_Timing/src
    ../../ESPressio-Units/src
    ../../ESPressio_Timing/tests/stubs
)

target_include_directories(espressio_event_dispatch_context_tests PRIVATE ${EVENT_HOST_TEST_INCLUDE_DIRS})
target_include_directories(espressio_event_observer_tests PRIVATE ${EVENT_HOST_TEST_INCLUDE_DIRS})
target_include_directories(espressio_event_reference_tests PRIVATE ${EVENT_HOST_TEST_INCLUDE_DIRS})
target_include_directories(espressio_event_broker_nonblocking_tests PRIVATE ${EVENT_HOST_TEST_INCLUDE_DIRS})
target_include_directories(espressio_event_dispatch_completion_tests PRIVATE ${EVENT_HOST_TEST_INCLUDE_DIRS})
target_include_directories(espressio_event_transport_packet_ownership_tests PRIVATE ${EVENT_HOST_TEST_INCLUDE_DIRS})

if(TARGET espressio_event_observer_no_rtti_tests)
    target_compile_features(espressio_event_observer_no_rtti_tests PRIVATE cxx_std_17)
    target_include_directories(espressio_event_observer_no_rtti_tests PRIVATE ${EVENT_HOST_TEST_INCLUDE_DIRS})
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach(target
        espressio_event_observer_tests
        espressio_event_reference_tests
        espressio_event_dispatch_context_tests
        espressio_event_broker_nonblocking_tests
        espressio_event_dispatch_completion_tests
        espressio_event_transport_packet_ownership_tests
    )
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endforeach()
    if(TARGET espressio_event_observer_no_rtti_tests)
        target_compile_options(espressio_event_observer_no_rtti_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
elseif(MSVC)
    foreach(target
        espressio_event_observer_tests
        espressio_event_reference_tests
        espressio_event_dispatch_context_tests
        espressio_event_broker_nonblocking_tests
        espressio_event_dispatch_completion_tests
        espressio_event_transport_packet_ownership_tests
    )
        target_compile_options(${target} PRIVATE /W4 /WX)
    endforeach()
endif()

if(ESPRESSIO_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach(target
        espressio_event_observer_tests
        espressio_event_reference_tests
        espressio_event_broker_nonblocking_tests
        espressio_event_dispatch_completion_tests
        espressio_event_transport_packet_ownership_tests
    )
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endforeach()
endif()

enable_testing()
add_test(NAME espressio_event_observer_tests COMMAND espressio_event_observer_tests)
add_test(NAME espressio_event_reference_tests COMMAND espressio_event_reference_tests)
add_test(NAME espressio_event_dispatch_context_tests COMMAND espressio_event_dispatch_context_tests)
add_test(NAME espressio_event_broker_nonblocking_tests COMMAND espressio_event_broker_nonblocking_tests)
add_test(NAME espressio_event_dispatch_completion_tests COMMAND espressio_event_dispatch_completion_tests)
add_test(NAME espressio_event_transport_packet_ownership_tests COMMAND espressio_event_transport_packet_ownership_tests)
if(TARGET espressio_event_observer_no_rtti_tests)
    add_test(NAME espressio_event_observer_no_rtti_tests COMMAND espressio_event_observer_no_rtti_tests)
endif()