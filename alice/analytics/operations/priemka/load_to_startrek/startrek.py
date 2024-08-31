from datetime import datetime
import pytz
import nirvana.job_context as nv
from operations.priemka.metrics_viewer.metrics_viewer import (
    get_nirvana_meta
)


def get_workflow_description(meta_data):
    ctx = nv.context()
    text = ctx.get_meta().get_workflow_url()

    if 'instanceComment' in meta_data.keys():
        text += ' !!(сер){comment}!!\n'.format(comment=meta_data.get('instanceComment'))
    else:
        text += '\n'
    return text


def get_time_duration_info(meta_data):
    start_time = datetime.strptime(meta_data.get('started', ''), "%Y-%m-%dT%H:%M:%S%z")
    cur_time = datetime.now(pytz.timezone('Europe/Moscow'))
    delta_time = cur_time - start_time
    hours = delta_time.days * 24 + delta_time.seconds // 3600
    minutes = (delta_time.seconds % 3600) // 60
    text = '{start} - {finish} (duration {hours}:{minutes})\n'.format(start=start_time.strftime("%Y-%m-%d %H:%M"),
                                                                      finish=cur_time.strftime("%Y-%m-%d %H:%M"),
                                                                      hours=hours, minutes=minutes)
    return text


def create_table(in2):
    text = '<{metrics table\n#|\n|| **basket** | **metrics_group** | **metric_name** | **diff** | **link** ||\n'
    pattern = '|| {basket} | {metrics_group} | {metric_name} | {diff} | {pulsar} ||\n'
    for item in in2:
        if 'pulsar_link_short' in item.keys():
            pulsar_link = '(({link} pulsar_link_short))'.format(link=item.get('pulsar_link_short'))
        elif 'link' in item.keys():
            pulsar_link = '(({link} link))'.format(link=item.get('link'))
        else:
            pulsar_link = ''

        if 'diff' in item.keys():
            diff = '{diff} ({percent}%)'.format(diff=round(float(item.get('diff')), 4),
                                                percent=round(float(item.get('diff_percent')), 2))
        else:
            diff_items = []
            if item.get('prod_quality') is not None:
                diff_items.append('prod: {:0.4f}'.format(float(item['prod_quality'])))
            if item.get('test_quality') is not None:
                diff_items.append('test: {:0.4f}'.format(float(item['test_quality'])))
            diff = ', '.join(diff_items)

        row = pattern.format(
            basket=item.get('basket', ''),
            metrics_group=item.get('metrics_group', ''),
            metric_name=item.get('metric_name', ''),
            diff=diff,
            pulsar=pulsar_link
        )
        text += row
    text += '|#}>'
    return text


def get_result(in1, in2):
    if len(in2) == 0:
        text = 'result: !!(зел)success!!\n'
    else:
        text = 'result: !!failure!! ({count1}/{count2} metrics)\n'.format(count1=len(in2), count2=len(in1))
        text += create_table(in2)

    return text


def get_startrek_message_text(in1, in2):
    meta_data, _ = get_nirvana_meta()

    text = '=== ue2e finished\n'

    text += get_workflow_description(meta_data)

    text += get_time_duration_info(meta_data)

    text += get_result(in1, in2)

    return text
