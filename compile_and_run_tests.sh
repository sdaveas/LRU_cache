#!/bin/bash
mkdir -p build/
cd build
g++ -O2 -o tests ../tests/tests.cpp -lpthread && ./tests
