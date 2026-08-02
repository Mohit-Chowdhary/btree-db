#include <cstdint>
#include "node.h"
#ifndef WAL
#define WAL

struct WALRecord{
    int page_num;
    BNode data; 
    uint32_t checksum;
};

uint32_t compute_checksum(int page_num, const BNode& data);

bool wal_write(const char * wal_filename, int page_num, const BNode& data);

bool wal_read_and_validate(const char* wal_filename, WALRecord& out);

void wal_clear(const char* wal_filename);

#endif