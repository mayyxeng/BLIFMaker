#include "Node.h"
#include <regex>
#include <fstream>
#include <iostream>
#include <vector>
int main(int argc, char **argv){
  Agraph_t *g = parseDotFile(argv[1], 1);
  BLIFCircuit circ(g, "my_circuit");
  std::ofstream os;
  os.open("../my_circuit.blif", std::ios::out);
  circ.parseAttributes();
  circ.printCircuit(os);
  // std::vector<int> *v = new std::vector<int>;
  // v->push_back(1);
  // v->push_back(2);
  // v->push_back(3);
  // v->push_back(4);
  // v->push_back(5);
  // for(auto _v : *v){
  //   std::cout << _v << std::endl;
  // }
}
