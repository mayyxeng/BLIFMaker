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
    // Set the node name
    setName(n, attr);
    // Set the node type
    setType(n, attr);
    // get input and output ports for each node
    getIOs(n);
  }
  // Traverse out edges to set connections
  for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n)) {
    for (Agedge_t *e = agfstout(graph, n); e; e = agnxtout(graph, e)) {

      std::string tailName = agnameof(agtail(e));
      std::string headName = agnameof(aghead(e));
      std::string tailPort = agget(e, "from");
      std::string headPort = agget(e, "to");
      NodeAttr_t *tailAttr = getAttributes(agtail(e));
      NodeAttr_t *headAttr = getAttributes(aghead(e));
      BLIFIO *from = tailAttr->outPort->getBLIFIOByName(tailPort);
      BLIFIO *to = headAttr->inPort->getBLIFIOByName(headPort);
      std::string connection =
          tailName + "_" + tailPort + "_to_" + headName + "_" + headPort;

      if (from && to) {
        std::cout << "visited edge from " << tailName << "(" << tailPort
                  << ") to " << headName << "(" << headPort << ")" << std::endl;
        from->connection = connection;
        to->connection = connection;
      } else {
        std::cerr << "Error: invalid edge connection from " << tailName << "("
                  << tailPort << ") to " << headName << "(" << headPort << ")"
                  << std::endl;
        exit(0);
      }
    }
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
void BLIFCircuit::getIOs(Agnode_t *node) {
  // We need to check whether inputs are specified or not since an entry node
  // does not have any inputs and we should avoid potential run time erros
  Agsym_t *sym = agattrsym(node, "in");
  NodeAttr_t *attrs = getAttributes(node);
  if ((sym && attrs->valid) || (sym && attrs->type == Exit)) {
    std::string inputExpression((char *)agget(node, "in"));
    // std::string inputExpression(
    //     std::string(" in1 :s1 in2 in3: 3 in4 : 10 in5"));
    std::cout << "found input expression of \"" << inputExpression
              << "\" for node " << agnameof(node) << std::endl;
    BLIFPort *port = new BLIFPort(inputExpression, node, FALSE, channelWidth);
    attrs->inPort = port;
  }

  sym = agattrsym(node, "out");
  if ((sym && attrs->valid) || (sym && attrs->type == Entry)) {
    std::string outputExpression((char *)agget(node, "out"));
    std::cout << "found output expression of \"" << outputExpression
              << "\" for node " << agnameof(node) << std::endl;
    BLIFPort *port = new BLIFPort(outputExpression, node, TRUE, channelWidth);
    attrs->outPort = port;
  }
}

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
  std::string indent_str_inner = get_indent_string(indent + 1);
  NodeAttr_t *attrs = getAttributes(model);
  os << indent_str << "#Node " << attrs->name << std::endl;
  if (attrs->valid) {
    os << indent_str << ".model " << attrs->typeStr << "\\" << std::endl;
    BLIFPort *inPort = attrs->inPort;
    os << indent_str << "#Inputs\n";
    for (auto io : *inPort->getIOPointer()) {
      os << indent_str_inner << io->name << "=" << io->connection << "\\\n";
    }
    BLIFPort *outPort = attrs->outPort;
    os << indent_str << "#Outputs\n";
    for (auto io : *outPort->getIOPointer()) {
      os << indent_str_inner << io->name << "=" << io->connection << "\\\n";
    }
  } else
    os << indent_str << "#Skipped" << std::endl;
}

BLIFPort::BLIFPort(std::string expr, Agnode_t *n, bool _mode, int _defWidth) {
  mode = _mode;
  defWidth = _defWidth;
  node = n;
  io = new std::vector<BLIFIO *>;
  parseExpr(expr);
}
void BLIFPort::parseExpr(std::string expr) {
  /*
    the syntax for inputs and outputs are as follows:
    expr -> expr | stmnt     # examples of expr; in1:3 in2 : 2 in3: 1 in4: 1 in5
    stmnt -> word | word(\\s*):(\\s*)digit   #ex of stmt; in1 :2  or in5
    word -> word    #Terminal ex; in1
    digit -> digit  #Terminal  ex; 22
  */

  // remove redundant whitespaces in the begining
  std::size_t strtpos = expr.find_first_not_of(' ');
  expr.erase(0, strtpos);
  // try to find a stmnt
  std::size_t wspos = expr.find(' '); // first whitepace position
  // std::cout << "Parsing expression:" << expr << std::endl;
  // std::cout << "wspos: " << wspos << std::endl;
  // std::cout << "size: " << expr.size() << std::endl;
  if (wspos == std::string::npos || wspos == expr.size() - 1) {
    // Reached the last stmnt
    // std::cout << "Expression is a statement \n";
    parseStmnt(expr);
  } else {
    std::string stmnt;
    std::size_t lkhdpos =
        expr.find_first_not_of(' ', wspos); // next not white space character
    std::size_t clnpos = expr.find(':');
    // std::cout << "lkhdpos: " << lkhdpos << std::endl;
    // std::cout << "clnpos: " << clnpos << std::endl;
    if (lkhdpos >= clnpos) {
      // Look for digits
      std::size_t dgtstrtpos = expr.find_first_of("0123456789", clnpos);

      std::size_t dgtndpos =
          expr.find_first_not_of("0123456789", dgtstrtpos + 1);

      // std::cout << "dgtstrtpos: " << dgtstrtpos << std::endl;
      // std::cout << "dgtndpos: " << dgtndpos << std::endl;
      if (dgtndpos <= dgtstrtpos) {
        std::cerr << "Error: expected decimal width for node " << agnameof(node)
                  << std::endl;
        exit(0);
      }

      std::size_t nxtpos = expr.find_first_not_of(' ', dgtndpos + 1);
      stmnt = expr.substr(0, dgtndpos);
      expr.erase(0, nxtpos - 1);
      // std::cout << "stmt:" << stmnt << std::endl;
      // std::cout << "expr:" << expr << std::endl;
      parseStmnt(stmnt);
      parseExpr(expr);
    } else {
      // Look for a word
      stmnt = expr.substr(0, lkhdpos - 1);
      expr.erase(0, lkhdpos - 1);
      // std::cout << "stmt:" << stmnt << std::endl;
      // std::cout << "expr:" << expr << std::endl;
      parseStmnt(stmnt);
      parseExpr(expr);
    }
  }
}

void BLIFPort::parseStmnt(std::string stmnt) {

  // remove all the whitepaces
  stmnt.erase(remove(stmnt.begin(), stmnt.end(), ' '), stmnt.end());
  if (stmnt.empty())
    return;
  // std::cout << "Parsing statement: " << stmnt << std::endl;
  std::size_t clnpos = stmnt.find(':');
  std::string portName;
  BLIFIO *newIO = new BLIFIO;
  if (clnpos == std::string::npos) {
    // This is the case when width is unspecified
    newIO->name = stmnt;
    newIO->width = defWidth;

  } else {

    newIO->name = stmnt.substr(0, clnpos);
    if (isValidNumber(stmnt.substr(clnpos + 1))) {
      std::istringstream conv(stmnt.substr(clnpos + 1));
      int w;
      conv >> w;
      newIO->width = w;
    } else {
      std::cerr << "Error: \"" << stmnt.substr(clnpos + 1)
                << "\" is not a number\n";
      exit(0);
    }
  }
  io->push_back(newIO);
  // std::cout << "port name is " << newIO->name << std::endl;
}
bool BLIFPort::isValidNumber(std::string s) {
  // std::cout << "checking validity of " << s << std::endl;
  std::string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it))
    ++it;
  return !s.empty() && it == s.end();
}

BLIFIO *BLIFPort::getBLIFIOByName(std::string name) {
  for (auto &ioObj : *io) {
    if (ioObj->name == name)
      return ioObj;
  }
  return NULL;
}
