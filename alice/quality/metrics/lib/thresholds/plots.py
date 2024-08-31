from operator import itemgetter

import numpy as np
import plotly.graph_objects as go

from plotly.io import to_html
from plotly.subplots import make_subplots


def _add_subplot(plots, name, x_values, x_name, y_values, y_name, thresholds, optimums, row, col):
    main_line = go.Scatter(
        x=x_values,
        y=y_values,
        text=[
            f'{x_name} = {x:.4f}, {y_name} = {y:.4f}, Threshold = {t:.4f}'
            for x, y, t in zip(x_values, y_values, thresholds)
        ],
        hoverinfo='text',
        name=name,
    )

    plots.add_trace(main_line, row=row, col=col)
    plots.update_xaxes(title_text=x_name, row=row, col=col)
    plots.update_yaxes(title_text=y_name, row=row, col=col)

    for optimum_type, optimum in optimums.items():
        threshold_idx = np.where(thresholds == optimum.optimal_threshold)

        x, = x_values[threshold_idx]
        y, = y_values[threshold_idx]
        text = f'Optimum value for {optimum_type}={optimum.optimal_metric_value:.4f}\n' \
            f'threshold = {optimum.optimal_threshold:.4f}\n{x_name} = {x:.4f}\n{y_name} = {y:.4f}'

        point = go.Scatter(
            x=[x],
            y=[y],
            text=[text],
            mode='markers',
            marker=dict(color='Green', symbol='circle', size=10),
            name=f'{optimum_type} optimum',
        )

        plots.add_trace(point, row=row, col=col)


def plot_curves_with_optimums(roc_curve, precision_recall_curve, optimums):
    plots = make_subplots(
        rows=1,
        cols=2,
        subplot_titles=('Precision-Recall', 'ROC')
    )

    precision, recall, thresholds = precision_recall_curve
    _add_subplot(
        plots, 'Precision-Recall', recall, 'recall', precision, 'precision', thresholds, optimums, row=1, col=1
    )

    fpr, tpr, thresholds = roc_curve
    _add_subplot(
        plots, 'ROC', fpr, 'FPR', tpr, 'TPR', thresholds, optimums, row=1, col=2
    )

    plots.update_layout(hovermode='closest', xaxis=dict(hoverformat='.4f'), yaxis=dict(hoverformat='.4f'))

    return plots


_COMBINE_HTML_PAGE_TEMPLATE = \
"""
<html>
    <head>
        <meta charset="utf-8">
        <script>
            window.addEventListener('load', function () {{
                document.querySelector(".loader-container").remove();

                var elements = document.getElementsByTagName('details');
                for (var element of elements) {{
                    element.style.removeProperty('visibility');
                    element.open = false;
                }}
            }})
        </script>
        <style>
            .loader {{
                border: 16px solid #f3f3f3; /* Light grey */
                border-top: 16px solid #3498db; /* Blue */
                border-radius: 50%;
                width: 120px;
                height: 120px;
                animation: spin 2s linear infinite;
            }}

            @keyframes spin {{
                0% {{ transform: rotate(0deg); }}
                100% {{ transform: rotate(360deg); }}
            }}
        </style>
    </head>
    <body>
        <div class="loader-container" style="justify-content: center; display: flex;">
            <div class="loader"></div>
        </div>
        {}
    </body>
</html>
"""

_COMBINE_PLOT_TEMPLATE = \
"""
        <details open style="visibility: hidden;">
            <summary>{name} plot</summary>
            {html_plot}
        </details>
"""


def combine_plots_in_html(name2plot: dict):
    html_plots = []

    for name, plot in sorted(name2plot.items(), key=itemgetter(0)):
        html_plot = to_html(plot, include_plotlyjs='cdn', full_html=False)
        html_plots.append(
            _COMBINE_PLOT_TEMPLATE.format(name=name, html_plot=html_plot)
        )

    return _COMBINE_HTML_PAGE_TEMPLATE.format('\n'.join(html_plots))
