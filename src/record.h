#include <iostream>
#ifndef RECORD_H
#define RECORD_H

const int RECORD_SIZE = 64;

struct RecordManager{
    FILE* file;
    int total_records;
};

RecordManager* rm_open(const char* filename);
int rm_insert(RecordManager* rm, const char* data, int len);
void rm_get(RecordManager* rm, int slot);


#endif