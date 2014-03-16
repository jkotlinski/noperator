#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc < 5 || strcmp(argv[1], "-o")) {
        puts("Usage: linker -o anim.out [source anims]");
        return 1;
    }

    FILE* out = fopen(argv[2], "wb");
    if (!out) {
        printf("Can't open %s\n", argv[2]);
        return 1;
    }

    char x = 0;
    char y = 0;

    for (int i = 3; i < argc; ++i) {
        FILE* in = fopen(argv[i], "rb");
        if (!in) {
            puts("Can't open %s!");
            return 0;
        }
        printf("%s...", argv[i]);

        while (1) {
            int ch = fgetc(in);
            if (ch == EOF) {
                break;
            }

            switch (ch) {
                case 0:  // RLE.
                    {
                        fgetc(in);
                        fgetc(in);
                    }
                    break;
                case 0x13:  // Home.
                    fgetc(in);
                    fgetc(in);
                    break;
                default:
                    fputc(ch, out);
                    break;
            }
        }

        fclose(in);
    }

    fclose(out);

    puts("ok!");

    return 0;
}
