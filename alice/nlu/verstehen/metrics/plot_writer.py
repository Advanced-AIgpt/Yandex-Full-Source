import os
from collections import defaultdict

import matplotlib.pyplot as plt
import numpy as np


class PlotWriter:
    def __init__(self):
        # metrics -> index -> values
        self._values = defaultdict(lambda: defaultdict(list))

    def add_metrics_values(self, index_name, metrics):
        """
        :param index_name: name of index
        :param metrics: dict of list of float, where key is metric name
        :return: void
        """
        for metric_name in metrics:
            self._values[metric_name][index_name] = metrics[metric_name]

    def save(self, path):
        if not os.path.exists(path):
            os.makedirs(path)

        plt_conf = plt.gcf()
        plt_def_size = plt_conf.get_size_inches()
        plt_conf.set_size_inches(plt_def_size * 3)
        for metric_name in self._values:
            plt.title(metric_name)
            bottom_border = np.inf
            top_border = -np.inf
            max_x = 1

            for index_name in self._values[metric_name]:
                current_values = self._values[metric_name][index_name]
                bottom_border = min(np.min(current_values), bottom_border)
                top_border = max(np.max(current_values), top_border)
                current_max_x = len(current_values) + 1
                x = np.arange(1, current_max_x)
                plt.plot(x, current_values, '-', label=index_name)
                max_x = max(current_max_x, max_x)
            plt.legend()
            plt.grid()
            plt.ylim((bottom_border, top_border))
            plt.yticks(np.linspace(bottom_border, top_border, num=60))
            plt.xticks(np.arange(1, max_x))

            plt.savefig(os.path.join(path,  metric_name + ".png"), format="png")
            plt.clf()

        plt_conf.set_size_inches(plt_def_size)
