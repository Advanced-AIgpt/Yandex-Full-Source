/**
 * @file util/ofst-symbol-table.h
 *
 * @date 2015
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2014 Yandex. All rights reserved.
 *
 * Using OpenFST symbol tables.
 */

#ifdef OFST_SYMBOL_TABLE_H
#  error "ofst-symbol-table.h: double inclusion"
#endif
#define OFST_SYMBOL_TABLE_H

#include "mem_blob.h"

#ifdef __cplusplus
extern "C" {
#endif

/* !!! Only "dense" symbol tables are supported. */

typedef struct ofst_symbol_table ofst_symbol_table_t;

ofst_symbol_table_t *ofst_symbol_table_read(char *fname);
ofst_symbol_table_t *ofst_symbol_table_read_from_blob(mem_blob_t blob);
void ofst_symbol_table_free(ofst_symbol_table_t *ost);

int ofst_symbol_table_size(ofst_symbol_table_t *ost);

char *ofst_symbol_table_symbol(ofst_symbol_table_t *ost, int code);
int ofst_symbol_table_code(ofst_symbol_table_t *ost, char *symbol); // -1 when not found.

#ifdef __cplusplus
}
#endif
