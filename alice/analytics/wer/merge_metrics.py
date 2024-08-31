import json
import io
import sys
import argparse
from collections import defaultdict


def main(argv):
    """
    Мерджит метрики из нашего кубика и кубика облака.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('--input_table')
    parser.add_argument('--metrics_report')
    parser.add_argument('--output_table')
    parser.add_argument('--id_input_column')
    parser.add_argument('--summary_metrics')
    args = parser.parse_args(argv[1:])
    with io.open(args.input_table, 'r') as f:
        input_table = json.load(f)
    with io.open(args.metrics_report, 'r') as f:
        metrics_report = json.load(f)
    id_input_column = args.id_input_column
    metrics_summary = defaultdict(int)
    for row in input_table:
        cur_id = row[id_input_column]
        for metric in metrics_report['records'][cur_id]:
            metric_name = metric['metric']
            metric_variant = metric['text_transformation_case']
            metric_value = metric['value']
            if metric_name == "MER-1.0":
                if metric_variant == 'norm':
                    metrics_summary["MER-1.0(norm)"] += metric_value
                else:
                    metrics_summary["MER-1.0"] += metric_value
            if metric_variant != 'none':
                row['metrics']['{}({})'.format(metric_name, metric_variant)] = metric_value
            else:
                row['metrics']['{}'.format(metric_name)] = metric_value
    for key in metrics_summary.keys():
        metrics_summary[key] /= len(input_table)
    with io.open(args.output_table, 'w', encoding='utf-8') as f:
        f.write(json.dumps(input_table, ensure_ascii=False))
    with io.open(args.summary_metrics, 'w', encoding='utf-8') as f:
        f.write(json.dumps(metrics_summary, ensure_ascii=False))


if __name__ == '__main__':
    main(sys.argv)
