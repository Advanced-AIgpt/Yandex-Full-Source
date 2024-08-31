#!/usr/bin/env python
# encoding: utf-8
from itertools import islice, izip
from openpyxl import load_workbook
from pandas import DataFrame, read_excel


def load_sheets(path):
    wb = load_workbook(path)
    return {ws.title: sheet_to_dataframe(ws)
            for ws in wb.worksheets}


def sheet_to_dataframe(ws):
    data = ws.values
    cols = next(data)[1:]
    data = list(data)
    idx = [r[0] for r in data]
    data = (islice(r, 1, None) for r in data)
    return DataFrame(data, index=idx, columns=cols)


def get_categories(wb):
    pass


def main_xlsx():
    import os
    path = os.path.expanduser('~/Voice/tasks/DIALOG-950/hints.xlsx')
    dframes = load_sheets(path)
    cats = dframes['categories']
    print cats.get_values()


# ==========================


def group_cats(hints_path='tmp/my_hints.tsv', cat_path='tmp/my_cats.tsv'):
    cats = (line.strip().split('\t') for line in open(cat_path))
    next(cats)
    groups = {int(idx): {'description': unicode(description, 'utf-8'),
                         'examples': []}
                for idx, description in cats}

    hints = (line.strip().split('\t') for line in open(hints_path))
    head = next(hints)
    for hint in hints:
        row = {field: unicode(val, 'utf-8')
               for field, val in izip(head, hint)}
        for idx in row['category'].split(';'):
            cat = groups[int(idx)]
            cat['examples'].append(row)

    return groups


def print_to_wiki(groups):
    for key, cat in sorted(groups.iteritems()):
        print u'{key}. {descr}\n#|'.format(key=key, descr=cat['description'])
        print u'|| **dialog** | **toloka** | **reference** | **solution** | **comment** ||'
        for row in cat['examples']:
            fmt = u'|| - {0[context_2]}\n - {0[context_1]}\n - {0[context_0]}\n - {0[reply]} | {0[toloka_result]} | {0[reference_result]} | {0[solution]} | {0[comment]} ||'
            print fmt.format(row)
        print u'|#\n'


def main():
    print_to_wiki(group_cats())


if __name__ == '__main__':
    main()
