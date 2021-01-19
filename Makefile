asmjit: bf.cpp main.cpp asmjit-bf.cpp
	g++ -m32 -o bf bf.cpp main.cpp asmjit-bf.cpp bf.h -I./asmjit/src/ -DASMJIT_STATIC -lrt ./asmjit/libasmjit.a

bf: bf.cpp main.cpp
	g++ -o bf bf.cpp main.cpp bf.h -m32

debug: bf.cpp main.cpp
	g++ -o bf bf.cpp main.cpp bf.h -g -m32

asm: bf
	objdump -S --disassemble bf > bf.s

clean:
	rm bf
