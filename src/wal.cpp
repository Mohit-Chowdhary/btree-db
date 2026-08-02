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

bool wal_write(const char * wal_filename, int page_num, const BNode& data){
    FILE* file = fopen(wal_filename,"wb");
    if(!file) return false;

    WALRecord rec{
        page_num,
        data,
        compute_checksum(page_num, data)
    };

    size_t written = fwrite(&rec, sizeof(WALRecord), 1, file);
    fflush(file);
    fclose(file);

    return written == 1;
}

bool wal_read_and_validate(const char* wal_filename, WALRecord& out){
    FILE* file = fopen(wal_filename, "rb");
    if(!file) return false; // nuth to recover

    size_t read = fread(&out, sizeof(WALRecord), 1, file);
    fclose(file);

    if(read != 1) return false;

    uint32_t expected = compute_checksum(out.page_num, out.data);
    if(expected != out.checksum) return false;

    return true;
}

void wal_clear(const char* wal_filename){
    FILE* file = fopen(wal_filename, "wb"); // clears file
    if(file) fclose(file);
}