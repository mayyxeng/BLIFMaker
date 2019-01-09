/** @file Node.cpp
 *  @brief Function and class method definitions for Node.h
 *
 *  This file contains function definitions for Node.h. The class BLIFCircuit
 * takes a graph representing the circuit and parses external attributes such as
 * i/o, type, etc..
 * @author Mahyar Emami (mayyxeng)
 * @bug setName and setType methods have redundant arguments
 */
#ifndef __BLIFMAKER_GRAPH_H__
#define __BLIFMAKER_GRAPH_H__

#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#include <iostream>
#include <string>
#include <vector>

#define ATTR_STR "attribtues"

Agraph_t *parseDotFile(std::string filePath, int verbosity);
std::string get_indent_string(int indent);
class BLIFPort;

class BLIFCircuit {

public:
  enum Type {
    Operator,
    Buffer,
    Constant,
    Fork,
    Merge,
    Select,
    Branch,
    Demux,
    Entry,
    Exit,
    _Error,
    _Null
  };
  enum Op { load, store, mul, add, icmp, sub, and_, _ErrorOp, _NullOp };
  std::string Type_str[10] = {"Operator", "Buffer", "Constant", "Fork",
                              "Merge",    "Select", "Branch",   "Demux",
                              "Entry",    "Exit"};
  std::string Op_str[7] = {"load", "store", "mul", "add", "icmp", "sub", "and"};
  typedef struct NodeAttr_t {
    Agrec_t h;
    std::string name;
    int width;
    Type type;
    Op op;
    std::string typeStr;
    bool valid;
    BLIFPort *inPort;
    BLIFPort *outPort;

  } NodeAttr_t;

  BLIFCircuit(Agraph_t *g, std::string name) : graph(g), name(name){};
  void parseAttributes();
  void printCircuit(std::ostream &os, int indent = 0);
  /** Architecture specific transformations start */
  /** @brief makes every fork in the circuit a fork2 */
  void makeFork2Single(Agnode_t *node, int level, int target_fanout, int index,
                       std::string base_name, Agnode_t *original_fork,
                       std::vector<Agnode_t *> &successors);
  void makeFork2();
  /** Architecture specific transformations end */
private:
  std::string name;
  int channelWidth;
  Agraph_t *graph;

  void printCircuitIO(std::ostream &os, int indent);
  void printSubckt(std::ostream &os, Agnode_t *model, int indent);
  void printBlackBoxes(std::ostream &os){};

  void setName(Agnode_t *node);
  void setType(Agnode_t *node);
  void setOp(Agnode_t *node);
  void getIOs(Agnode_t *node);
  void genConnection(Agedge_t *e);

  NodeAttr_t *getAttributes(Agnode_t *node);
  Type isValidType(std::string typeStr);
  Op isValidOp(std::string opStr);
};

struct BLIFIO {
  // bool mode;
  int width;
  std::string name;
  std::string connection;
};
class BLIFPort {
public:
  bool mode;
  BLIFPort(std::string expr, Agnode_t *n, bool _mode, int _defWidth = 32);
  std::vector<BLIFIO *> *getIOPointer() { return io; };
  int getDefaultWidth() { return defWidth; };
  BLIFIO *getBLIFIOByName(std::string name);
  int ioCount() { return io->size(); }

private:
  void parseExpr(std::string expr);
  void parseStmnt(std::string stmnt);
  bool isValidNumber(std::string s);

  std::string name;

  int defWidth;
  std::vector<BLIFIO *> *io;
  Agnode_t *node;
};
#endif //__BLIFMAKER_GRAPH_H__
