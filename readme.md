## A toy example of JIT (subroutine threading) implementation of BF.

Note, mandelbrot benchmark does not work because memory limit. Need a dynamic
allocator for page size as well as for the virtual memory of the engine.
However, the factor benchmark is good enough to show the difference.

Tested on: Manjaro Linux x86_64 with Intel i5-8600 (6) @ 4.300GHz.

TODO:
- [X] Extend native JIT with 64bit machine. 
    * Edited: All to 32 bit now.
    Native subroutine threading with 64bit can not be easily done. 
    Basically, we need to allocate page nearby so that the relative address can
    be expressed in `JUMP REL32`. But the process is tedious and not worth to
    explored in this toy example.  On the other hand, indirects call (by
    loading address in register) is just same as dispatch, and far call is too
    expensive.  This is another great point of why we should not write native
    JIT, and why subroutine threading is becoming a less good option compared
    to jitting instructions directly with the help of library. (Because even
    with library, for subroutine threading you need to make sure it is using
    relative call instead of indirect call or even far call)

    * See: https://stackoverflow.com/questions/54947302/handling-calls-to-potentially-far-away-ahead-of-time-compiled-functions-from-j 
    * See: https://stackoverflow.com/questions/38961192/how-to-execute-a-call-instruction-with-a-64-bit-absolute-address
- [ ] clean up code, add comment, readability.
- [X] Fix makefile.
    You need to build asmjit as a 32bit static library, put it in `asmjit/`
