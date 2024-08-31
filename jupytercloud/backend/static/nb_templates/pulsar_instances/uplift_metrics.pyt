# ---
# jupyter:
#   jupytext:
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.5'
#       jupytext_version: 1.14.0
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
#   nb_template:
#     name_en: Get uplift metrics
#     description_en: >-
#       Get uplift metrics for the best model
# ---

import warnings
warnings.filterwarnings('ignore')
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from causalml.metrics import plot, auuc_score
from scipy.stats import ttest_ind
from nile.api.v1 import clusters
cluster = clusters.YT('hahn')

# ### Start here
#
# Для начала убедись, что у тебя готовы статистики по пользователям из валидационного сета. Они должны быть посчитаны по дням начиная с запуска экспа.
#
# Кроме того, сделай предсказания по всем пользователям и укажи путь до таблички ниже. Чтобы сделать предсказания достаточно нажать на кнопку Predict в инстансе модели и указать нужные параметры.
#
# Наконец, укажи метрику, которая будет таргетом.

target_feature = ''
user_stats_path = ''
preds_path = ''

best_instance_id = '{{ instance_id }}'

# ### Load data

# +
from yql.api.v1.client import YqlClient

# OAuth token can be passed via YqlClient() argument or YQL_TOKEN env variable
# Open https://yql.yandex-team.ru/oauth in browser to get
# your token

client = YqlClient(token='')
# -

query = f"""
PRAGMA yt.InferSchema = '1000';
use hahn;
SELECT
    u.*,
    preds.`{best_instance_id}` as uplift_score
FROM `{user_stats_path}` as u
INNER JOIN `{preds_path}` as preds
on u.phone_pd_id = preds.phone_pd_id
"""

print(query.format(best_instance_id, user_stats_path, preds_path))

# + [markdown] tags=[]
# #### Получение данных
#
# Эта штука ниже может занять какое-то время, прошу подождать

# +
request = client.query(
    query.format(best_instance_id, user_stats_path, preds_path),
    syntax_version=1,
)

request.run()
df_by_day = request.table.full_dataframe
# -

df_by_day['dt'] = pd.to_datetime(df_by_day.dt)
df_by_day['treatment_group'] = df_by_day.is_treatment.replace(
    {1: 'test', 0: 'control'},
)

df_by_week_all = (
    df_by_day.groupby(
        [
            pd.Grouper(key='dt', freq='W'),
            'phone_pd_id',
            'treatment_group',
            'uplift_score',
        ],
    )[[target_feature]]
    .sum()
    .unstack(level=0, fill_value=0)
    .stack()
    .reset_index()
)

df_by_week_groupped = (
    df_by_week_all.groupby(['dt', 'treatment_group'])[target_feature]
    .mean()
    .unstack()
    .reset_index()
)
df_by_week_groupped['diff'] = (
    df_by_week_groupped['test'] - df_by_week_groupped['control']
)
df_by_week_groupped = df_by_week_groupped.rename_axis(None, axis=1)
df_by_week_groupped['diff_cumsum'] = df_by_week_groupped['diff'].cumsum()

# +
df_by_week_groupped.plot(x='dt', y='diff')
plt.title(f'Diff by week: {target_feature}')
plt.show()

df_by_week_groupped.plot(x='dt', y='diff_cumsum')
plt.title(f'Cumsum diff by week: {target_feature}')
plt.show()
# -

# ### General Uplift metrics

df = (
    df_by_day.groupby(['phone_pd_id', 'is_treatment', 'uplift_score'])[
        target_feature
    ]
    .sum()
    .reset_index()
)

auuc_score(
    df.drop('phone_pd_id', axis=1),
    treatment_col='is_treatment',
    outcome_col=target_feature,
)

plot(
    df.drop('phone_pd_id', axis=1),
    treatment_col='is_treatment',
    outcome_col=target_feature,
)

# ### Get segments size

df['bucket'] = np.random.randint(0, 100, size=(len(df),))

df = df.sort_values(by='uplift_score', ascending=False)


def run_bucket_test(data, top, target_feature):
    limit = int(top * len(data))
    data = data.iloc[:limit].copy()
    treat_metrics = (
        data[data.is_treatment == 1]
        .groupby('bucket')[target_feature]
        .mean()
        .values
    )
    control_metrics = (
        data[data.is_treatment == 0]
        .groupby('bucket')[target_feature]
        .mean()
        .values
    )
    if treat_metrics.mean() > control_metrics.mean():
        _, p_value = ttest_ind(treat_metrics, control_metrics, equal_var=False)
    else:
        p_value = 1
    return p_value


# +
step = 0.1
threshold = 0.05

p_values = []
segment_sizes = np.arange(step, 1, step)
max_size = 0
for top in segment_sizes:
    p_value = run_bucket_test(df, top, target_feature)
    p_values.append(p_value)
    if p_value <= threshold:
        max_size = top
print('Max segment size: ', max_size)
# -

plt.plot(segment_sizes, p_values)
plt.title('P-value for each segment size')
plt.ylim(-0.05, 1.05)
plt.show()

# ### Select segment size

segment_size = 0.2

score_limit = df_by_week_all.uplift_score.quantile(1 - segment_size)

df_by_week_all['good_users'] = df_by_week_all.uplift_score > score_limit

# ### Conf intervals

# +
from itertools import repeat
from multiprocessing import Pool


def get_mean_bootstrap_samples(data):
    indx = np.random.randint(0, len(data), (len(data)))
    return np.mean(data[indx], axis=0)


def stat_interval(stat, alpha=0.05):
    return np.percentile(stat, [100 * alpha / 2.0, 100 * (1 - alpha / 2.0)])


def run_mp(params):
    np.random.seed()
    test_metric, control_metric = params
    diff = get_mean_bootstrap_samples(
        test_metric,
    ) - get_mean_bootstrap_samples(control_metric)
    return diff


# -


def get_conf_intervals(df):
    results = []
    for dt in df.dt.unique():
        tmp = (
            df[df.dt <= dt]
            .groupby(['phone_pd_id', 'treatment_group'])[target_feature]
            .sum()
            .reset_index()
        )

        test = tmp[tmp.treatment_group == 'test'][target_feature].values
        control = tmp[tmp.treatment_group == 'control'][target_feature].values

        diff = np.mean(test) - np.mean(control)

        pool = Pool(processes=6)
        res = pool.map(run_mp, repeat((test, control), 100))
        pool.close()

        lower, upper = stat_interval(res)

        results.append([dt, diff, lower, upper])

    return pd.DataFrame(results, columns=['dt', 'diff', 'lower', 'upper'])


good_users = get_conf_intervals(df_by_week_all[df_by_week_all.good_users])
bad_users = get_conf_intervals(df_by_week_all[~df_by_week_all.good_users])

# +
plt.figure(figsize=(10, 8))

plt.plot(good_users.dt, good_users['diff'], 'ob-', label='ml_good_people')
plt.plot(good_users.dt, good_users[['lower', 'upper']], 'b--', alpha=0.3)

plt.plot(bad_users.dt, bad_users['diff'], 'or-', label='ml_bad_people')
plt.plot(bad_users.dt, bad_users[['lower', 'upper']], 'y--', alpha=0.3)

plt.axhline(y=0, color='r', alpha=0.2, linestyle='--')
plt.title(
    f'Cumsum {target_feature}: diff with conf interval (bootstrap, alpha=0.05)',
)
plt.xticks(rotation=70, ticks=good_users.dt)
plt.legend()
plt.show()
