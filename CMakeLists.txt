cmake_minimum_required(VERSION 3.15)

project(CpuID)

add_executable(cpuid)

target_sources(cpuid
    PRIVATE
        Processor.cpp
        main.cpp
)