#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void hexdump(int p, int q, char *s)
{
  int i;
  for (i = 0; i < p; i++) {
    fprintf(stdout, "0x%02x,", (int)s[i]&255);
  }
}

int main(int argc, char **argv)
{
  FILE *f;
  char row[16];
  int i, j, c;

  if (argc != 2) {
    fprintf(stderr, "syntax: hexdump <filename> {0xSTART 0xFINISH}\n");
    exit(0);
  }
  f = fopen(argv[1], "rb");
  j = 0;
  fprintf(stdout, "{\n");
  for (;;) {
    for (i = 0; i < 16; i++) {
      c = fgetc(f);
      if (c == EOF) break;
      row[i] = c;
    }
    if ((c == EOF) && (i == 0)) break;
    fprintf(stdout, " /* %04x */ ", j);
    hexdump(i, j, row);
    fprintf(stdout, "\n");
    if (c == EOF) break;
    j += 16;
  }
  fprintf(stdout, "};\n");
}
