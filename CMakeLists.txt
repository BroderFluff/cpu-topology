cmake_minimum_required(VERSION 3.15)

project(CpuID)

add_executable(cpuid)

target_sources(cpuid
    PRIVATE
        Processor.cpp
        Thread.cpp
        main.cpp
)

target_compile_features(cpuid
    PRIVATE
        cxx_std_20
)
