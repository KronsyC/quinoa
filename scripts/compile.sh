#!/bin/bash
./scripts/generateTokenDefinitions.py > /dev/null
cmake . -B cmake -G Ninja
cmake --build cmake