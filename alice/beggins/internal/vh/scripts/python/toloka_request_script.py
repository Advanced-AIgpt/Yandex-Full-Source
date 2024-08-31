def toloka_request(v, w, x):
    import yt.wrapper as yt
    client = yt.YtClient(v['cluster'], config=yt.config.config)
    table = client.read_table(v['table'])
    current_search_strings = [row['text'] for row in table]
    return {
        'current_search_strings': current_search_strings,
        'positive_strings': [line.strip() for line in w],
        'negative_strings': [line.strip() for line in x],
    }
