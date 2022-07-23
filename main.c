#include "9cc.h"

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  Token *token = tokenize(argv[1]);
  Node *node = parse(argv[1], token);
  codegen(node);
  return 0;
}
