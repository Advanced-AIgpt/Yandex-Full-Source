import os

import vh

from dict.mt.make.libs.common import MTFS


def tmp_table_to_json_option(option_name, table):
    return vh.op(name='MR Tables to JSON', id='fe4d2727-5ac8-4286-be1b-585aaa850f8a')(
        table1=table, key1=option_name
    )['json']


def build_pipelines(mtdata: MTFS, pipelines: list, yt_token: str, save_results: bool, yt_cluster: str = 'hahn'):
    mtfs_root = mtdata.base_mtdata.root

    all_destination_paths = dict()
    last_make_dirs_result = None

    for pipeline in pipelines:
        pipeline_update = pipeline(mtdata)

        for k, v in pipeline_update.items():
            if save_results:
                dst_path = os.path.join(mtfs_root, k)
                dir_path = os.path.dirname(dst_path)
                if dir_path not in all_destination_paths:
                    make_dirs_result = vh.op(name='MR Create Directory', id='33d81b2c-1d18-11e7-904c-3c970e24a776')(
                        src_path=dir_path,
                        src_cluster=yt_cluster,
                        yt_token=yt_token
                    )

                    if last_make_dirs_result is not None:
                        make_dirs_result.execute_after(last_make_dirs_result)
                    last_make_dirs_result = make_dirs_result
                    all_destination_paths[dir_path] = make_dirs_result
                make_dirs_result = all_destination_paths[dir_path]

                if isinstance(v, vh.YTTable):
                    vh.op(name='MR Copy Table', id='23762895-cf87-11e6-9372-6480993f8e34')(
                        src=v,
                        dst_path=dst_path,
                        yt_token=yt_token
                    ).execute_after(make_dirs_result)
                else:
                    vh.op(name='MR Upload File', id='fe5be04f-5e6b-494a-8673-961535b036a3')(
                        content=v,
                        dst_path=dst_path,
                        mr_default_cluster=yt_cluster,
                        write_mode='OVERWRITE',
                        yt_token=yt_token
                    ).execute_after(make_dirs_result)
        mtdata.update(pipeline_update)


# TODO remove this hack
def cast_to_mrtable(input):
    return vh.op(id='58860ad8-548f-46ca-afe4-fb5cca18669f', name='Cast to mrTable')(
        input=input,
        yt_table_outputs=['output']
    )['output']


# TODO remove this hack
def cast_any_to_any(input, out_type):
    return vh.op(id='0c972ebb-5bad-4db2-8e14-c902811df27d', name='Cast any to any')(
        input=input
    )[out_type]
