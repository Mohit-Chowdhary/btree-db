#include <cstdint>
#include "node.h"
#ifndef WAL
#define WAL

struct WALRecord{
    int page_num;
    BNode data; 
    uint32_t checksum;
    bool valid = false;
};

uint32_t compute_checksum(int page_num, const BNode& data);

bool wal_write(FILE* wal_file, int page_num, const BNode& data);
bool wal_read_and_validate(FILE* wal_file, WALRecord& out);
void wal_clear(FILE* wal_file);

#endif