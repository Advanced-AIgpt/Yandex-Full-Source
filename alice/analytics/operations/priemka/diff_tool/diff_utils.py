import re
import json
import numpy as np
import pandas as pd
import yt.wrapper as yt
import jupytercloud.nirvana as jn
from datetime import datetime, timedelta
from IPython.display import display, HTML
from operator import __gt__, __lt__, __ne__
from scipy.stats import ttest_rel


class DiffTool(object):
    """
    Укрупненный анализ факторов расхождений метрики качества в ue2e
    обрабатывает 1 корзину.
    """

    def __init__(self, table_a, table_b, yt_token=None, basket=None, verbose=False, hide_examples=False):
        """
        table_a, table_b -- пути MR таблиц в hahn или pd.DataFrame с результатами
        basket -- избранная корзина (для таблиц приемки с несколькими корзинами) или автоопределение
        hide_examples -- подавляет вывод примеров из индивидуальных запросов
        """
        self.log = []
        self.yt_token = yt_token
        self.verbose = verbose
        self.basket = basket
        self.df_a = self._load_instance(table_a, verbose=self.verbose)
        self.df_b = self._load_instance(table_b, verbose=self.verbose)
        self._build_ab(verbose=self.verbose)
        self.hide_examples = hide_examples

        self.scenarios_count = self.df_ab.generic_scenario_a.value_counts()
        self._build_key_stats()
        self._build_asr_stats()
        self._build_chg_scen_stats()
        self._build_scen_same_stats()
        self._build_nan_stats()

    @staticmethod
    def read_yt_table(table_path, token=None):
        """reads table from hahn, returns DF"""
        yt.config["proxy"]["url"] = 'hahn.yt.yandex.net'
        client = yt.YtClient(proxy='hahn', token=token)
        read_result = client.read_table(table_path, raw=False)
        raw_data = [row for row in read_result.rows]
        return pd.DataFrame(raw_data)

    def _load_instance(self, table, verbose=False):
        """loads table, drops dupes with smaller metric, sorts by req_id"""
        if type(table) != pd.DataFrame:  # allows to pass pre-loaded tables
            df = self.read_yt_table(table, self.yt_token)
            self.log.append(('LOAD', f"<b>Loaded table from {table}</b>"))
        else:
            df = table
            self.log.append(('LOAD', f"<b>Loaded table as pd.DataFrame</b>"))

        if self.basket is None:
            self.basket = df.basket.mode()[0]  # using most frequent basket value as default
            print(f"basket '{self.basket}' auto-detected")
            self.log.append(('LOAD', f"basket '{self.basket}' auto-detected"))

        self.log.append(('LOAD',
                         f"<pre>  raw shape: {df.shape}; metric_integral: {df.metric_integral.mean():.5f}</pre>"))

        df = df.loc[df.basket == self.basket]

        df = df \
            .sort_values(by='metric_integral', ascending=False) \
            .drop_duplicates(subset=['req_id'], keep='first') \
            .sort_values(by='req_id') \
            .set_index('req_id')
        self.log.append(
            ('LOAD', f"<pre>  selected basket: {self.basket}; processed shape: {df.shape}, metric_integral: {df.metric_integral.mean():.5f}</pre>"))
        return df

    def _build_ab(self, verbose=False):
        assert set(self.df_a.index).difference(set(self.df_b.index)) == set(), "Наборы запросов в таблицах не совпадают!"  # match idx on req_ids
        self.instance_size = self.df_a.shape[0]

        # list of columns to include / drop into AB Dataframe
        common_cols = ['app', 'basket', 'session_id', 'text', 'voice_url']
        drop_cols = ['metric_geo', 'metric_video', 'fraud_eosp', 'metric_search', 'metric_commands', 'screenshot_url',
                     'mm_scenario', 'generic_scenario_human_readable', 'metric_general_conversation_fraud', 'hashsum',
                     'metric_toloka_gc', 'metric_alarms_timers', 'metric_translate', 'toloka_intent', 'state0',
                     'metric_radio',
                     'setrace_url', 'metric_no_commands', 'metric_music'] + \
                    ['voice_url', 'session', 'result', 'general_toloka_intent', 'session_id', 'basket', 'is_command'] + \
                    ['has_eosp_tag', 'metric_integral_iot', 'metric_news', 'metric_timetables', 'metric_weather',
                     'screenshot_url']

        drop_cols_a = [c for c in drop_cols if c in self.df_a.columns]
        drop_cols_b = [c for c in [*drop_cols, *common_cols] if c in self.df_b.columns]

        self.df_ab = pd.merge(self.df_a.drop(drop_cols_a, axis=1),
                              self.df_b.drop(drop_cols_b, axis=1),
                              on='req_id', suffixes=('_a', '_b'), )

        self.df_ab['diff_'] = self.df_ab.metric_integral_b - self.df_ab.metric_integral_a

        self.df_ab.loc[self.df_ab.metric_integral_a.isna() | self.df_ab.metric_integral_b.isna(), 'type'] = 'nan'
        self.df_ab.loc[self.df_ab.type.isna() & (self.df_ab.asr_text_a != self.df_ab.asr_text_b), 'type'] = 'asr'
        self.df_ab.loc[self.df_ab.type.isna() & (self.df_ab.generic_scenario_a != self.df_ab.generic_scenario_b), 'type'] = 'chg_scen'
        self.df_ab.loc[self.df_ab.type.isna() & (self.df_ab.diff_ != 0), 'type'] = 'scen_same'
        self.df_ab.type = self.df_ab.type.fillna('no_changes')  # catch all into no_changes category

        self.log.append(('LOAD', f"<b>AB dataframe created with shape {self.df_ab.shape};  "
                                 f"Total diff = {self.df_b.metric_integral.mean() - self.df_a.metric_integral.mean():.5f}</b>"))

    def _build_key_stats(self):
        """
        calculates table with summary diff in key diff sources
        returns None, result in self.df_key (DF)
         """
        key_stats = dict()
        key_stats['ASR_changes'] = self.calc_stats(self.df_ab[self.df_ab.type.str.startswith('asr')], add_pvalue=True)
        key_stats['changed_scenario'] = self.calc_stats(self.df_ab[self.df_ab.type == 'chg_scen'], add_pvalue=True)
        key_stats['same_scenario'] = self.calc_stats(self.df_ab[self.df_ab.type.str.startswith('scen')], add_pvalue=True)
        key_stats['nan_in_metrics'] = self.calc_stats(self.df_ab[self.df_ab.type == 'nan'])
        key_stats['no_changes'] = self.calc_stats(self.df_ab[self.df_ab.type == 'no_changes'])
        key_stats['TOTAL'] = self.calc_stats(self.df_ab, add_pvalue=True)
        self.df_key = pd.DataFrame(key_stats).T

    def _build_asr_stats(self):
        asr_index = self.df_ab[self.df_ab.type.str.startswith('asr')].index
        if asr_index.shape[0] == 0:
            self.df_asr = pd.DataFrame()
            return

        for req_id, row in self.df_ab.loc[asr_index, :].iterrows():
            if row['asr_text_a'] in row['asr_text_b']:
                self.df_ab.loc[req_id, 'type'] = 'asr_over'
            elif row['asr_text_b'] in row['asr_text_a']:
                self.df_ab.loc[req_id, 'type'] = 'asr_under'

        asr_stats = dict()
        asr_stats['under_listen'] = self.calc_stats(self.df_ab[self.df_ab.type == 'asr_under'])
        asr_stats['over_listen'] = self.calc_stats(self.df_ab[self.df_ab.type == 'asr_over'])
        asr_stats['asr_other'] = self.calc_stats(self.df_ab[self.df_ab.type == 'asr'])
        asr_stats['ASR_TOTAL'] = self.calc_stats(self.df_ab[self.df_ab.type.str.startswith('asr')])
        self.df_asr = pd.DataFrame(asr_stats).T

    def _build_chg_scen_stats(self):
        """
        calculates table with summary diff in cases of scenario changes
        returns None, result in DFs: self.df_scen_changed, self.df_scen_changed_dir
        """
        df_scen_diff = self.df_ab[self.df_ab.type == 'chg_scen'].copy()
        if df_scen_diff.shape[0] == 0:  # if no changes of this kind
            self.df_scen_changed = pd.DataFrame()
            self.df_scen_changed_dir = pd.DataFrame()
            return

        df_scen_a = df_scen_diff.groupby(['generic_scenario_a']).agg(
            {'text': 'count', 'diff_': 'mean'}).rename(columns={'text': 'num'})
        df_scen_b = df_scen_diff.groupby(['generic_scenario_b']).agg(
            {'text': 'count', 'diff_': 'mean'}).rename(columns={'text': 'num'})
        df_scen_changed = df_scen_a.join(df_scen_b, lsuffix='_lose', rsuffix='_gain', how='outer').fillna(0)
        df_scen_changed['contrib'] = (df_scen_changed.num_lose * df_scen_changed.diff__lose +
                                      df_scen_changed.num_gain * df_scen_changed.diff__gain
                                      ) / self.instance_size / 2
        df_scen_changed[['num_lose', 'num_gain']] = df_scen_changed[['num_lose', 'num_gain']].astype(int)
        df_scen_changed['num'] = df_scen_changed[['num_lose', 'num_gain']].sum(axis=1)
        df_scen_changed = df_scen_changed.sort_values(by='num', ascending=False)
        df_scen_changed['scenario_total'] = self.scenarios_count
        df_scen_changed['pct_of_scenario'] = (df_scen_changed.num_lose + df_scen_changed.num_gain) / self.scenarios_count
        self.df_scen_changed = df_scen_changed.drop(columns=['num'])
        self.df_scen_changed = self.df_scen_changed[~self.df_scen_changed.contrib.isna()]

        # TOP change directions
        self.df_scen_changed_dir = df_scen_diff[df_scen_diff.diff_ != 0] \
            .groupby(['generic_scenario_a', 'generic_scenario_b']) \
            .agg({'text': 'count', 'diff_': 'mean'}) \
            .reset_index().rename(columns={'text': 'num'}) \
            .sort_values('num', ascending=False).reset_index(drop=True)
        self.df_scen_changed_dir['contrib'] = (self.df_scen_changed_dir.num * self.df_scen_changed_dir.diff_) / self.instance_size

    def _build_scen_same_stats(self):
        """
        calculates table with summary diff in cases of scenario changes
        returns None, result in self.df_same_scenario
        """
        df_same = self.df_ab[self.df_ab.type.str.startswith('scen')].copy()

        if df_same.shape[0] == 0:  # if no changes of this kind
            self.df_same_scenario = pd.DataFrame()
            self.df_same = pd.DataFrame()
            return

        top_same_scenarios = df_same[df_same.diff_ != 0].generic_scenario_a.value_counts().index
        
        if top_same_scenarios.shape[0] == 0:
            self.df_same_scenario = pd.DataFrame()
            self.df_same = pd.DataFrame()
            return

        scen_stats = {}
        for scenario in top_same_scenarios:
            scen_stats[scenario] = self.calc_stats(df_same[df_same.generic_scenario_a == scenario])
        dfs = pd.DataFrame(scen_stats).T
        dfs['scenario_total'] = self.scenarios_count
        dfs.loc[dfs['scenario_total'] > 0, 'pct_of_scenario'] = (dfs.n_pos + dfs.n_neg) / dfs['scenario_total']
        self.df_same_scenario = dfs
        self.df_same = df_same

    def _build_nan_stats(self):
        nan_stats = dict()
        nan_stats['a_ok_b_nan'] = self.calc_stats(
            self.df_ab[~self.df_ab.metric_integral_a.isna() & self.df_ab.metric_integral_b.isna()])
        nan_stats['a_nan_b_ok'] = self.calc_stats(
            self.df_ab[self.df_ab.metric_integral_a.isna() & ~self.df_ab.metric_integral_b.isna()])
        nan_stats['both_nan'] = self.calc_stats(
            self.df_ab[self.df_ab.metric_integral_a.isna() & self.df_ab.metric_integral_b.isna()])
        nan_stats['both_nan']['contrib'] = 0  # patch to avoid rounding noise
        nan_stats['nan_metric'] = self.calc_stats(
            self.df_ab[self.df_ab.metric_integral_a.isna() | self.df_ab.metric_integral_b.isna()])
        self.df_nan = pd.DataFrame(nan_stats).T
        self.df_nan.loc[:, 'contrib'] = self.df_nan.loc[:, 'contrib'].fillna(0)

    def calc_stats(self, df, add_pvalue=False):
        """
        :param df: dataframe slice with some of the instance diff rows from self.df_ab
        :return: dict with key params of the diff slice
        """
        if df.shape[0] == 0:
            return {'size': 0, 'n_pos': 0, 'n_neg': 0, 'contrib': 0}
        stats = {
            'size': df.shape[0],
            'n_pos': df[df.diff_ > 0].shape[0],
            'n_neg': df[df.diff_ < 0].shape[0],
            'avg_pos': df.loc[df.diff_ > 0, 'diff_'].mean(),
            'avg_neg': df.loc[df.diff_ < 0, 'diff_'].mean(),
            'share_pos': round(df[df.diff_ > 0].shape[0] / df.shape[0], 3),
            'share_neg': round(df[df.diff_ < 0].shape[0] / df.shape[0], 3),
        }
        if add_pvalue and any((df.diff_ != 0) & ~df.diff_.isna()):
            stats['pvalue'] = round(ttest_rel(df.metric_integral_a.values, df.metric_integral_b.values, nan_policy='omit').pvalue, 5)
        
        if df.diff_.isna().sum() == 0:
            stats['contrib'] = round((df.metric_integral_b.mean() - df.metric_integral_a.mean()) *
                                     df.shape[0] / self.instance_size, 5)
        else:
            diff0 = self.df_ab.metric_integral_b.mean() - self.df_ab.metric_integral_a.mean()
            df1 = self.df_ab[['metric_integral_a', 'metric_integral_b']].copy()
            df1.loc[df.index, :] = 0
            diff1 = df1.metric_integral_b.mean() - df1.metric_integral_a.mean()
            stats['contrib'] = diff0 - diff1
        return stats

    def build_json_report(self, fielddate, device):
        """ produces report in json format for upload to StatFace """

        report = []

        def add_metric_record(metric, value):
            """ creates dict for single-metric record upload"""
            report.append({'fielddate': fielddate,
                           'device': device,
                           'basket': self.basket,
                           'metric': metric,
                           'value': round(value, 5), })

        for row in self.df_key.itertuples():
            if row.Index.lower() != 'no_changes':
                add_metric_record(row.Index.lower(), row.contrib)

        major_scenarios = ['music', 'ontofacts', 'video', 'general_conversation',
                           'serp', 'find_poi', 'route']
        major_scenarios_contrib = 0

        for scen in major_scenarios:
            if scen in self.df_same_scenario.index:
                scenario_contrib = self.df_same_scenario.loc[scen, 'contrib']
                major_scenarios_contrib += scenario_contrib
                add_metric_record('scenario_' + scen, scenario_contrib)

        if self.df_same_scenario.shape[0] > 0:
            scenario_other_contrib = self.df_same_scenario.contrib.sum() - major_scenarios_contrib
        else:
            scenario_other_contrib = 0
        add_metric_record('scenario_other', scenario_other_contrib)

        return json.dumps(report)

    def send_json_report(self, fielddate=None, device=None):
        """ sends json report when in Nirvana cube """
        if jn.is_nirvana() and device is not None and fielddate is not None:
            report = self.build_json_report(fielddate, device)
            output0 = jn.get_custom_output(0, local_path='diff_json_report.json')
            output0.write_text(report)
            if self.verbose:
                print('JSON РЕЗУЛЬТАТ:\n', report)
            print(f"отправлен json отчёт с {len(json.loads(report))} записей")

    @staticmethod
    def display_df(df_):
        """ nicely renders datarame """
        if df_ is None or df_.shape[0] == 0:
            display(HTML('<b> В этом разделе нет изменений, влияющих на метрику качества. </b>'))
            return

        df = df_.copy()
        pct_cols = [c for c in df.columns if c[:4] in ['pct_', 'freq', 'rati', 'shar']]
        int_cols = [c for c in df.columns if c[:4] in ['size', 'n_po', 'n_ne', 'num_']]
        metric_cols = [c for c in df.columns if c[:4] in ['metr', 'diff', 'avg_']]

        def highlight_significant(s, limit, props=''):
            return np.where(s < limit, props, '')

        df[pct_cols] = df[pct_cols] * 100
        contrib_lim = 0.003
        display(df.style \
                .set_table_styles([
                    {'selector': 'tr', 'props': [('font-size', '12pt'), ('font-family', 'Trebuchet MS',)]},
                    {'selector': 'th', 'props': [('font-size', '12pt'), ('font-family', 'Helvetica',)]},
                    ]) \
                .format({c: '{:.1f}%' for c in pct_cols}) \
                .format({c: '{:.2f}' for c in metric_cols}) \
                .format({c: '{:.0f}' for c in int_cols}) \
                .background_gradient(subset=['contrib'], cmap='PiYG', vmin=-contrib_lim, vmax=contrib_lim) 
#                 .apply(highlight_significant, props='background-color:#ffa', limit = 0.03, 
#                        subset=[c for c in df.columns if c == 'pvalue'])
               )
#         commented out call to highlight_significant() - not compatible with pandas==1.4
#         originally based on examples from https://pandas.pydata.org/docs/user_guide/style.html
#         need to re-test highlight_significant() with pandas==1.4.x

    @staticmethod
    def display_diffs(df):
        show_cols = ['metric_integral_a', 'metric_integral_b', 'text', 'answer_a', 'answer_b', 'action_a', 'action_b', 'diff_']
        metric_cols = [c for c in df.columns if c[:4] in ['metr', 'diff', 'avg_']]
        display(
            df.loc[:, show_cols].sort_values(by='diff_').style \
              .set_properties(**{'text-align': 'left'}) \
              .format({c: '{:.2f}' for c in metric_cols}))

    def display_key(self):
        self.display_df(self.df_key)

    def display_asr_summary(self):
        self.display_df(self.df_asr)

    def display_asr_examples(self, n_rows=10, asr_type='asr'):
        if self.hide_examples:
            print("Показ примеров отключен.")
            return
        dfa = self.df_ab.loc[self.df_ab.type.str.startswith(asr_type),
                             ['text', 'metric_integral_a', 'metric_integral_b',
                              'diff_', 'asr_text_a', 'asr_text_b', 'type']]
        display(dfa.sample(frac=1).head(n_rows).sort_values(by='diff_'))

    def display_changed_scenario_summary(self, n_rows=10):

        self.display_df(self.df_scen_changed[:n_rows])

    def display_changed_scenario_directions(self, n_rows=10):
        self.display_df(self.df_scen_changed_dir[:n_rows])

    def display_same_scenario_summary(self, n_rows=10):
        self.display_df(self.df_same_scenario[:n_rows])

    def display_same_scenario_examples(self, scenario, diff_sign='neg', n_rows=10):
        if self.hide_examples:
            print("Показ примеров отключен.")
            return

        if self.df_same is None or self.df_same.shape[0] == 0:
            display('<b> В этом разделе нет изменений. </b>')
            return

        if diff_sign.lower()[:3] == 'neg':
            compare_fn = __lt__
        elif diff_sign.lower()[:3] == 'pos':
            compare_fn = __gt__
        elif diff_sign.lower()[:3] in ['all', 'any']:
            compare_fn = __ne__
        else:
            raise ValueError("diff_sign can only be 'positive', 'negative' or 'any'.")
        self.display_diffs(
            self.df_same[(self.df_same.generic_scenario_a == scenario) &
                         (compare_fn(self.df_same.diff_, 0))] \
                .sample(frac=1)[:n_rows] \
                .sort_values(by='diff_', ascending=False))

    def display_top_scen_examples(self, n_rows=5, min_contrib=0.0005):
        if self.hide_examples:
            print("Показ примеров отключен.")
            return

        if self.df_same_scenario is None or self.df_same_scenario.shape[0] == 0:
            display('<b> В этом разделе нет изменений. </b>')
            return

        top_same_scenarios = [self.df_same_scenario.index[0], ] + \
                             [s for s in self.df_same_scenario.index[1:] if
                              abs(self.df_same_scenario.loc[s, 'contrib']) > min_contrib]
        for s in top_same_scenarios:
            display(HTML(f"<h3>выборка изменений в сценарии '{s}':</h3>"))
            self.display_same_scenario_examples(s, 'any', n_rows=n_rows)

    def display_nan_summary(self):
        self.display_df(self.df_nan)

    def print_log(self, key="LOAD"):
        for line in self.log:
            if line[0] == key:
                display(HTML(line[1]))

    def display_diff_tables(self, with_examples=False):
        """ выводит в одну ячейку все основные отчеты
        и избранные примеры (если with_examples=True) """

        basket_style = 'width: 100%; border-bottom: 2px solid #000; text-align:left;'
        display(HTML(f'<H1 style="{basket_style}"><br>Корзина: "{self.basket}"</h1>'))

        display(HTML("<H2><br>00. Используемые данные</h2>"))

        self.print_log(key='LOAD')
        display(HTML("<H2><br>0. Ключевые компоненты различий</h2>"))
        self.display_key()

        display(HTML("<H2><br>1. ASR factors</h2>"))
        self.display_asr_summary()
        if with_examples:
            display(HTML("<H3><br>ASR examples</h3>"))
            self.display_asr_examples(10, asr_type='asr')

        display(HTML("<H2><br>2. Изменения МЕЖДУ СЦЕНАРИЯМИ</h2>"))
        display(HTML("<H3><br>Сценарии с наибольшим влиянием при изменении классификации сценария</h3>"))
        self.display_changed_scenario_summary()
        display(HTML("<H3><br>Популярные направления изменения классификации сценариев</h3>"))
        self.display_changed_scenario_directions()

        display(HTML("<H2><br>3. Изменения ВНУТРИ сценариев</h2>"))
        self.display_same_scenario_summary()
        if with_examples:
            display(HTML("<H3><br>Выборки из сценариев со значительным влиянием:</h3>"))
            #     критерий: №1 по числу + все с contrib > 0.0005
            self.display_top_scen_examples(n_rows=10, min_contrib=0.0005)

        display(HTML("<H2><br>Изменения по причине отсутствия значений (NANs)</h2>"))
        self.display_nan_summary()


class MultiDiff(object):

    def __init__(self, table_a, table_b, yt_token=None, print_multi_summary=False, hide_examples=False):
        if type(table_a) != pd.DataFrame:  # allows to pass pre-loaded tables
            table_a = DiffTool.read_yt_table(table_a, yt_token)
        if type(table_b) != pd.DataFrame:  # allows to pass pre-loaded tables
            table_b = DiffTool.read_yt_table(table_b, yt_token)

        self.baskets = table_a.basket.unique().tolist()
        self.diffs = dict()
        print('processing baskets:')
        for b in self.baskets:
            self.diffs[b] = DiffTool(table_a, table_b, yt_token=yt_token,
                                     basket=b, hide_examples=hide_examples)
            print(b, end='; ')
        print(f"\n{len(self.baskets)} basket(s) processed.")

        if print_multi_summary and len(self.baskets) > 1:
            self.show_multi_summary()

    def __call__(self, basket):
        return self.diffs[basket]

    def show_multi_summary(self):
        dfm = pd.DataFrame({basket: self.diffs[basket].df_key.contrib for basket in self.baskets}).T
        dfm = dfm.drop(columns = 'no_changes')  # remove column of all zeros

        def highlight_neg(x):
            return ['background-color: lightpink' if v <= -0.001 else '' for v in x]

        def highlight_pos(x):
            return ['background-color: palegreen' if v >= 0.001 else '' for v in x]

        display(dfm.style \
                   .apply(highlight_neg, subset=dfm.columns[1:]) \
                   .apply(highlight_pos, subset=dfm.columns[1:]) \
                   .set_table_styles([
                       {'selector': 'tr', 'props': [('font-size', '12pt'), ('font-family', 'Trebuchet MS',)]},
                       {'selector': 'th', 'props': [('font-size', '12pt'), ('font-family', 'Helvetica',)]},
                       ])
               )


def detect_ue2e_group(df):
    """ detect ue2e group based on most frequent app """
    assert 'app' in df.columns
    app_mode = df.app.value_counts().index[0]
    app_to_group = {'search_app_prod': 'general',
                    'browser_prod': 'general',
                    'yabro_prod': 'general',
                    'navigator': 'navi_auto',
                    'auto': 'navi_auto',
                    'tv': 'tv',
                    'quasar': 'quasar',
                    'yandexmini': 'quasar',
                    'small_smart_speakers': 'quasar',
                    'yandexmax': 'quasar',
                    }
    return app_to_group.get(app_mode)


def detect_date(table_name):
    """ detects date string, if present at the end of MR table name
    return date string or None if invalid or not detected"""
    date_string = table_name.split('/')[-1]
    try:
        pd.to_datetime(date_string)
        return date_string
    except:
        return None


def add_previous_table(table_a=None, table_b=None):
    """ adds previous table name if only one table name is passed
        fails if only 1 table name provided that does not end with yyyy-mm-dd
    """

    def get_previous_table_name(input_table):
        DATE_FORMAT = '%Y-%m-%d'
        input_date = input_table.split('/')[-1]
        previous_date = (datetime.strptime(input_date, DATE_FORMAT) - timedelta(days=1)).strftime(DATE_FORMAT)
        previous_table = input_table.replace(input_date, previous_date)
        return previous_table

    if table_a is not None and table_b is None:
        table_a, table_b = table_b, table_a
    if table_a is None or table_a == table_b:
        assert re.match(r'\d\d\d\d-\d\d-\d\d', table_b.split('/')[-1]), "if not a cron table - provide both table names"
        table_a = get_previous_table_name(table_b)
        print(f"добавлено имя прошлой таблицы: {table_a}")
    return table_a, table_b
