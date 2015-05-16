#ifndef NODE_H_INCLUDED
#define NODE_H_INCLUDED
#include <vector>
#include "glm/glm.hpp"


class Node {
private:
  glm::mat3x3  lcs;
  std::vector<Node> children;
}


#endif