asmjit: bf.cpp main.cpp asmjit-bf.cpp
	g++ -o bf bf.cpp main.cpp asmjit-bf.cpp bf.h -I./asmjit/src/ -DASMJIT_STATIC -lasmjit -L./asmjit -Wl,-rpath ./asmjit

bf: bf.cpp main.cpp
	g++ -o bf bf.cpp main.cpp bf.h -m32

debug: bf.cpp main.cpp
	g++ -o bf bf.cpp main.cpp bf.h -g -m32

asm: bf
	objdump -S --disassemble bf > bf.s

clean:
	rm bf
