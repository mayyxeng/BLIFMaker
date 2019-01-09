/** @file Node.cpp
 *  @brief Function and class method definitions for Node.h
 *
 *  This file contains function definitions for Node.h. The class BLIFCircuit
 * takes a graph representing the circuit and parses external attributes such as
 * i/o, type, etc..
 * @author Mahyar Emami (mayyxeng)
 * @bug setName and setType methods have redundant arguments
 */
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
    setName(n);
    // Set the node type
    setType(n);
    // Set the op of Operator types;
    setOp(n);
    // get input and output ports for each node
    getIOs(n);
  }
  // Traverse out edges to set connections
  for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n)) {
    for (Agedge_t *e = agfstout(graph, n); e; e = agnxtout(graph, e)) {

      //   std::string tailName = agnameof(agtail(e));
      //   std::string headName = agnameof(aghead(e));
      //   std::string tailPort = agget(e, "from");
      //   std::string headPort = agget(e, "to");
      //   NodeAttr_t *tailAttr = getAttributes(agtail(e));
      //   NodeAttr_t *headAttr = getAttributes(aghead(e));
      //   BLIFIO *from = tailAttr->outPort->getBLIFIOByName(tailPort);
      //   BLIFIO *to = headAttr->inPort->getBLIFIOByName(headPort);
      //   std::string connection = tailName + "." + tailPort + "*" +
      //                            tailAttr->typeStr + "*" + "~" + headName +
      //                            "." + headPort + "*" + headAttr->typeStr +
      //                            "*";
      //   std::string connection = genConnection(agtail(e), aghead(e));
      //   std::cout << "visiting edge from " << tailName << "(" << tailPort
      //             << ") to " << headName << "(" << headPort << ")" <<
      //             std::endl;
      //   if (!from) {
      //     std::cout << "\tInvalid tail " << tailPort << " of " << tailName
      //               << "\n";
      //   }
      //   if (!to) {
      //     std::cout << "\tInvalid head " << headPort << " of " << headName
      //               << "\n";
      //   }
      //   if (from && to) {
      //     from->connection = connection;
      //     to->connection = connection;
      //   } else {
      //     std::cerr << "Error: invalid edge connection from " << tailName <<
      //     "("
      //               << tailPort << ") to " << headName << "(" << headPort <<
      //               ")"
      //               << std::endl;
      //     exit(0);
      //   }
      genConnection(e);
    }
  }
}

void BLIFCircuit::genConnection(Agedge_t *e) {
  std::string tailName = agnameof(agtail(e));
  std::string headName = agnameof(aghead(e));
  std::string tailPort = agget(e, "from");
  std::string headPort = agget(e, "to");
  NodeAttr_t *tailAttr = getAttributes(agtail(e));
  NodeAttr_t *headAttr = getAttributes(aghead(e));
  BLIFIO *from = tailAttr->outPort->getBLIFIOByName(tailPort);
  BLIFIO *to = headAttr->inPort->getBLIFIOByName(headPort);
  std::string connection = tailName + "." + tailPort + "*" + tailAttr->typeStr +
                           "*" + "~" + headName + "." + headPort + "*" +
                           headAttr->typeStr + "*";
  std::cout << "visiting edge from " << tailName << "(" << tailPort << ") to "
            << headName << "(" << headPort << ")" << std::endl;
  if (!from) {
    std::cout << "\tInvalid tail " << tailPort << " of " << tailName << "\n";
  }
  if (!to) {
    std::cout << "\tInvalid head " << headPort << " of " << headName << "\n";
  }
  if (from && to) {
    from->connection = connection;
    to->connection = connection;
  } else {
    std::cerr << "Error: invalid edge connection from " << tailName << "("
              << tailPort << ") to " << headName << "(" << headPort << ")"
              << std::endl;
    exit(0);
  }
}
void BLIFCircuit::setName(Agnode_t *node) {
  NodeAttr_t *attributes = getAttributes(node);
  attributes->name = std::string(agnameof(node));
}
void BLIFCircuit::setType(Agnode_t *node) {
  std::string typeStr(agget(node, "type"));
  NodeAttr_t *attributes = getAttributes(node);
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
void BLIFCircuit::setOp(Agnode_t *node) {
  NodeAttr_t *attrs = getAttributes(node);
  if (attrs->type == Operator) {
    std::string opStr(agget(node, "op"));
    opStr.erase(remove(opStr.begin(), opStr.end(), ' '), opStr.end());
    Op op_ = isValidOp(opStr);
    if (op_ == _NullOp) {
      std::cerr << "Operator op can not be NULL\n";
      exit(1);
    } else if (op_ == _ErrorOp) {
      std::cerr << "Error: unkown op \"" << opStr << "\"\n";
      exit(1);
    } else {
      attrs->op = op_;
    }
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
BLIFCircuit::Op BLIFCircuit::isValidOp(std::string opStr) {
  if (opStr.empty())
    return _NullOp;
  for (int i = 0; i < 10; i++) {
    if (opStr == Op_str[i]) {
      return (Op)i;
    }
  }
  return _ErrorOp;
}
void BLIFCircuit::getIOs(Agnode_t *node) {
  // We need to check whether inputs are specified or not since an entry node
  // does not have any inputs and we should avoid potential run time erros
  Agsym_t *sym = agattrsym(node, "in");
  NodeAttr_t *attrs = getAttributes(node);
  attrs->inPort = nullptr;
  attrs->outPort = nullptr;
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
  /*
    A trail always terminates on stores, but this shouldn't happen with the BLIF
    because a STORE node could be regarded as general IO.
    What I am doing here could result in bugs, because attributeParse method may
    miss out on the new nodes!
  */
  if (attrs->op == store) {
    BLIFPort *port = new BLIFPort("out1", node, TRUE, channelWidth);
    attrs->outPort = port;
    std::cout << "Info: " << agnameof(node)
              << " is of op = store\n\tinferring outport\n";
    std::string storeOutName = agnameof(node);
    storeOutName = storeOutName + std::string("_lsq");
    // Create a node representing an exit point
    Agnode_t *storeOut = agnode(graph, (char *)storeOutName.c_str(), 1);
    // Append the necessary attributes
    NodeAttr_t *attrStoreOut = new NodeAttr_t;
    attrStoreOut =
        (NodeAttr_t *)agbindrec(storeOut, ATTR_STR, sizeof(NodeAttr_t), FALSE);
    agset(storeOut, "in", "in1");
    agset(storeOut, "type", "Exit");
    setName(storeOut);
    setType(storeOut);
    setOp(storeOut);
    // Create the edge between store and exit point
    Agedge_t *storeOutEdge = agedge(
        graph, node, storeOut, (char *)(storeOutName + "_edge").c_str(), 1);
    agset(storeOutEdge, "from", "out1");
    agset(storeOutEdge, "to", "in1");
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
  os << indent_str << ".model " << name << "\n";

  printCircuitIO(os, indent + 1);
  for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n)) {
    printSubckt(os, n, indent + 2);
  }
  os << indent_str << ".end\n";
}
void BLIFCircuit::printCircuitIO(std::ostream &os, int indent) {
  std::cout << "Printing node as entry node(input)\n";
  std::string indent_str = get_indent_string(indent);
  NodeAttr_t *attrs;
  os << ".inputs\\\n";
  for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n)) {
    attrs = getAttributes(n);
    if (attrs->type == Entry) {
      std::cout << "Found \"Entry\" (input) node " << agnameof(n);
      std::cout << " of width " << attrs->width << std::endl;
      auto ios = attrs->outPort->getIOPointer();
      for (auto iter = ios->begin(); iter != ios->end(); iter++) {
        os << indent_str << (*iter)->connection << " ";
      }
    }
  }

  os << "\n";
  os << ".outputs\\\n";
  for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n)) {
    attrs = getAttributes(n);
    if (attrs->type == Exit) {
      std::cout << "Found \"Exit\" (input) node " << agnameof(n);
      std::cout << " of width " << attrs->width << std::endl;
      auto ios = attrs->inPort->getIOPointer();
      for (auto iter = ios->begin(); iter != ios->end(); iter++) {
        os << indent_str << (*iter)->connection << " ";
      }
    }
  }
  os << "\n";
}

void BLIFCircuit::printSubckt(std::ostream &os, Agnode_t *model, int indent) {
  std::string indent_str = get_indent_string(indent);
  std::string indent_str_inner = get_indent_string(indent + 1);
  NodeAttr_t *attrs = getAttributes(model);
  os << indent_str << "#Node " << attrs->name << std::endl;
  if (attrs->valid) {
    os << indent_str << ".subckt " << attrs->typeStr << "\\" << std::endl;
    BLIFPort *inPort = attrs->inPort;

    for (auto io : *inPort->getIOPointer()) {
      if (!io->connection.empty())
        os << indent_str_inner << io->name << "=" << io->connection << " ";
    }

    BLIFPort *outPort = attrs->outPort;
    for (auto io : *outPort->getIOPointer()) {
      if (!io->connection.empty())
        os << indent_str_inner << io->name << "=" << io->connection << " ";
    }
    os << std::endl;
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

  if (wspos == std::string::npos || wspos == expr.size() - 1) {
    // Reached the last stmnt
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
  stmnt.erase(remove(stmnt.begin(), stmnt.end(), '+'), stmnt.end());
  stmnt.erase(remove(stmnt.begin(), stmnt.end(), '-'), stmnt.end());
  stmnt.erase(remove(stmnt.begin(), stmnt.end(), '?'), stmnt.end());
  if (stmnt.empty())
    return;
  std::cout << "Parsing statement: " << stmnt << std::endl;
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

void BLIFCircuit::makeFork2Single(Agnode_t *node, int level, int target_fanout,
                                  int index, std::string base_name,
                                  Agnode_t *original_fork,
                                  std::vector<Agnode_t *> &successors) {

  std::cout << "Target fanout is " << target_fanout << std::endl;
  if (target_fanout >= 2) {
    std::cout << "Creating fork2 node " << std::endl;
    std::string new_name =
        base_name + "_l" + std::to_string(level) + "_c" + std::to_string(index);

    // create a node to replace the old fork node with big fanout

    Agnode_t *new_node = agnode(graph, (char *)new_name.c_str(), true);
    // Append attribtues to the new node
    NodeAttr_t *new_attr = new NodeAttr_t;
    new_attr =
        (NodeAttr_t *)agbindrec(new_node, ATTR_STR, sizeof(NodeAttr_t), FALSE);
    std::cout << "\tsetting in1 input" << std::endl;
    agset(new_node, "in", "in1");
    std::cout << "\tsetting out1 out2 outputs" << std::endl;
    agset(new_node, "out", "out1 out2");
    std::cout << "\tsetting type to Fork" << std::endl;
    agset(new_node, "type", "Fork");
    std::cout << "\tsetting name to " << new_name << std::endl;
    setName(new_node);
    std::cout << "\tsetting enum type to Fork" << std::endl;
    setType(new_node);

    new_attr->outPort =
        new BLIFPort(std::string("out1 out2"), new_node, TRUE, channelWidth);
    new_attr->inPort =
        new BLIFPort(std::string("in1"), new_node, FALSE, channelWidth);

    std::cout << "New node generation succeeded" << std::endl;
    // Creating an edge between the new node and node.
    // NOTE: here it is assumed that the original dot file does not have fork
    // trees in other words, a fork can not have a fork predecessor in the
    // orignal graph
    auto new_edge = agedge(graph, node, new_node,
                           (char *)(new_name + "_edge").c_str(), TRUE);
    agset(new_edge, "to", "in1");
    if (level == 0) {
      std::cout << "Connecting predecessor to new fork root" << std::endl;
      auto edge = agedge(graph, node, original_fork, NULL, FALSE);
      std::string origin_port = agget(edge, "from");
      agset(new_edge, "from", (char *)origin_port.c_str());

    } else {
      std::cout << "Creating new edges in the fork tree" << std::endl;
      std::string from_port = "out" + std::to_string(index + 1);
      agset(new_edge, "from", (char *)from_port.c_str());
    }
    std::cout << "edge " << agnameof(new_edge) << " created" << std::endl;
    std::cout << "\t with tail " << agnameof(agtail(new_edge)) << std::endl;
    std::cout << "\t and head " << agnameof(aghead(new_edge)) << std::endl;
    genConnection(new_edge);

    // Recurse
    makeFork2Single(new_node, level + 1, target_fanout / 2, 0, base_name,
                    original_fork, successors);
    makeFork2Single(new_node, level + 1, target_fanout - target_fanout / 2, 1,
                    base_name, original_fork, successors);

  } else {
    std::cout << "Reached leaf of the fork tree" << std::endl;
    if (successors.size()) {
      auto successor = successors.back();
      std::string edge_name = agnameof(node) + std::string("_edge");
      auto edge = agedge(graph, original_fork, successor, NULL, FALSE);
      //std::string successor_port = agget(edge, "to");
      auto new_edge = agedge(graph, node, successors.back(),
                         (char *)edge_name.c_str(), TRUE);
      std::string port_name = "out" + std::to_string(index + 1);
      agset(new_edge, "from", (char*)port_name.c_str());
      agset(new_edge, "to", agget(edge, "to"));
      genConnection(new_edge);
      successors.pop_back();
      std::cout << successors.size() << " successors are left" << std::endl;

    } else {
      std::cout << "Something is wrong." << std::endl;
    }
  }

  // An edge is connected from node to new_node
  // setOp(root);
  // for (Agedge_t *e = agfstout(graph, node); e; e = agnxtout(graph, e)) {
  //   auto head = aghead(e);
  //   std::cout << "diconnecting " << agnameof(node) << " from " <<
  //   agnameof(head)
  //             << std::endl;
  // }
}

void BLIFCircuit::makeFork2() {

  for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n)) {
    auto attr = getAttributes(n);
    if (attr->type == Fork && attr->valid && attr->outPort->ioCount() > 2) {
      std::cout << "Found fork node " << agnameof(n) << " of size "
                << attr->outPort->ioCount() << " > 2" << std::endl;
      // delete an edge by calling agdeledge(graph, e)
      // delete a node by calling agdelnode(graph, n)
      std::cout << "Node " << agnameof(agtail(agfstin(graph, n)))
                << " is the predecessor" << std::endl;
      auto predecessor = agtail(agfstin(graph, n));

      std::vector<Agnode_t *> successors;
      for (Agedge_t *e = agfstout(graph, n); e; e = agnxtout(graph, e))
        successors.push_back(aghead(e));
      makeFork2Single(predecessor, 0, attr->outPort->ioCount(), 0,
                      std::string(agnameof(n)), n, successors);
      attr->valid = FALSE;
      std::cout << "Fork transformation generation succeeded" << std::endl;
    }
  }
}
