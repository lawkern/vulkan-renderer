/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#define S(Literal) (string){sizeof(Literal)-1, (u8 *)Literal}

typedef struct {
   size Length;
   u8 *Data;
} string;

typedef struct {
   string Before;
   string After;
   bool Found;
} cut;
