#!/usr/bin/env python
# encoding: utf-8


phrases = {
    'headers': 'bin_0	bin_1	bin_2_5	bin_5_10	bin_11_20	bin_21_30	bin_31_50	bin_51_100	bin_101',
    'columns': ['2579	3381	4671	2563	1599	683	627	499	201',
                '3022	4051	5512	2771	1575	704	690	526	217'],
    'totals': [16136, 18329],
}

minutes = {
    'headers': 'bin_1	bin_1_3	bin_3_5	bin_5_10	bin_10_15	bin_15_30	bin_30_60	bin_60_120	bin_120',
    'columns': ['9599	1600	988	1330	726	1228	540	117	8',
                '11616	1620	926	1344	796	1319	605	99	4'],
    'totals': [16136, 18329]
}


def to_perc_wiki_table(headers, columns, totals):
    headers = ['..'.join(h.lstrip('bin_').split('_'))
               for h in headers.split()]
    columns = map(str.split, columns)

    if totals is not None:
        assert len(columns) == len(totals), 'inconsistent number of columns'
    assert len(headers) == len(columns[0]), 'inconsistent number of rows'

    print '#|'
    for row_idx, hdr in enumerate(headers):
        print ('|| **%s** ' % hdr),

        if totals is None:
            for col in columns:
                print ('| %s ' % col[row_idx]),
        else:
            for col, total in zip(columns, totals):
                perc = 100.0 * float(col[row_idx]) / total
                if perc >= 10:
                    fmt = '| %s (%d%%) '
                elif perc > 1:
                    fmt = '| %s (%.1f%%) '
                else:
                    fmt = '| %s (%.2f%%) '
                print (fmt % (col[row_idx], perc)),

        print '||'

    if totals is not None:
        print '|| total | %s ||' % ' | '.join(map(str, totals))

    print '|#'


#to_perc_wiki_table(**minutes)

to_perc_wiki_table(**phrases)
