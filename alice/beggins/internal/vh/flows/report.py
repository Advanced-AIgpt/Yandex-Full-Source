import vh3
from vh3.decorator import graph

from vh3.extras.json import dump

from alice.beggins.internal.vh.flows.common import (
    static_name, EmbedderType
)

from alice.beggins.internal.vh.operations import ext
from alice.beggins.internal.vh.scripts.scripter import Scripter
from alice.beggins.internal.vh.scripts.python import (
    make_report_script
)


@static_name('make report')
@graph(workflow_id='https://nirvana.yandex-team.ru/flow/3ad66ff3-bd77-4bdc-8d3c-f47a64f35e21')
def make_report(
    threshold: vh3.JSON,

    train_metrics: vh3.JSON,
    val_metrics: vh3.JSON,
    accept_metrics: vh3.JSON,

    threshold_plot: vh3.HTML,
    train_matrix: vh3.HTML,
    val_matrix: vh3.HTML,
    accept_matrix: vh3.HTML,

    year_logs_stats: vh3.JSON,
    week_logs_stats: vh3.JSON,

    model_name: vh3.String = vh3.Factory(lambda: vh3.context.model_name),
    embedder: EmbedderType = vh3.Factory(lambda: vh3.context.embedder),
    week: vh3.String = vh3.Factory(lambda: vh3.context.week),
    st_ticket: vh3.String = vh3.Factory(lambda: vh3.context.st_ticket),
    catboost_params: vh3.String = vh3.Factory(lambda: vh3.context.catboost_params)
) -> vh3.HTML:

    all_info = vh3.extras.json.dump({
        'model_name': model_name,
        'embedder': embedder,
        'week':  week,
        'st_ticket': st_ticket,

        'threshold': threshold,
        'graph_link': vh3.expr.meta.workflow_url,
        'catboost_params': catboost_params,

        'train_metrics': train_metrics,
        'val_metrics': val_metrics,
        'accept_metrics': accept_metrics,
        'year_logs_stats': year_logs_stats,
        'week_logs_stats': week_logs_stats,

        'threshold_plot': ext.html_to_text(
            html=threshold_plot,
            **vh3.block_args(name='threshold plot to text')
        ),
        'train_matrix': ext.html_to_text(
            html=train_matrix,
            **vh3.block_args(name='train plot to text')
        ),
        'val_matrix': ext.html_to_text(
            html=val_matrix,
            **vh3.block_args(name='val plot to text')
        ),
        'accept_matrix': ext.html_to_text(
            html=accept_matrix,
            **vh3.block_args(name='accept plot to text')
        )
    }, **vh3.block_args(name='dump all info'))

    template = ext.arc_export_file(
        path='alice/beggins/internal/vh/templates/report.html',
        **vh3.block_args(name='get template')
    ).text

    template_json = vh3.extras.json.dump({
        'template': template,
    }, **vh3.block_args(name='dump template'))

    report = ext.python3_json_process(
        code=Scripter.to_string(make_report_script),
        in1=all_info,
        in2=template_json,
        **vh3.block_args(name='make report')
    ).html_report

    return report


@static_name('send report to ticket')
@graph(workflow_id='https://nirvana.yandex-team.ru/flow/52c8fbe3-dea6-4d38-8078-60a93f5ef9b1')
def send_report_to_ticket(
    report: vh3.HTML,
    model_name: vh3.String = vh3.Factory(lambda: vh3.context.model_name),
    st_ticket: vh3.String = vh3.Factory(lambda: vh3.context.st_ticket)
) -> None:

    need_send_report = ext.light_groovy_json_filter_if(
        filter=vh3.Expr(f'"{st_ticket:expr}" != "null" && "{st_ticket:expr}" != ""'),
        input=dump({}),
        **vh3.block_args(name='if st_ticket'),
    )

    binary_report = ext.convert_any_to_binary_data(
        file=report,
        **vh3.block_args(
            name='convert to binary',
            dynamic_options=need_send_report.output_true,
        )
    )

    comment = vh3.Expr(f"## Отчет по обучению *{model_name:expr}*\n{vh3.expr.meta.workflow_url:expr}")

    ext.star_trek_comment_with_attachments(
        issue_key=vh3.Expr(f'{st_ticket:expr}'),
        message=comment,
        attachments=vh3.Connections(('report.html', binary_report)),
        **vh3.block_args(name='send to tracker')
    )
