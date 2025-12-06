#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Должно быть больше аргументов\n");
    return 1;
  }

  char *substring = argv[1];
  char *filename = argv[2];

  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("grep");
    return 1;
  }

  char line[1000];


  while (fgets(line, sizeof(line), file)) {
    int len_line = strspn(line, "\n");
    int f = 1;
    putchar('0');
    for (int i = 0; i <= len_line; i++) {
	char c1 = substring[i];
	char c2 = filename[i];
	int a1 = (int)'A';
	int a2 = (int)'a';
	int z1 = (int)'Z';
	int b1 = (int)c1;
	int b2 = (int)c2;
	putchar('1');
	if (b1 > a1  && b1 < z1) { 
		c1 = (char)(b1 - a1 + a2);
		putchar('2');
	}
	if (b2 > a1  && b2 < z1) { 
		c2 = (char)(b2 - a1 + a2);
		putchar('3');
	}
	if ((int)c1 != (int)c2){
		putchar('4');
		f = 0;
		break;
	}

    }
    if (f) {
      printf("%s\n", line);
      putchar('5');
    }

    
  }

  fclose(file);
  return 0;
}
