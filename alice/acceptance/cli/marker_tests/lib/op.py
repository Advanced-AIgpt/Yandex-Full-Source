# coding: utf-8

import vh

SINGLE_OPTION_TO_JSON = vh.op(
    id='2fdd4bb4-4303-11e7-89a6-0025909427cc',
)
JSON_TO_MR_TABLE_OP = vh.op(
    name='MR Write JSON to Directory (Create New)',
    id='5fe73009-86d1-4eaf-8c22-50c1f8b3097f',
)
MR_TABLE_TO_JSON_OP = vh.op(
    name='MR Read JSON',
    id='fcf99a2d-500b-407f-8075-f94d5e54a50e',
)
YQL_1 = vh.op(
    name='YQL 1',
    id='423ba830-b7e1-464e-9131-e9821a0f4c3c'
).partial(yt_table_outputs=['output1'])
YQL_2 = vh.op(
    name='YQL 2',
    id='c424ef02-de07-4e0e-9c72-334f22ee492d'
).partial(yt_table_outputs=['output1', 'output2'])
DEEP_DOWNLOADER_OP = vh.op(
    id='20956afe-14f4-4fd3-83e6-22bc81447c8f',
)
