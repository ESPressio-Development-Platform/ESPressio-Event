cmake_minimum_required(VERSION 3.13)
project(ESPressioEventObserverTests LANGUAGES CXX)

find_package(Threads REQUIRED)

option(ESPRESSIO_ENABLE_SANITIZERS "Enable address and undefined-behavior sanitizers" OFF)

add_executable(espressio_event_observer_tests test_event_observer.cpp)
add_executable(espressio_event_reference_tests test_event_references.cpp)
add_executable(espressio_event_dispatch_context_tests test_event_dispatch_context.cpp)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_executable(espressio_event_observer_no_rtti_tests test_event_observer.cpp)
    target_compile_options(espressio_event_observer_no_rtti_tests PRIVATE -fno-rtti)
endif()

target_compile_features(espressio_event_dispatch_context_tests PRIVATE cxx_std_17)
target_include_directories(espressio_event_dispatch_context_tests PRIVATE
    ../src
)
target_compile_features(espressio_event_observer_tests PRIVATE cxx_std_17)
target_compile_features(espressio_event_reference_tests PRIVATE cxx_std_17)
target_link_libraries(espressio_event_reference_tests PRIVATE Threads::Threads)
target_include_directories(espressio_event_observer_tests PRIVATE
    stubs
    ../src
    ../../ESPressio-System/src
    ../../ESPressio-Observable/src
    ../../ESPressio_Timing/src
    ../../ESPressio-Units/src
    ../../ESPressio_Timing/tests/stubs
)
target_include_directories(espressio_event_reference_tests PRIVATE
    stubs
    ../src
    ../../ESPressio-System/src
    ../../ESPressio-Observable/src
    ../../ESPressio_Timing/src
    ../../ESPressio-Units/src
    ../../ESPressio_Timing/tests/stubs
)

if(TARGET espressio_event_observer_no_rtti_tests)
    target_compile_features(espressio_event_observer_no_rtti_tests PRIVATE cxx_std_17)
    target_include_directories(espressio_event_observer_no_rtti_tests PRIVATE
        stubs
        ../src
        ../../ESPressio-System/src
        ../../ESPressio-Observable/src
        ../../ESPressio_Timing/src
        ../../ESPressio-Units/src
        ../../ESPressio_Timing/tests/stubs
    )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(espressio_event_observer_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror
    )
    target_compile_options(espressio_event_reference_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror
    )
    target_compile_options(espressio_event_dispatch_context_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror
    )
    if(TARGET espressio_event_observer_no_rtti_tests)
        target_compile_options(espressio_event_observer_no_rtti_tests PRIVATE
            -Wall -Wextra -Wpedantic -Werror
        )
    endif()
elseif(MSVC)
    target_compile_options(espressio_event_observer_tests PRIVATE /W4 /WX)
    target_compile_options(espressio_event_reference_tests PRIVATE /W4 /WX)
    target_compile_options(espressio_event_dispatch_context_tests PRIVATE /W4 /WX)
endif()

if(ESPRESSIO_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(espressio_event_observer_tests PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer
    )
    target_link_options(espressio_event_observer_tests PRIVATE
        -fsanitize=address,undefined
    )
    target_compile_options(espressio_event_reference_tests PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer
    )
    target_link_options(espressio_event_reference_tests PRIVATE
        -fsanitize=address,undefined
    )
endif()

enable_testing()
add_test(NAME espressio_event_observer_tests COMMAND espressio_event_observer_tests)
add_test(NAME espressio_event_reference_tests COMMAND espressio_event_reference_tests)
add_test(NAME espressio_event_dispatch_context_tests COMMAND espressio_event_dispatch_context_tests)
if(TARGET espressio_event_observer_no_rtti_tests)
    add_test(NAME espressio_event_observer_no_rtti_tests COMMAND espressio_event_observer_no_rtti_tests)
endif()
