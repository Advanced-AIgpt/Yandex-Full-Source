from yt import wrapper as yt
from datetime import datetime
from re import compile
from nirvana import job_context as nv
from json import dump, loads

def get_creation_time(file):
    '''
    Typical output of `yt.get(file + "/@creation_time")` is `"2021-04-14T15:16:01.932113Z"`.
    '''
    creation_time = yt.get(file + "/@creation_time")
    creation_date = datetime.strptime(creation_time, "%Y-%m-%dT%H:%M:%S.%fZ").date()
    return creation_date

def get_file_type(file):
    file_type = yt.get(file + "/@type")
    return file_type

def parse_nirvana_url(file):
    '''
    Typical output of `yt.get(file + "/@_yql_op_title")` is `"YQL Nirvana Operation: https://nirvana.yandex-team.ru/process/0e5d5ddf/graph/operation/4fffbe9d"`.
    function returns `"https://nirvana.yandex-team.ru/process/0e5d5ddf/"`.
    '''
    parts = yt.get(file + "/@_yql_op_title").split("/")
    url = f"https://nirvana.yandex-team.ru/process/{parts[4]}/"
    return url

def find_mr_tables(path="//home/voice/asr/zbp/weekly_results", cluster="hahn"):
    '''
    First we find the last two (wrt creation time) MR tables in `path` on `cluster`.
    From these tables we extract:
        1. links to nirvana graphs (which produced these tables), creation times -> output 1
        2. path to the second newest table, cluster -> output 2
    '''
    yt.config.set_proxy(cluster)
    
    file_names = yt.list(path)
    valid_pattern = compile("[2-9][0-9][0-9][0-9]-[0-1][0-9]-[0-3][0-9]")
    valid_file_names = list(filter(lambda name: (get_file_type(path + "/" + name) == "table") & bool(valid_pattern.fullmatch(name)), file_names))
    creation_times = map(lambda name: get_creation_time(path + "/" + name), valid_file_names)
    reordered = sorted(zip(valid_file_names, creation_times), key=lambda T: T[1])
    
    target_file_second_last = path + "/" + reordered[-2][0]
    target_file_last = path + "/" + reordered[-1][0]
    
    nirvana_url_last = parse_nirvana_url(target_file_last)
    nirvana_url_second_last = parse_nirvana_url(target_file_second_last)
    
    date_created_last = get_creation_time(target_file_last)
    date_created_second_last = get_creation_time(target_file_second_last)
    
    params = {"param": [f'old_nirvana_url="{nirvana_url_second_last}"', f'old_date="{date_created_second_last}"', f'new_nirvana_url="{nirvana_url_last}"', f'new_date="{date_created_last}"']}
    table = {"table": target_file_second_last, "cluster": cluster}
    return params, table

if __name__ == "__main__":
    ctx = nv.context()
    kwargs = loads(ctx.get_parameters().get('kwargs', '{}'))
    inputs = ctx.get_inputs()
    outputs = ctx.get_outputs()

    dates_and_links, table_name = find_mr_tables(path=kwargs.get("path", 1), cluster=kwargs.get("cluster", 1))
    dates_and_links["param"].append(f'python_path="{kwargs.get("python_path", 1)}"')
    
    with open(outputs.get('retvalue'), 'w') as f:
        dump(dates_and_links, f, indent=4, ensure_ascii=False)

    with open(outputs.get('output2'), 'w') as f:
        dump(table_name, f, indent=4, ensure_ascii=False)
