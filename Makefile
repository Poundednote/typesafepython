##
# Project typesafepython
#
# @file


typesafepython: main.cpp
	g++ -std=c++20 -g -O0 main.cpp -Wall -Wno-sign-compare -o ./build/tspython.out
	g++ -std=c++20 -g -O0 tests.cpp -Wall -Wno-sign-compare -o ./test

# end
