#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.

set -o errexit

export POSIXLY_CORRECT=1

BUILD="-DHX_RELEASE=0 -O0"

ERRORS="-Wall -Wextra -pedantic-errors -Werror -Wfatal-errors -Wcast-qual \
	-Wdisabled-optimization -Wshadow -Wundef -Wconversion -Wdate-time \
	-Wmissing-declarations -Wno-unused-variable"

FLAGS="-m32 -ggdb3 -fdiagnostics-absolute-paths -fdiagnostics-color=always"

HX_DIR=`pwd`

# Build artifacts are not retained.
rm -rf ./bin; mkdir ./bin && cd ./bin

for FILE in $HX_DIR/src/*.c; do
	ccache clang $BUILD $ERRORS $FLAGS -I$HX_DIR/include \
		-std=c17 -pthread -c $FILE & PIDS="$PIDS $!"
done

for FILE in $HX_DIR/src/*.cpp $HX_DIR/example/example.cpp; do
	ccache clang++ $BUILD $ERRORS $FLAGS -I$HX_DIR/include \
		-std=c++20 -pthread -fno-exceptions -fno-rtti \
		-c $FILE & PIDS="$PIDS $!"
done

for PID in $PIDS; do
	if ! wait "$PID"; then
		exit 1
	fi
done

ccache clang++ $BUILD $FLAGS *.o -lpthread -lstdc++ -lm -o hxexample

cp $HX_DIR/example/example.cfg .

echo exit | ./hxexample
echo "\nRun hxexample from the bin directory to test interactively."

echo 🐉🐉🐉
