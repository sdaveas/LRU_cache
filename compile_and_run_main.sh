#!/bin/bash
mkdir -p build/
cd build
g++ -O2 -o lru_cache ../src/main.cpp -lpthread && ./lru_cache
