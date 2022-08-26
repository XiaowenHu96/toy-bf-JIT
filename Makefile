bf: bf.cpp main.cpp asmjit-bf.cpp ./asmjit/libasmjit.a
	g++ -m32 -o bf bf.cpp main.cpp asmjit-bf.cpp bf.h -I./asmjit/src/ ./asmjit/libasmjit.a

./asmjit/libasmjit.a: 
	cd asmjit && cmake -DCMAKE_CXX_FLAGS:STRING=-m32 -DASMJIT_STATIC=TRUE && make -j

debug: bf.cpp main.cpp
	g++ -o bf bf.cpp main.cpp bf.h -g -m32

asm: bf
	objdump -S --disassemble bf > bf.s

clean:
	rm bf && cd asmjit && make clean
