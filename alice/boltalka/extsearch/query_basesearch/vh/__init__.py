import itertools
from alice.boltalka.extsearch.query_basesearch.lib.grequests import QueryBasesearch
import vh
import yt.wrapper as yt


def make_queries(lines, args):
    for line in lines:
        context = []
        for i in itertools.count():
            k = f'context_{i}'
            if k not in line:
                break
            context.append(line[k])
        query = '\n'.join(reversed(context))
        if args.use_entity:
            query = line['movie_id'], query
        yield query

def query_basesearch(
    input_table: vh.mkinput(vh.YTTable),
    output_table: vh.mkoutput(vh.YTTable),
    host: vh.mkoption(str, default='general-conversation-hamster.yappy.beta.yandex.ru'),
    port: vh.mkoption(int, default=80),
    max_results: vh.mkoption(int, default=1),
    min_ratio: vh.mkoption(float, default=1.0),
    context_weight: vh.mkoption(float, default=1.0),
    ranker: vh.mkoption(str, default='catboost'),
    extra_params: vh.mkoption(str, default=''),
    experiments: vh.mkoption(str, default=''),
    output_attr: vh.mkoption(str, default='reply'),
    max_retries: vh.mkoption(int, default=0),
    pool_size: vh.mkoption(int, default=1),
    non_deterministic: vh.mkoption(bool, default=False),
    use_entity: vh.mkoption(bool, default=False)
):
    query_basesearcher = QueryBasesearch(host, port, experiments, max_results,
                                         min_ratio, context_weight, ranker,
                                         extra_params, pool_size, max_retries,
                                         not non_deterministic, output_attr)
    def generate():
        lines, lines2 = itertools.tee(yt.read_table(input_table), 2)
        for line, replies in zip(lines, query_basesearcher.process_iterator(make_queries(lines2, args), args.use_entity)):
            for el in replies:
                line.update(el)
                yield line
    yt.write_table(output_table, generate())

query_basesearch_lazy = vh.lazy.from_annotations(query_basesearch)
