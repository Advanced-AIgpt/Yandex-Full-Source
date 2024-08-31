#ifndef MEM_BLOB_H
#  define MEM_BLOB_H

#include <stddef.h>

typedef struct mem_blob {
    void *data;
    size_t size;
} mem_blob_t;

typedef struct fst_data {
    const char *fst_name;
    mem_blob_t blob;
} fst_data_t;

#endif
