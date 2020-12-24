#!/bin/bash
echo "Testing interpreter"
time ./bf --interpreter tests/factor.bf <<< 179424691  
echo "Testing jit"
time ./bf --jit tests/factor.bf <<< 179424691  
