#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>
int main(int argc, char **argv) {
  int i, j, k, c, rc;
  FILE *rom[4];
  if (argc != 5) {
    fprintf(stderr, "syntax: combine file.t7 file.p7 file.u7 file.r7 > file.rom\n");
    exit(EXIT_FAILURE);
  }
  for (i = 1; i <= 4; i++) {
    rom[i-1] = fopen(argv[i], "rb");
    if (rom[i-1] == NULL) {
      fprintf(stderr, "combine: cannot open \'%s\' - %s\n", argv[i], strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
  for (i = 0; i < 4; i+=2) {
    for (j = 0; j < 2048; j++) {
      for (k = 0; k <= 1; k++) {
        c = fgetc(rom[i+k]);
        if (c == EOF) {
          fprintf(stderr, "combine: file too small - \'%s\'\n", argv[i+k+1]);
          exit(EXIT_FAILURE);
        }
        rc = fputc(c, stdout);
        if (rc == EOF)  {
          fprintf(stderr, "combine: cannot write to \'%s\' - %s\n", argv[i+k+1], strerror(errno));
          exit(EXIT_FAILURE);
        }
      }
    }
    for (k = 0; k <= 1; k++) {
      if (fgetc(rom[i+k]) != EOF) {
        fprintf(stderr, "combine: WARNING: excess data in \'%s\'\n", argv[i+k+1]);
        exit(EXIT_FAILURE);
      }
      fclose(rom[i+k]);
    }
  }
  rc = fclose(stdout);
  if (rc != 0)  {
    fprintf(stderr, "combine: cannot write output - %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
  return 0;
}
