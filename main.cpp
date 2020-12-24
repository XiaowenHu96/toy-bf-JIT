#include "bf.h"
#include <iostream>

// main --interpreter/--jit target
int main(int argc, char **argv) {
  if (argc >= 3) {
    if (std::string(argv[1]) == "--interpreter") {
      interpreter::interpreterExecute(argv[2]);
    } else if (std::string(argv[1]) == "--jit") {
      jit::jitExecute(argv[2]);
    }
    return 0;
  }
  std::cerr << "./bf --interpreter/--jit target_file\n";
  return -1;
}
