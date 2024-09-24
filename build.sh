#!/bin/sh

# For more build options, visit
# https://klee.github.io/build-script/

# Base folder where dependencies and KLEE itself are installed
BASE=$HOME/klee_build
BUILD_SUFFIX="Release"

## KLEE Required options
# Build type for KLEE. The options are:
# Release
# Release+Debug
# Release+Asserts
# Release+Debug+Asserts
# Debug
# Debug+Asserts
KLEE_RUNTIME_BUILD="Release"

COVERAGE=0
ENABLE_DOXYGEN=0
USE_TCMALLOC=0
TCMALLOC_VERSION=2.9.1
USE_LIBCXX=1
# Also required despite not being mentioned in the guide
SQLITE_VERSION="3400100"

## LLVM Required options
LLVM_VERSION=14
ENABLE_OPTIMIZED=1
ENABLE_DEBUG=0
DISABLE_ASSERTIONS=1
REQUIRES_RTTI=1

## Solvers Required options
# SOLVERS=STP
SOLVERS=BITWUZLA:Z3:STP

## Google Test Required options
GTEST_VERSION=1.11.0

## json options
JSON_VERSION=v3.11.3

## immer options
IMMER_VERSION=v0.8.1

## UClibC Required options
UCLIBC_VERSION=klee_uclibc_v1.3
# LLVM_VERSION is also required for UClibC

## Z3 Required options
Z3_VERSION=4.8.15

STP_VERSION=2.3.3
MINISAT_VERSION=master

BITWUZLA_VERSION=0.3.2

KEEP_PARSE="true"
while [ $KEEP_PARSE = "true" ]; do
if [ "$1" = "--debug" ] || [ "$1" = "-g" ]; then
    BUILD_SUFFIX="Debug"
    ENABLE_OPTIMIZED=0
    ENABLE_DEBUG=1
    KLEE_RUNTIME_BUILD="Debug+Asserts"
    shift 1
else
    KEEP_PARSE="false"
fi
done

ENABLE_WARNINGS_AS_ERRORS=0

BASE="$BASE" BUILD_SUFFIX="$BUILD_SUFFIX" KLEE_RUNTIME_BUILD=$KLEE_RUNTIME_BUILD COVERAGE=$COVERAGE ENABLE_DOXYGEN=$ENABLE_DOXYGEN USE_TCMALLOC=$USE_TCMALLOC USE_LIBCXX=$USE_LIBCXX LLVM_VERSION=$LLVM_VERSION ENABLE_OPTIMIZED=$ENABLE_OPTIMIZED ENABLE_DEBUG=$ENABLE_DEBUG DISABLE_ASSERTIONS=$DISABLE_ASSERTIONS REQUIRES_RTTI=$REQUIRES_RTTI SOLVERS=$SOLVERS GTEST_VERSION=$GTEST_VERSION UCLIBC_VERSION=$UCLIBC_VERSION STP_VERSION=$STP_VERSION MINISAT_VERSION=$MINISAT_VERSION Z3_VERSION=$Z3_VERSION BITWUZLA_VERSION=$BITWUZLA_VERSION SQLITE_VERSION=$SQLITE_VERSION JSON_VERSION=$JSON_VERSION IMMER_VERSION=$IMMER_VERSION SANITIZER_BUILD=$SANITIZER_BUILD SANITIZER_LLVM_VERSION=$SANITIZER_LLVM_VERSION ENABLE_WARNINGS_AS_ERRORS=$ENABLE_WARNINGS_AS_ERRORS ./scripts/build/build.sh klee --install-system-deps
