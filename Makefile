##
# Project typesafepython
#
# @file
# @version 0.1

typesafepython: main.cpp
	clang++ -g -O0 -std=c++17 main.cpp -Wall -o ./build/tspython.out


# end
