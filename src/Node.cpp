#include "../include/Node.h"
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

Agraph_t *parseDotFile(std::string filePath, int verbosity) {

  FILE *f;
  // Read graph from file
  f = fopen(filePath.c_str(), "r");
  Agraph_t *g = agread(f, nullptr);
  fclose(f);
  // Traverse nodes and print some info
  if (verbosity > 0) {
    std::cout << "Read graph " << agnameof(g) << " from file:" << filePath
              << std::endl;
    std::cout << "Graph " << agnameof(g) << " is a ";
    if (agisdirected(g))
      std::cout << "directed ";
    if (agisstrict(g))
      std::cout << "strict ";
    std::cout << "with:\n\t" << agnnodes(g) << " nodes"
              << "\n\t" << agnedges(g) << " edges"
              << "\n\t" << agnsubg(g) << " subgraph(s)\n";
    Agsym_t *globalSym = agattrsym(g, "channel_width");
    if (!globalSym) {
      std::cout << "Warning: channel_width not specified." << std::endl;
    } else {
      std::cout << "\tchannel_width is " << agget(g, "channel_width")
                << std::endl;
    }
    for (Agnode_t *n = agfstnode(g); n; n = agnxtnode(g, n)) {
      Agsym_t *sym;
      sym = agattrsym(n, "width");
      if (!sym && verbosity > 1) {
        std::cout << "Warning: width attribute for node " << agnameof(n)
                  << " is not specified, using 32..\n"
                  << std::endl;
      }
      std::cout << "Found node " << agnameof(n) << " with type ";
      std::string typeStr(agget(n, "type"));
      if (typeStr.empty())
        std::cout << "UNSPECIFIED(will be ignored)" << std::endl;
      std::cout << agget(n, "type") << std::endl;
    }
  }
  return g;
}
std::string get_indent_string(int indent) {
  std::string indent_str("");
  for (int i = 0; i < indent; i++)
    indent_str = indent_str + std::string("\t");
  return indent_str;
}

void BLIFCircuit::parseAttributes() {
  Agsym_t *globalSym = agattrsym(graph, "channel_width");

  if (!globalSym) {
    std::cout << "Warning: channel_width not specified." << std::endl;
    std::cout << "Default channel_width is set to 32" << std::endl;
    channelWidth = 32;
  } else {
    std::istringstream conv(std::string(agget(graph, "channel_width")));
    conv >> channelWidth;
  }
  // Traverse nodes to get attributes
  for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n)) {
    // First we need to bind our desired attributes
    NodeAttr_t *attr = new NodeAttr_t;
    attr = (NodeAttr_t *)agbindrec(n, ATTR_STR, sizeof(NodeAttr_t), FALSE);
    // Then we need to set proper values in the binded attribute/record
    setName(n, attr);
    setType(n, attr);
    getInputs(n, attr);
    // getOutputs(n, attr);
    // Traverse out edges and get attribtues
    for (Agedge_t *e = agfstout(graph, n); e; e = agnxtout(graph, e)) {
      std::cout << "visited edge from " << agnameof(agtail(e)) << " to "
                << agnameof(aghead(e)) << std::endl;
    }
    std::cout << "Traversed out edges\n";
    // processIO(n, attr);
  }
}
void BLIFCircuit::setName(Agnode_t *node, NodeAttr_t *attributes) {
  attributes->name = std::string(agnameof(node));
}
void BLIFCircuit::setType(Agnode_t *node, NodeAttr_t *attributes) {
  std::string typeStr(agget(node, "type"));
  // Remove whitespaces
  typeStr.erase(remove(typeStr.begin(), typeStr.end(), ' '), typeStr.end());
  Type t_ = isValidType(typeStr);
  attributes->type = t_;
  attributes->valid = FALSE;
  if (t_ == _Null) {
    // std::cout << "COMMENT" << std::endl;
    attributes->name = std::string("NULL");

  } else if (t_ == _Error) {
    std::cerr << "Error: unkown type \"" << typeStr << "\"\n";
    exit(1);
  } else {
    attributes->typeStr = typeStr;
    if (t_ != Exit && t_ != Entry)
      attributes->valid = TRUE;
  }
}
BLIFCircuit::Type BLIFCircuit::isValidType(std::string typeStr) {
  if (typeStr.empty())
    return _Null;
  for (int i = 0; i < 10; i++) {
    //  std::cout << typeStr << ":" << Type_str[i] << std::endl;
    if (typeStr == Type_str[i]) {
      // std::cout << "match\n";
      return (Type)i;
    }
  }
  return _Error;
}
void BLIFCircuit::getInputs(Agnode_t *node, NodeAttr_t *attributes) {
  // We need to check whether inputs are specified or not since an entry node
  // does not have any inputs and we should avoid potential run time erros
  Agsym_t *sym = agattrsym(node, "in");
  NodeAttr_t *attrs = getAttributes(node);
  if (sym && attrs->valid) {
    // std::string inputExpression = agget(node, "in");
    std::string inputExpression(std::string("in1 :1 in2 in3: 3 in4 : 10 in5"));
    std::cout << "found input expression of \"" << inputExpression
              << "\" for node " << agnameof(node) << std::endl;
    BLIFPort *port = new BLIFPort(inputExpression);

    attrs->port = port;
  }
}

bool BLIFCircuit::isNumber(std::string str, int &num){};
BLIFCircuit::NodeAttr_t *BLIFCircuit::getAttributes(Agnode_t *n) {
  return (NodeAttr_t *)aggetrec(n, ATTR_STR, 0);
}
void BLIFCircuit::printCircuit(std::ostream &os, int indent) {
  const std::string header("#### BLIF netlist of DFG circuit\n");
  std::string indent_str = get_indent_string(indent);

  std::time_t t =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  os << indent_str << "#### File Created: " << std::ctime(&t);

  os << indent_str << header;
  os << indent_str << ".model " << name << std::endl;

  for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n)) {

    printModel(os, n, indent + 1);
    // processIO(n, attr);
  }
}
void BLIFCircuit::printIO(std::ostream &os, int indent) {}
void BLIFCircuit::printModel(std::ostream &os, Agnode_t *model, int indent) {
  std::string indent_str = get_indent_string(indent);
  NodeAttr_t *attrs = getAttributes(model);
  os << indent_str << "#Node " << attrs->name << std::endl;
  if (attrs->valid)
    os << indent_str << ".model " << attrs->typeStr << "\\" << std::endl;
  else
    os << indent_str << "#Skipped" << std::endl;
}

BLIFPort::BLIFPort(std::string expr) { parseExpr(expr); }
void BLIFPort::parseExpr(std::string expr) {
  /*
    the syntax for inputs and outputs are as follows:
    expr -> expr | stmt     # examples of expr; in1:3 in2 : 2 in3: 1 in4: 1 in5
    stmt -> word | word(\\s*):(\\s*)digit   #ex of stmt; in1 :2  or in5
    word -> word    #Terminal ex; in1
    digit -> digit  #Terminal  ex; 22
  */
  std::size_t wspos = expr.find(' '); // first whitepace position
  std::size_t lkahdpos =
      expr.find_first_not_of(' ', wspos); // next not white space character

  if(expr[lkahdpos] == ':'){
    std::cout << "found :\n";
    std::cout << "Lookahead string is:" << expr.substr(lkahdpos + 1) << std::endl;
  }
}
