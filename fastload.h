void __fastcall__ loader_init(void);
void __fastcall__ loader_load(const char* filename);
void __fastcall__ loader_open(const char* filename);
int __fastcall__ loader_getc(void);  /* -1 = eof */
