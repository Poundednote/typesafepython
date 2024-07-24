@echo off
set CommonCompilerFlags= -D DEBUG=1 -fno-exceptions -g -gcodeview -std=c++20 -O0 -Wall
set CommonLinkerFlags=-L kernel32.lib
mkdir "build"
pushd "build"
clang++ %CommonCompilerFlags% ..\main.cpp -o main.exe %CommonLinkerFlags%
clang++ %CommonCompilerFlags% ..\tests.cpp -o tests.exe %CommonLinkerFlags%
