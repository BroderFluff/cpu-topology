cmake_minimum_required(VERSION 3.15)

project(CpuID)

add_executable(cpuid)

set_target_properties(cpuid
    PROPERTIES
        CXX_STANDARD_REQUIRED ON
        CXX_STANDARD 20
        CXX_EXTENSIONS OFF
        EXPORT_COMPILE_COMMANDS ON
)

target_sources(cpuid
    PRIVATE
        Processor.cpp
        Thread.cpp
        main.cpp
)

target_compile_options(cpuid
    PRIVATE
        -O3
        -s
)
