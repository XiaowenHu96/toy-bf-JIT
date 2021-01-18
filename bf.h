#include <string>
#include <vector>
#include <fstream>
#include <iostream>

namespace Interpreter {
void interpreterExecute(std::string fileName);
};
namespace JIT {
void jitExecute(std::string fileName);
};

namespace Parser {
    struct Op {
      char type;
      size_t address;
    };

    struct Program {
      std::vector<Op *> instructions;
    };

    Program parse(std::istream &stream);
}; 

namespace AsmJIT {
    void jitExecute(std::string fileName);
};
