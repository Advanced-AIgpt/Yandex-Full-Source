# ---
# jupyter:
#   jupytext:
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.5'
#       jupytext_version: 1.4.1
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
#   nb_template:
#     name_en: Collect experiment metrics
#     description_en: >-
#       Check experiment status and collect metrics for all instances
# ---

import pandas as pd
from pulsar import ParamsTuningExperimentView, PulsarClient, Metrics

# ## Pulsar Client
# Get your Pulsar OAuth token by [instruction](https://wiki.yandex-team.ru/pulsar/kak-poluchit-oauth-token-dlja-pulsara/).
#
# #### Pass this token to `token` parameter below or set environment variable `PULSAR_TOKEN` or put your token to the file `~/.pulsar/token`.

client = PulsarClient(token='')

# ## Experiment status

# +
experiment_id = '{{ experiment_id }}'

experiment = ParamsTuningExperimentView(experiment_id=experiment_id, pulsar_client=client)
print('Experiment status', experiment.get_status())

# +
# experiment.restart_failed_instances()
# -

# ## Best instance

experiment.get_best_instance()

experiment.get_best_parameters()

# ## Get instances

instances = experiment.get_results()

metrics_list = []
for instance in instances:
    instance_id = instance.instance_id
    d = {'instance_id': instance_id}

    instance_metrics = client.get(instance_id).metrics
    if instance_metrics:
        instance_metrics = instance_metrics.to_dict()
        for metric in instance_metrics:
            d[metric['name']] = metric['value']

    metrics_list.append(d)

metrics = pd.DataFrame(metrics_list)

metrics
