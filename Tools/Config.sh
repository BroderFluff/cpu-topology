#!/bin/bash

declare build_type="Debug"
declare binary_dir="build"

while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--build-type)
            build_type="$2"
            shift
        ;;
        *)
            echo "Unknown argument: \"$1\"." 1>&2
        ;;
    esac
    shift
done

cmake -B "${binary_dir}" \
    -DCMAKE_BUILD_TYPE="${build_type}" \
    -DCMAKE_VERBOSE_MAKEFILE=ON \
    -Wdev \
    -G "Ninja"