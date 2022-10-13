#!/bin/bash
scripts/generateTokenDefinitions.py
# cmake . -B build -G Ninja > /dev/null
cmake --build build
