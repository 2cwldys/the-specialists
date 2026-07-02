#include <cstddef>
#include <cstdio>
typedef int string_t; typedef float vec_t;
struct Vector { float x,y,z; };
typedef unsigned char byte; typedef struct edict_s edict_t;
typedef Vector vec3_t;
#include "/home/stevenlafl/Projects/goldsrc.net/mod-ts/engine/progdefs.h"
int main(){
#define P(f) printf("%#lx %s\n", offsetof(entvars_t,f), #f);
P(gamestate) P(oldbuttons) P(groupinfo) P(iuser1) P(iuser2) P(iuser3) P(iuser4) P(fuser1)
return 0;}
