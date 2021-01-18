// Another JIT implementation using ASMJIT lib.
#include "asmjit/x86.h"
#include "bf.h"
#include <fstream>

namespace AsmJIT {

// Global data
constexpr int MEMORY_SIZE = 30000;
size_t dataptr = 0;
std::vector<uint8_t> memory(MEMORY_SIZE, 0);
// JitRuntime must be alive the whole time.
asmjit::JitRuntime rt;
// Wrap instruction with function, later generate machine code to call them
// >
void gt() { dataptr++; }
// <
void lt() { dataptr--; }
// +
void plus() { memory[dataptr]++; }
// -
void minus() { memory[dataptr]--; }
// .
void dot() { std::cout.put(memory[dataptr]); }
// ,
void comma() { memory[dataptr] = std::cin.get(); }

struct JitOP {
  char type;
  size_t address;
  void (*jitBlock)();
};

void callFunction(void (*target)(), asmjit::x86::Compiler &cc) {
  asmjit::InvokeNode *invokeNode;
  cc.invoke(&invokeNode, asmjit::imm((void *)target),
            asmjit::FuncSignatureT<void *>());
}

void finishBlock(asmjit::x86::Compiler &cc) {
  cc.ret();
  cc.endFunc();
  cc.finalize();
}

std::vector<JitOP *> jitEmiter(const Parser::Program &p) {
  // Initialize state.
  size_t pc = 0;
  std::vector<JitOP *> ret; // we omit memory management here.

  // asmjit
  asmjit::CodeHolder code;
  code.init(rt.environment());
  asmjit::x86::Compiler cc(&code);
  cc.addFunc(asmjit::FuncSignatureT<void>());

  while (pc < p.instructions.size()) {
    auto *op = p.instructions[pc];
    switch (op->type) {
    case '>':
      callFunction(gt, cc);
      break;
    case '<':
      callFunction(lt, cc);
      break;
    case '+':
      callFunction(plus, cc);
      break;
    case '-':
      callFunction(minus, cc);
      break;
    case '.':
      callFunction(dot, cc);
      break;
    case ',':
      callFunction(comma, cc);
      break;
    case '[': {
      // Pack the current block.
      finishBlock(cc);
      JitOP *op = new JitOP{.type = 'j'};
      rt.add(&(op->jitBlock), &code);
      ret.push_back(op);

      // Generate jump, address is empty now, will be filled during next pass.
      // just for the sake of simplicity.
      ret.push_back(new JitOP{.type = '['});
      // Clean up.
      code.reset();
      code.init(rt.environment());
      code.attach(&cc);
      cc.addFunc(asmjit::FuncSignatureT<void>());
      break;
    }
    case ']': {
      finishBlock(cc);
      JitOP *op = new JitOP{.type = 'j'};
      rt.add(&(op->jitBlock), &code);
      ret.push_back(op);

      ret.push_back(new JitOP{.type = ']'});
      // Clean up.
      code.reset();
      code.init(rt.environment());
      code.attach(&cc);
      cc.addFunc(asmjit::FuncSignatureT<void>());
      break;
    }
    }
    ++pc;
  }

  // pack and finish the last jit block
  finishBlock(cc);
  JitOP *op = new JitOP{.type = 'j'};
  rt.add(&(op->jitBlock), &code);
  ret.push_back(op);

  // Make up the missing jump address.
  pc = 0;
  while (pc < ret.size()) {
    auto *op = ret[pc];
    switch (op->type) {
    case '[': {
      int bracket_nesting = 1;
      size_t saved_pc = pc;

      while (bracket_nesting && ++pc < ret.size()) {
        if (ret[pc]->type == ']') {
          bracket_nesting--;
        } else if (ret[pc]->type == '[') {
          bracket_nesting++;
        }
      }
      if (!bracket_nesting) {
        op->address = pc;
        ret[pc]->address = saved_pc;
      }
      pc = saved_pc;
    }
    }
    ++pc;
  }
  return ret;
}

// The actual engine to execute the jited code.
void jitEngine(const std::vector<JitOP *> p) {
  // Initialize state.
  size_t pc = 0;

  while (pc < p.size()) {
    auto *op = p[pc];
    switch (op->type) {
    case 'j':
      op->jitBlock();
      break;
    case '[':
      if (memory[dataptr] == 0) {
        pc = op->address;
      }
      break;
    case ']':
      if (memory[dataptr] != 0) {
        pc = op->address;
      }
      break;
    }
    ++pc;
  }
}

void jitExecute(std::string fileName) {
  std::ifstream infile(fileName);
  auto program = Parser::parse(infile);
  auto jitedP = jitEmiter(program);
  jitEngine(jitedP);
}

}; // namespace AsmJIT
