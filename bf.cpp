// Quote from someone on stackoverflow:
// "Just because a jitter generates machine code does not mean that it itself
// needs to be written in assembly. It makes no sense to do so." So what we are
// doing here is not recommended at all. Anyone wants JIT should use a modern
// library. But this example does help us understand JIT.

#include "bf.h"
#include <fstream>
#include <iostream>
#include <string>
#include <sys/mman.h>
#include <vector>

namespace Parser {
// One small modification I did for BF is to use direct jump for [ and ].
// (In contrast to looking for the next ] or [ one by one)
// This is because interpreter and JIT has different length of instructions.
// Therefore, using direct jump can eliminate the performance difference
// and make our benchmark more precise.

Program parse(std::istream &stream) {
  Program program;
  std::string s;
  // First parse eliminate comment
  for (std::string line; std::getline(stream, line);) {
    for (auto c : line) {
      if (c == '>' || c == '<' || c == '+' || c == '-' || c == '.' ||
          c == ',' || c == '[' || c == ']') {
        s.push_back(c);
      }
    }
  }
  // Second parse generate opcode
  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (c == '>' || c == '<' || c == '+' || c == '-' || c == '.' || c == ',') {
      program.instructions.push_back(new Op{.type = c});
    }
    if (c == '[') {
      int bracket_nesting = 1;
      size_t saved_i = i;

      while (bracket_nesting && ++i < s.size()) {
        if (s[i] == ']') {
          bracket_nesting--;
        } else if (s[i] == '[') {
          bracket_nesting++;
        }
      }
      if (!bracket_nesting) {
        program.instructions.push_back(new Op{.type = c, .address = i});
      } else {
        std::cerr << "unmatched '[' \n";
      }
      i = saved_i;
    }
    if (c == ']') {
      int bracket_nesting = 1;
      size_t saved_i = i;

      while (bracket_nesting && i > 0) {
        --i;
        if (s[i] == ']') {
          bracket_nesting++;
        } else if (s[i] == '[') {
          bracket_nesting--;
        }
      }
      if (!bracket_nesting) {
        program.instructions.push_back(new Op{.type = c, .address = i});
      } else {
        std::cerr << "unmatched ']' \n";
      }
      i = saved_i;
    }
  }
  return program;
}

}; // namespace Parser

// No magic in the interpreter.
namespace Interpreter {
constexpr int MEMORY_SIZE = 30000;

void basicInterpreter(const Parser::Program &p) {
  // Initialize state.
  std::vector<uint8_t> memory(MEMORY_SIZE, 0);
  size_t pc = 0;
  size_t dataptr = 0;

  while (pc < p.instructions.size()) {
    auto *op = p.instructions[pc];
    switch (op->type) {
    case '>':
      dataptr++;
      break;
    case '<':
      dataptr--;
      break;
    case '+':
      memory[dataptr]++;
      break;
    case '-':
      memory[dataptr]--;
      break;
    case '.':
      std::cout.put(memory[dataptr]);
      break;
    case ',':
      memory[dataptr] = std::cin.get();
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
    pc++;
  }
}

void interpreterExecute(std::string fileName) {
  std::ifstream infile(fileName);
  auto program = Parser::parse(infile);
  basicInterpreter(program);
}
} // namespace interpreter

namespace JIT {
// For jit, we do not rely on switch dispatch, instead, we use JIT to generate
// 'call' instructions to glue the program together.
// This technique is known as subroutine threading.

// Return an executable page.
void *alloc_executable_memory(size_t size) {
  void *ptr = mmap(0, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == (void *)-1) {
    perror("mmap");
    return NULL;
  }
  return ptr;
}

// Global data
constexpr int MEMORY_SIZE = 30000;
size_t dataptr = 0;
std::vector<uint8_t> memory(MEMORY_SIZE, 0);

// Following we put each instruction into its own function, so later we can
// generate machine code to call them
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

// New intermediate representation for our engine.
// only three types of instruction, a JIt block ('j'), a '[' or a ']'
// We do not jit [ and ] because branch operation can be hard to handle
// and they occupy relative small portion of the code (not performance critical)
struct JitOP {
  char type;
  size_t address;
  void (*jitBlock)();
};

// Some basic Macro
#define PUSH_RBP() ptr[ip++] = 0x55
#define MOV_RSP_RBP()                                                          \
  ptr[ip++] = 0x48;                                                            \
  ptr[ip++] = 0x89;                                                            \
  ptr[ip++] = 0xe5
#define POP_RBP() ptr[ip++] = 0x5d
#define CALL() ptr[ip++] = 0xe8
#define RET() ptr[ip++] = 0xc3
#define GEN_CALL_TO(func)                                                      \
  PUSH_RBP();                                                                  \
  MOV_RSP_RBP();                                                               \
  CALL();                                                                      \
  ip = calcualteAndPushRelativeAddress((void *)&func, ptr, ip);                \
  POP_RBP();

// One of the main challenge in calling a function is to calcualte the relative
// address.
size_t calcualteAndPushRelativeAddress(void *targetFunc, void *ptr, size_t ip) {
  uint8_t *currentIP = (uint8_t *)ptr + ip;
  // Note here, this is how the page look like: call 0x00 0x00 0x00 0x00
  // The ptr point to the byte after call.
  // Therefore, Relative address = target - (current + 4)
  long relativeAddress = ((uint8_t *)targetFunc - (uint8_t *)currentIP) - 4;
  // Translate it into little endian?
  ((uint8_t *)ptr)[ip++] = (uint8_t)relativeAddress;
  ((uint8_t *)ptr)[ip++] = *((uint8_t *)&relativeAddress + 1);
  ((uint8_t *)ptr)[ip++] = *((uint8_t *)&relativeAddress + 2);
  ((uint8_t *)ptr)[ip++] = *((uint8_t *)&relativeAddress + 3);
  return ip;
}

std::vector<JitOP *> jitEmiter(const Parser::Program &p) {
  // Initialize state.
  size_t pc = 0;
  std::vector<JitOP *> ret; // we omit memory management here.
  auto ptr = (uint8_t *)alloc_executable_memory(4096);
  size_t ip = 0;

  while (pc < p.instructions.size()) {
    auto *op = p.instructions[pc];
    switch (op->type) {
    case '>':
      GEN_CALL_TO(gt);
      break;
    case '<':
      GEN_CALL_TO(lt);
      break;
    case '+':
      GEN_CALL_TO(plus);
      break;
      break;
    case '-':
      GEN_CALL_TO(minus);
      break;
    case '.':
      GEN_CALL_TO(dot);
      break;
    case ',':
      GEN_CALL_TO(comma);
      break;
    case '[':
      // Pack the current block.
      RET();
      ret.push_back(new JitOP{.type = 'j', .jitBlock = (void (*)())ptr});
      // Generate jump, address is empty now, will be filled during next pass.
      // just for the sake of simplicity.
      ret.push_back(new JitOP{.type = '['});
      // Clean up.
      ip = 0;
      ptr = (uint8_t *)alloc_executable_memory(4096);
      break;
    case ']':
      RET();
      ret.push_back(new JitOP{.type = 'j', .jitBlock = (void (*)())ptr});
      ret.push_back(new JitOP{.type = ']'});
      // Clean up.
      ip = 0;
      ptr = (uint8_t *)alloc_executable_memory(4096);
      break;
    }
    ++pc;
  }

  // pack and finish the last jit block
  RET();
  ret.push_back(new JitOP{.type = 'j', .jitBlock = (void (*)())ptr});

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
} // namespace jit
