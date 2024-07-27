@echo off
set CommonCompilerFlags= -D DEBUG=1 -fno-exceptions -Zi -std:c++20 -Od -W4
set CommonLinkerFlags=kernel32.lib
mkdir "build"
pushd "build"
cl %CommonCompilerFlags% ..\main.cpp %CommonLinkerFlags%
cl %CommonCompilerFlags% ..\tests.cpp %CommonLinkerFlags%
