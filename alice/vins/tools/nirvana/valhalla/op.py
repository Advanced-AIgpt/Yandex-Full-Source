# coding: utf-8

import vh


BUILD_ARC_OP = vh.op(
    name='Build Arcadia Project',
    owner='finiriarh',
    version='1.20',
)
YA_PACKAGE_OP = vh.op(
    name='YA_PACKAGE',
    owner='finiriarh',
)
GET_MR_TABLE_OP = vh.op(
    name='Get MR Table',
    owner='nirvana',
)
DUMP_DATASET_OP = vh.op(
    name='VINS dump dataset',
    id='08f8c25f-ab3b-4216-aedd-e8226f4f7602',
)
NORMALIZE_OP = vh.op(
    name='[VINS] Normalize requests',
    id='ed8d598c-1b0e-43c7-aa6f-a0151413a704',
)
DOWNLOADER_OP = vh.op(
    name='VINS Downloader',
    id='422e3cb4-e385-499c-8770-79c9f08ffe85',
)
YQL_2_OP = vh.op(
    name='YQL 2',
    id='d5e044a9-a9a8-4958-a38f-b9b24f15b51e',
).partial(yt_table_outputs=['output1'])
