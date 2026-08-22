#include "wal.h"
#include <cstdio>
#include <cstring>

uint32_t compute_checksum(int page_num,const BNode& data){
    uint32_t sum = 0;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(&page_num);
    for(size_t i=0; i<sizeof(page_num); i++) sum+=p[i];

    const unsigned char* d = reinterpret_cast<const unsigned char*>(&data);
    for(size_t i=0; i<sizeof(BNode); i++) sum+=d[i];

    return sum;
}

bool wal_write(FILE* wal_file, int page_num, const BNode& data){
    WALRecord rec{
        page_num,
        data,
        compute_checksum(page_num, data),
        true
    };

    fseek(wal_file, 0, SEEK_SET);
    size_t written = fwrite(&rec, sizeof(WALRecord), 1, wal_file);
    fflush(wal_file);
    return written == 1;
}

void wal_clear(FILE* wal_file){
    fseek(wal_file, 0, SEEK_SET);
    bool invalid = false;
    fwrite(&invalid, sizeof(bool), 1, wal_file); // stomp just the valid flag
    fflush(wal_file);
}

bool wal_read_and_validate(FILE* wal_file, WALRecord& out){
    fseek(wal_file, 0, SEEK_SET);
    size_t read = fread(&out, sizeof(WALRecord), 1, wal_file);
    if(read != 1 || !out.valid) return false;

    uint32_t expected = compute_checksum(out.page_num, out.data);
    return expected == out.checksum;
}