@echo off
set CommonCompilerFlags=-DDEBUG=1 -DNOREDEF=0 /GR- -Zi -std:c++20 -Od -W4 -wd4018 -wd4201 -w44062 /IC:\Users\rayyan\AppData\Local\Programs\Python\Python312\include
set CommonLinkerFlags=C:\Users\rayyan\AppData\Local\Programs\Python\Python312\libs\python312.lib
mkdir "build"
pushd "build"


cl %CommonCompilerFlags% ..\preprocessor.cpp
preprocessor.exe ..\parser.h > ..\generation\generated.h
preprocessor.exe ..\tokeniser.h >> ..\generation\generated.h
preprocessor.exe ..\typing.h >> ..\generation\generated.h
cl %CommonCompilerFlags% ..\main.cpp %CommonLinkerFlags%

IF (%1 == "test") (
        cl %CommonCompilerFlags% ..\tests.cpp %CommonLinkerFlags%
)


