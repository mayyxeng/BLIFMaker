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
  std::string Type_str[10] = {
      "Operator", "Buffer", "Constant", "Fork",  "Merge",
      "Select",   "Branch", "Demux",    "Entry", "Exit"};
  typedef struct NodeAttr_t {
    Agrec_t h;
    std::string name;
    int width;
    Type type;
    std::string typeStr;
    bool valid;
    BLIFPort *inPort;
    BLIFPort *outPort;

  } NodeAttr_t;


  BLIFCircuit(Agraph_t *g, std::string name) : graph(g), name(name){};

  // private:
  std::string name;
  int channelWidth;
  Agraph_t *graph;

  // bool fileOpen;
  void parseAttributes();
  void printCircuit(std::ostream &os, int indent = 0);
  void printModel(std::ostream &os, Agnode_t *model, int indent);
  void printIO(std::ostream &os, int indent = 0);
  void setName(Agnode_t *node, NodeAttr_t *attribtues);
  void setType(Agnode_t *node, NodeAttr_t *attribtues);
  void getIOs(Agnode_t *node);
  //void getOutputs(Agnode_t *node, NodeAttr_t *attribtues);

  //TODO: make this safe by using const
  NodeAttr_t *getAttributes(Agnode_t *node);
  Type isValidType(std::string typeStr);

};

class BLIFModel {
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
    Exit
  };

  // static const std::string Type_str[10] = {
  //     "Operator", "Buffer", "Constant", "Fork",  "Merge",
  //     "Select",   "Branch", "Demux",    "Entry", "Exit"};
  BLIFModel(Agnode_t *node){};

private:
  std::string name;
};

struct BLIFIO{
  //bool mode;
  int width;
  std::string name;
  std::string connection;

};
class BLIFPort {
public:
  bool mode;
  BLIFPort(std::string expr, Agnode_t *n, bool _mode, int _defWidth=32);
  std::vector<BLIFIO*>* getIOPointer(){return io;};
  int getDefaultWidth(){return defWidth;};
  BLIFIO *getBLIFIOByName(std::string name);
private:
  void parseExpr(std::string expr);
  void parseStmnt(std::string stmnt);
  bool isValidNumber(std::string s);


  std::string name;

  int defWidth;
  std::vector<BLIFIO*> *io;
  Agnode_t *node;
};
#endif //__BLIFMAKER_GRAPH_H__
