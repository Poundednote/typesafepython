@echo off
set CommonCompilerFlags= -D DEBUG=1 /GR- -Zi -std:c++20 -Od -W4 -wd4018 -wd4201 -w44062
set CommonLinkerFlags=kernel32.lib
mkdir "build"
pushd "build"
cl %CommonCompilerFlags% ..\preprocessor.cpp
preprocessor.exe ..\parser.h > ..\generation\generated.h
preprocessor.exe ..\tokeniser.h >> ..\generation\generated.h
preprocessor.exe ..\typing.h >> ..\generation\generated.h
cl %CommonCompilerFlags% ..\tpython.cpp %CommonLinkerFlags%
cl %CommonCompilerFlags% ..\tests.cpp %CommonLinkerFlags%
