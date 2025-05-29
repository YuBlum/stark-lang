#include <stdio.h>

int
main(int argc, const char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  stark <file>\n");
  }
  const char *file = argv[1];
  printf("%s\n", file);
  return 0;
}
