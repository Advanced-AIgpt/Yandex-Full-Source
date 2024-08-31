from __future__ import print_function
from __future__ import unicode_literals
import argparse
from collections import Counter
import yt.wrapper as yt


def is_query_about_math(query):
    math_symbols = '1234567890'
    math_words = [
        'ноль', 'один', 'два', 'три', 'четыре', 'пять', 'шесть', 'семь', 'девять',
        'двенадцать', 'четырнадцать', 'пятнадцать', 'шестнадцать', 'семнадцать', 'девятнадцать',
        'дцать', 'десят',
        'сорок', 'девяносто', 'сто', 'тысяча', 'миллион', 'миллиард',
    ]
    math_ops_symbols = ['=', '+', '/', '*', '-']
    math_ops_words = ['умножить', 'плюс', 'минус', 'вычесть', 'делить']

    math_symbols_count = sum(query.count(ch) for ch in math_symbols)
    math_words_count = sum(query.count(word) for word in math_words)
    math_ops_symbols_count = sum(query.count(word) for word in math_ops_symbols)
    math_ops_words_count = sum(query.count(word) for word in math_ops_words)

    query_words = query.split()
    query_symbols_count = sum(len(word) for word in query_words)
    query_words_count = len(query_words)

    if math_ops_words_count != 0:
        return True
    if math_symbols_count == 0 and math_words_count == 0:
        return False
    if (math_symbols_count + math_ops_symbols_count) / query_symbols_count > 0.5:
        return True
    if math_words_count / query_words_count > 0.66:
        return True

    return False


def is_query_about_games(query):
    return 'игр' in query or 'ыгр' in query


def is_query_about_horoscope(query):
    return 'гороскоп' in query


def is_query_about_precise_recipe(query):
    index = query.find('рецепт')
    if index < 0:
        return False

    # Запрос общий, если в нем после слова "рецепт" идет максимум одно слово
    query_after_keyword = query[index:]
    return len(query_after_keyword.split()) > 2


def is_query_one_word(query):
    return len(query.split()) == 1


def is_query_nonsense(query):
    return is_query_one_word(query) or is_query_about_math(query) or is_query_about_precise_recipe(query)


def preprocess_query(query):
    query = query.split()
    return ' '.join(query).lower()


def fix_answer(row):
    if row['answer'] == 'NO':
        return row
    if row['confidence'] > 0.5:
        return row
    if is_query_about_games(row['query']) or is_query_about_horoscope(row['query']):
        return row
    row['answer'] = 'NO'

    return row


def PreparationMapper(row):
    row['query'] = preprocess_query(row['query'])

    yield row


def PreparationReducer(key, rows):
    answers = Counter()
    goldens = Counter()
    results = Counter()
    confidences = Counter()

    skill_name = None
    skill_description = None

    for row in rows:
        skill_name = row['skill_name']
        skill_description = row['skill_description']
        answers[row['answer']] += 1
        goldens[row['golden']] += 1
        results += Counter(row['results'])
        confidences[row['answer']] += 0 if row['confidence'] is None else row['confidence']

    del answers[None]
    del goldens[None]
    del results[None]
    del confidences[None]

    result = {
        'query': key['query'],
        'skill_id': key['skill_id'],
        'skill_name': skill_name,
        'skill_description': skill_description
    }

    if is_query_nonsense(key['query']):
        result['aggregate_type'] = 'nonsense_query'
        result['answer'] = 'NONSENSE'
        result['confidence'] = 1
    elif len(goldens) > 0:
        result['aggregate_type'] = 'goldens'
        result['answer'] = goldens.most_common(1)[0][0]
        result['confidence'] = 1
    elif len(results) > 0:
        result['aggregate_type'] = 'results'
        result['answer'] = results.most_common(1)[0][0]
        result['confidence'] = results.most_common(1)[0][1] / sum(results.values())
    elif len(answers) > 0:
        result['aggregate_type'] = 'answers'
        result['answer'] = answers.most_common(1)[0][0]
        result['confidence'] = confidences[result['answer']] / answers.most_common(1)[0][1]

    if 'answer' in result:
        yield fix_answer(result)


def NonsenseMapper(row):
    if row['answer'] == 'NONSENSE':
        yield row


def NonsenseReducer(key, rows):
    result = {
        'query': key['query'],
        'answer': 'NONSENSE'
    }

    yield result


def YesNoReducer(key, rows):
    for row in rows:
        if 'skill_id' not in row:  # NONSENSE, skip all
            return
        yield row


def main():
    parser = argparse.ArgumentParser(prog='Toloka cleaner')
    parser.add_argument('--yt-cluster', required=True)
    parser.add_argument('--yt-input', required=True)
    parser.add_argument('--yt-output-yes-no', required=True)
    parser.add_argument('--yt-output-nonsense', required=True)
    args = parser.parse_args()

    client = yt.YtClient(args.yt_cluster)
    yt_input_nodes = client.list(args.yt_input)
    yt_input_tables = []
    for node in yt_input_nodes:
        if client.get(f'{args.yt_input}/{node}/@type') == 'table':
            yt_input_tables.append(f'{args.yt_input}/{node}')
    print(f'Input table: {len(yt_input_tables)}')

    yt_after_prep_table = client.create_temp_table()
    client.run_map_reduce(
        PreparationMapper,
        PreparationReducer,
        yt_input_tables,
        yt_after_prep_table,
        reduce_by=['query', 'skill_id'],
        format=yt.JsonFormat(attributes={"encode_utf8": False})
    )
    client.run_map_reduce(
        NonsenseMapper,
        NonsenseReducer,
        yt_after_prep_table,
        args.yt_output_nonsense,
        reduce_by=['query'],
        format=yt.JsonFormat(attributes={"encode_utf8": False})
    )
    client.run_sort(yt_after_prep_table, sort_by=['query'])
    client.run_sort(args.yt_output_nonsense, sort_by=['query'])
    client.run_reduce(
        YesNoReducer,
        [args.yt_output_nonsense, yt_after_prep_table],
        args.yt_output_yes_no,
        reduce_by=['query'],
        format=yt.JsonFormat(attributes={"encode_utf8": False})
    )


if __name__ == '__main__':
    main()

