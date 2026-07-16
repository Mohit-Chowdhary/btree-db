#ifndef META_D
#define META_D

struct Meta{
    int root_page;
};

Meta read_meta(const char* filename);
void write_meta(const char* filename, const Meta& meta);
#endif