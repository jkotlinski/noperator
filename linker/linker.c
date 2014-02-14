#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc < 4 || strcmp(argv[1], "-o")) {
        puts("Usage: linker -o anim.out [source anims]");
        return 0;
    }
    for (int i = 1; i < argc; ++i)
        puts(argv[i]);

    return 0;
}
