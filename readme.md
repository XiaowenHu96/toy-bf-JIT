## A toy example of JIT (subroutine threading) implementation of BF.

Note, mandelbrot benchmark does not work because memory limit. Need a dynamic
allocator for page size as well as for the virtual memory of the engine.
However, the factor benchmark is good enough to show the difference.

Tested on: Manjaro Linux x86_64 with Intel i5-8600 (6) @ 4.300GHz.

TODO:
[] Extend native JIT with 64bit machine.
[] clean up code, add comment, readability.
[] Fix makefile.
