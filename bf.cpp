// Quote from someone on stackoverflow:
// "Just because a jitter generates machine code does not mean that it itself
// needs to be written in assembly. It makes no sense to do so." So what we are
// doing here is not recommanded at all. Anyone wants JIT should use a modern
// library. But this example does help us understand JIT.

#include "bf.h"
#include <fstream>
#include <iostream>
#include <string>
#include <sys/mman.h>
#include <vector>

namespace Parser {
struct Program {
  std::string instructions;
};

Program parse(std::istream &stream) {
  Program program;
  for (std::string line; std::getline(stream, line);) {
    for (auto c : line) {
      if (c == '>' || c == '<' || c == '+' || c == '-' || c == '.' ||
          c == ',' || c == '[' || c == ']') {
        program.instructions.push_back(c);
      }
    }
  }
  return program;
}

}; // namespace Parser

namespace Interpreter {
constexpr int MEMORY_SIZE = 30000;

void basicInterpreter(const Parser::Program &p) {
  // Initialize state.
  std::vector<uint8_t> memory(MEMORY_SIZE, 0);
  size_t pc = 0;
  size_t dataptr = 0;

  while (pc < p.instructions.size()) {
    char instruction = p.instructions[pc];
    switch (instruction) {
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
        int bracket_nesting = 1;
        size_t saved_pc = pc;

        while (bracket_nesting && ++pc < p.instructions.size()) {
          if (p.instructions[pc] == ']') {
            bracket_nesting--;
          } else if (p.instructions[pc] == '[') {
            bracket_nesting++;
          }
        }

        if (!bracket_nesting) {
          break;
        } else {
          std::cerr << "unmatched '[' at pc=" << saved_pc;
        }
      }
      break;
    case ']':
      if (memory[dataptr] != 0) {
        int bracket_nesting = 1;
        size_t saved_pc = pc;

        while (bracket_nesting && pc > 0) {
          pc--;
          if (p.instructions[pc] == '[') {
            bracket_nesting--;
          } else if (p.instructions[pc] == ']') {
            bracket_nesting++;
          }
        }

        if (!bracket_nesting) {
          break;
        } else {
          std::cerr << "unmatched ']' at pc=" << saved_pc;
        }
      }
      break;
    default: {
      std::cerr << "bad char '" << instruction << "' at pc=" << pc;
    }
    }
  }
}

void interpreterExecute(std::string fileName) {
  std::ifstream infile(fileName);
  auto program = Parser::parse(infile);
  basicInterpreter(program);
}
} // namespace Interpreter

namespace jit {
// For jit, we do not rely on switch dispatch, instead, we use JIT to generate
// 'call' instructions to glue the program together.
// This technique is known as subroutine threading.
void *alloc_executable_memory(size_t size) {
  void *ptr = mmap(0, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == (void *)-1) {
    perror("mmap");
    return NULL;
  }
  return ptr;
}
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
// only three types of instruction, a JIt block, a [ or a ]
// We do not jit [ and ] because branch operation can be hard to handle
// and they occupy realtive small portion of the code (not performance critical)
struct Op {
  int type;
  void (*jitedBlock)();
};

// Some basic marco
#define PUSH_RBP() ptr[ip++] = 0x55
#define MOV_RBP_RSP()                                                          \
  ptr[ip++] = 0x48;                                                            \
  ptr[ip++] = 0x89;                                                            \
  ptr[ip++] = 0xe5
#define POP_RBP() ptr[ip++] = 0x5d
// One of the main challenge in calling a function is to calcualte the relative
// address, which is hard because we are JITing it!
size_t calcualteAndPushRelativeAddress(void *targetFunc, void *currentIP,
                                       size_t ip) {
  long relativeAddress = ((u_int8_t *)targetFunc - (u_int8_t *)currentIP) - 5;
  // Translate it into little endian? TODO; double check here please.
  u_int8_t *ret = new u_int8_t[4];
  ((uint8_t *)currentIP)[ip++] = (u_int8_t)relativeAddress;
  ((uint8_t *)currentIP)[ip++] = *((u_int8_t *)&relativeAddress + 1);
  ((uint8_t *)currentIP)[ip++] = *((u_int8_t *)&relativeAddress + 2);
  ((uint8_t *)currentIP)[ip++] = *((u_int8_t *)&relativeAddress + 3);
  return ip;
}

std::vector<Op *> jitEmiter(const Parser::Program &p) {
  // Initialize state.
  size_t pc = 0;
  std::vector<Op *> ret; // we omit memory management here.
  auto ptr = (u_int8_t *)alloc_executable_memory(4096);
  size_t ip = 0;

  while (pc < p.instructions.size()) {
    char instruction = p.instructions[pc];
    switch (instruction) {
    case '>':
      PUSH_RBP();
      MOV_RBP_RSP();
      ip = calcualteAndPushRelativeAddress((void *)&gt, ptr + ip, ip);
      break;
    case '<':
      PUSH_RBP();
      MOV_RBP_RSP();
      ip = calcualteAndPushRelativeAddress((void *)&lt, ptr + ip, ip);
      break;
    case '+':
      PUSH_RBP();
      MOV_RBP_RSP();
      ip = calcualteAndPushRelativeAddress((void *)&plus, ptr + ip, ip);
      break;
    case '-':
      PUSH_RBP();
      MOV_RBP_RSP();
      ip = calcualteAndPushRelativeAddress((void *)&minus, ptr + ip, ip);
      break;
    case '.':
      PUSH_RBP();
      MOV_RBP_RSP();
      ip = calcualteAndPushRelativeAddress((void *)&dot, ptr + ip, ip);
      break;
    case ',':
      PUSH_RBP();
      MOV_RBP_RSP();
      ip = calcualteAndPushRelativeAddress((void *)&comma, ptr + ip, ip);
      break;
    case '[':
    case ']':
    default: {
      std::cerr << "bad char '" << instruction << "' at pc=" << pc;
    }
    }
    ++pc;
  }
  if (ip != 0) {
    // pack and finish the last jit block
    ret.push_back(new Op{.type = 0, .jitedBlock = (void (*)())ptr});
  }
  return ret;
}

void jitEngine(const std::vector<Op *> p) {
  // Initialize state.
  size_t pc = 0;

  while (pc < p.size()) {
    auto *op = p[pc];
    switch (op->type) {
    case 0:
      op->jitedBlock();
    default: {
      std::cerr << "bad instruction '" << op->type << "' at pc=" << pc;
    }
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
