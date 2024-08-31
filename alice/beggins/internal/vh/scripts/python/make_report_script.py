from bs4 import BeautifulSoup
from jinja2 import Template


def prepare_content(data):
    graph_link = data['graph_link']
    meta = {
        'model_name': data['model_name'],
        'embedder':  data['embedder'],
        'threshold': data['threshold']['threshold'],
        'graph': f"<a href='{graph_link}'>{graph_link}</a>",
        'catboost_params': str(data['catboost_params']),
        'st_ticket': data['st_ticket'],
        'week': data['week']
    }

    metrics = {
        'train': data['train_metrics']['metrics'],
        'val': data['val_metrics']['metrics'],
        'accept': data['accept_metrics']['metrics']
    }
    metric_names = list(data['train_metrics']['metrics'].keys())

    logs = {
        'week_logs_stats': sorted(data['week_logs_stats'], key=lambda x: x['cnt'], reverse=True),
        'week_matches': sum([intent_stat['cnt'] for intent_stat in data['week_logs_stats']]),
        'year_logs_stats': sorted(data['year_logs_stats'], key=lambda x: x['cnt'], reverse=True),
        'year_matches': sum([intent_stat['cnt'] for intent_stat in data['year_logs_stats']]),
    }

    threshold_plot = BeautifulSoup(data['threshold_plot'], 'html.parser').div
    train_matrix = BeautifulSoup(data['train_matrix'], 'html.parser').div
    val_matrix = BeautifulSoup(data['val_matrix'], 'html.parser').div
    accept_matrix = BeautifulSoup(data['accept_matrix'], 'html.parser').div

    plots = {
        'threshold':  threshold_plot,
        'train': train_matrix,
        'val': val_matrix,
        'accept': accept_matrix
    }

    content = {
        'meta': meta,
        'metric_names': metric_names,
        'metrics': metrics,
        'plots': plots,
        'logs': logs
    }
    return content


def main(in1, in2, in3, mr_tables, token1=None, token2=None, param1=None, param2=None, html_file=None):
    content = prepare_content(in1[0])
    template = in2[0]['template']
    html_report = Template(template, trim_blocks=True).render(content=content)
    html_file.write(html_report)
