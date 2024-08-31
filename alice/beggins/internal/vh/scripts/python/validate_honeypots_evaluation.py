import yt.wrapper as yt


def validate_honeypots_evaluation(evaluated_honeypots_table, yt_token, threshold: float):
    yt.config.config['token'] = yt_token
    yt.config.config['proxy']['url'] = evaluated_honeypots_table['cluster']

    error_message = str()
    evaluated_honeypots_table = list(yt.read_table(evaluated_honeypots_table['table'], format='json'))
    for evaluated_honeypot in evaluated_honeypots_table:
        if evaluated_honeypot['target'] == 0 and evaluated_honeypot['score'] > threshold:
            error_message += f'false positive: <{evaluated_honeypot["text"]}> with score {evaluated_honeypot["score"]}\n'
        if evaluated_honeypot['target'] == 1 and evaluated_honeypot['score'] < threshold:
            error_message += f'false negative: <{evaluated_honeypot["text"]}> with score {evaluated_honeypot["score"]}\n'

    if error_message:
        raise Exception(error_message)


def main(in1, in2, in3, mr_tables, token1=None, token2=None, param1=None, param2=None, html_file=None):
    return validate_honeypots_evaluation(mr_tables[0], token1, float(param1))
