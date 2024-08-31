from alice.beggins.internal.vh.operations import ext

import vh3

from vh3.decorator import graph
from vh3.extras.json import dump


def calculate_toloka_parameters_script() -> vh3.Expr:
    return vh3.Expr('''
parameters_from_tom = v
parameters_from_user = w

result = {
    'classification_task_question': None,
    'classification_project_instructions': None,
    'positive_honeypots': None,
    'negative_honeypots': None,
}

if parameters_from_tom is not None:
    result['classification_task_question'] = parameters_from_tom['classification-task-question']
    result['classification_project_instructions'] = parameters_from_tom['classification-project-instructions']
    result['positive_honeypots'] = parameters_from_tom['positive_honeypots']
    result['negative_honeypots'] = parameters_from_tom['negative_honeypots']

for parameter in result:
    if parameters_from_user[parameter] is not None:
        result[parameter] = parameters_from_user[parameter]

for parameter in result:
    if result[parameter] is None:
        raise Exception(f'The parameter <{parameter}> is missed')

return result
''')


def escape_string(string: vh3.String) -> vh3.Expr:
    return vh3.Expr(f'${{{string:var}?j_string}}')


def process_json_string(string: vh3.String) -> vh3.Expr:
    return vh3.Expr(f'[#if {string:var}??]"{escape_string(string):expr}"[#else]null[/#if]')


@graph(workflow_id='https://nirvana.yandex-team.ru/flow/d6191de4-e7b0-4da8-b669-cdfefead27d2')
def get_toloka_parameters() -> vh3.JSON:
    """
    Generate Toloka parameters
    :return: Json with parameters values. Example:
    ```
    {
        "classification_task_question": ...,
        "classification_project_instructions": ...,
        "positive_honeypots": ...,
        "negative_honeypots": ...,
    }
    ```
    """
    if_result = ext.light_groovy_json_filter_if(
        filter=vh3.Expr(f'"{vh3.context.tom_creation_url:expr}" != "null"'),
        input=dump({}),
        **vh3.block_args(name='if (tom_creation_url != null)'),
    )
    parameters_from_tom = ext.scrape_tom_graph(
        graph_url=vh3.context.tom_creation_url,
        **vh3.block_args(dynamic_options=if_result.output_true),
    )

    parameters_from_user = ext.single_option_to_json_output(vh3.Expr(f'''
    {{
        "classification_task_question": {process_json_string(vh3.context.classification_task_question):expr},
        "classification_project_instructions": {process_json_string(vh3.context.classification_project_instructions):expr},
        "positive_honeypots": {process_json_string(vh3.context.positive_honeypots):expr},
        "negative_honeypots": {process_json_string(vh3.context.negative_honeypots):expr}
    }}
    '''))

    return ext.python3_any_any_any_to_json(
        input0_type='json-mem', input1_type='json-mem', input2_type='none',
        body=calculate_toloka_parameters_script(),
        input0=parameters_from_tom.options, input1=parameters_from_user,
        **vh3.block_args(name='build toloka parameters'),
    )
