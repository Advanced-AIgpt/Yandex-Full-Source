import yt.wrapper as yt


def extract_statistic(mr_table, yt_token):
    cluster = mr_table['cluster']
    table_path = mr_table['table']
    client = yt.YtClient(proxy=cluster, token=yt_token)
    table = client.read_table(table_path)
    for row in table:
        return row


def main(in1, in2, in3, mr_tables, token1=None, token2=None, param1=None, param2=None, html_file=None):
    return extract_statistic(mr_table=mr_tables[0], yt_token=token1)
