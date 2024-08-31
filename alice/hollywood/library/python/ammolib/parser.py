import collections


def parse_graphs_sources_list(graphs_sources_list):
    """
    :param graphs_sources_list: - is the `-s` parameter of request_sampler tool.
    """
    result = collections.defaultdict(set)
    for pair in graphs_sources_list.split(","):
        sub_result = _parse_graphs_sources_pair(pair)
        for graph, sources in sub_result.iteritems():
            for source in sources:
                result[graph].add(source)
    return result


def _parse_graphs_sources_pair(graphs_sources):
    split_res = graphs_sources.split(":")
    if len(split_res) == 2:
        graphs = split_res[0]
        sources = split_res[1]
    else:
        graphs = ""
        sources = split_res[0]
    result = collections.defaultdict(set)
    for graph in graphs.split("."):
        for source in sources.split("."):
            result[graph].add(source)
    return result


def make_output_filenames(filename_prefix, graphs_sources_dict):
    result = []
    for graph, sources in graphs_sources_dict.iteritems():
        for source in sources:
            if graph == "":
                filename = "_".join([filename_prefix, source])
            else:
                filename = "_".join([filename_prefix, graph, source])
            result.append(filename)
    return result
