# -*-coding: utf8 -*-
from utils.nirvana.op_caller import call_as_operation


def main(assigns, answers, fielddate=None):
    """
    Оценка надёжности толокерских ответов
    :param list[dict] assigns: Ответы на валидационные вопросы
    :param dict[str, str] answers:  {"task_id": "answer"}
    :param str|None fielddate:
    """
    correct = sum(answers[a['inputValues']['key']] == a['outputValues']['result']
                  for a in assigns)
    accuracy = correct / float(len(assigns))
    return {'fielddate': fielddate, 'accuracy': accuracy}


if __name__ == '__main__':
    call_as_operation(main, input_spec={
        'assigns': {'link_name': 'assigns', 'parser': 'json'},
        'answers': {'link_name': 'answers', 'parser': 'json'},
    })
