#include "Node.h"
#include <regex>
#include <fstream>
#include <iostream>

int main(int argc, char **argv){
  Agraph_t *g = parseDotFile(argv[1], 1);
  BLIFCircuit circ(g, "my_circuit");
  std::ofstream os;
  os.open("../my_circuit.blif", std::ios::out);
  circ.parseAttributes();
  circ.printCircuit(os);
}
