#!/usr/bin/env python3

import argparse
import json
import sys
import xlrd


TRANS_COLS = {'Content Name': 'name', 'Content Uuid': 'provider_item_id',
              'Seasons Count': 'seasons_count', 'Content Type Name': 'type'}
TRANS_TYPE = {'ott-episode': 'tv_show', 'ott-movie': 'movie'}


def add_all_items(sheet, all_items):
    # The top row of export files is a header row.
    col_order = [sheet.cell(0, col).value for col in range(sheet.ncols)]

    for row in range(1, sheet.nrows):
        item = dict()
        is_valid = True

        for col in range(sheet.ncols):
            field_name = TRANS_COLS.get(col_order[col])
            if field_name:
                value = sheet.cell(row, col).value
                if not value:
                    is_valid = False
                    print('Sheet %s, %d:%d: value is empty' % (sheet.name, row + 1, col + 1),
                          file=sys.stderr)
                    break

                if field_name == 'type':
                    value = TRANS_TYPE[value]
                elif field_name == 'seasons_count':
                    value = int(value)

                item[field_name] = value

        if is_valid:
            item['provider_name'] = 'kinopoisk'
            all_items += [{'item': item}]


if __name__ == '__main__':
    arg_parser = argparse.ArgumentParser(description='XLSX-to-JSON check list converter')
    arg_parser.add_argument('--in-table', type=str, help='Input XLSX table containing check items')
    args = arg_parser.parse_args()

    kp_table = xlrd.open_workbook(args.in_table, formatting_info=False)

    all_items = []
    for sheet in kp_table.sheets():
        add_all_items(sheet, all_items)

    result = {'single_items': all_items}
    print(json.dumps(result, sort_keys=True, indent=2, ensure_ascii=False))
