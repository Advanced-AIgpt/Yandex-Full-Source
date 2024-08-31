import vh
import json
import numbers
from collections import OrderedDict


pulsar_delete_instance_op = vh.op(id="bd726318-dade-4929-8c83-b22ab9b72109")
pulsar_get_instance_op = vh.op(id="c837d5ef-59fc-4cbf-8b98-41742669113e")
pulsar_add_instance_op = vh.op(id="3b3f40b1-0138-4195-b486-de70624ed2ed")
pulsar_update_instance_op = vh.op(id="def088d0-ad59-4557-8e83-4ef48453acf1")


def add_pulsar_instance(name, model_name, model_version, model_options,
                        dataset_name, dataset_info, tags, user_datetime):
    return pulsar_add_instance_op(
        _inputs={
            'metrics': vh.data_from_str('{}')
        },
        _options={
            'model_name': model_name,
            'model_version': model_version,
            'model_options': json.dumps(model_options),
            'dataset_name': dataset_name,
            'dataset_info': json.dumps(dataset_info),
            'tags': tags,
            'user_datetime': user_datetime,
            'name': name.replace(' ', '_'),
            'timestamp': '1574238058',
        },
    )['info']


def get_pulsar_instance(instance_info):
    return pulsar_get_instance_op(
        _dynamic_options=[
            instance_info
        ],
        _options={
            'instance_id': 'set_later'
        }
    )['info']


def update_pulsar_instance(instance_info, metrics, data):
    return pulsar_update_instance_op(
        _inputs={
            'metrics': metrics,
            'data': data
        },
        _dynamic_options=[
            instance_info
        ],
        _options={
            'instance_id': 'set_later'
        }
    )['info']


def add_update_pulsar_instance(name, model_name, model_version, model_options,
                               dataset_name, dataset_info, tags, user_datetime,
                               metrics, data):
    return update_pulsar_instance(
        add_pulsar_instance(name, model_name, model_version, model_options,
                            dataset_name, dataset_info, tags, user_datetime),
        metrics,
        data)


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_file=vh.mkinput(vh.File), output_file=vh.mkoutput(vh.File))
def get_report_metrics(input_file, output_file):
    with open(input_file) as f:
        report = json.load(f, object_pairs_hook=OrderedDict)
    report = OrderedDict((k, v) for k, v in report.items() if isinstance(v, numbers.Number))
    with open(output_file, 'w') as f:
        json.dump(report, f)
    return output_file
