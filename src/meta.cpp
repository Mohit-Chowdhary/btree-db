#include "meta.h"
#include <cstdio>

Meta read_meta(const char* filename){
    Meta meta{0};
    FILE* f = fopen(filename,"rb");
    if(f){
        fread(&meta, sizeof(Meta),1,f);
        fclose(f);
    }
    return meta;
}

void write_meta(const char* filename, const Meta& meta){
    FILE* f = fopen(filename, "wb");
    if(f){
        fwrite(&meta, sizeof(Meta), 1, f);
        fflush(f);
        fclose(f);
    }
}