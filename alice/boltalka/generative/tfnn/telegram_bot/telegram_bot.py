# coding=utf-8
from __future__ import print_function, unicode_literals, division

import argparse
import json
import logging
import re
import time
import threading
from copy import copy

import requests
import telegram
from telegram.ext import Updater, MessageHandler, Filters, CommandHandler
from ml_data_reader.iterators import gather
from tfnn.score import score
from tokenizer import Tokenizer

from alice.boltalka.generative.tfnn.infer_lib.infer import load_model, infer_model
from alice.boltalka.generative.tfnn.preprocess import preprocess_contexts


logging.basicConfig(
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s', level=logging.INFO
)

from flask import Flask, request
app = Flask("zeliboba")


@app.route('/ask_bot')
def default():
    text = request.args.get('text')
    options = json.loads(request.args.get('options') or '{}')
    try:
        body = request.get_json()
        text = text or body.get('text')
        options = options or body.get('options', {})
    except:
        pass
    if not text:
        return 'Request is empty', 400
    with model_lock:
        return global_handler.do_call(text, {'user_params': options})


@app.route('/score')
def score_handler():
    text = request.args.get('text')
    try:
        text = text or request.get_json().get('text')
    except ValueError:
        pass
    if not text:
        return 'Bad request, text is empty', 400
    return global_handler.score(text)


def get_default_input(mode):
    input_map = {
        'seq2seq': [''],
        'insertion_transformer': ['', '']
    }
    return copy(input_map[mode])


class TokenizerClient:
    def __init__(self, model_name, url):
        self.model_name = model_name
        self.url = url

    def tokenize(self, text):
        response = requests.post(self.url, json=json.dumps({'text': text, 'model_name': self.model_name}))
        return " ".join(response.json())


class TokenizerWrapper:
    def __init__(self, tokenizer):
        self.tokenizer = tokenizer

    def tokenize(self, text):
        return self.tokenizer.tokenize(text.encode('utf-8')).decode('utf-8')


model_lock = threading.Lock()


class TransaliceBotHandler:
    def __init__(self, args):
        self.args = args
        self.models_map = dict()
        self.models_args_map = self._read_args(args)
        self.tokenizers_map = self._init_tokenizers(self.models_args_map)

    def _read_args(self, args):
        model_names = args.model_name
        model_args = [
            'model_name', 'model', 'mode', 'model_path', 'tokenizer_settings', 'token_to_id_voc_path', 'hp'
        ]

        args_dict = vars(args)

        if not all(len(model_names) == len(args_dict[arg]) for arg in model_args):
            raise ValueError('If you specify many models, you need to propagate all other models params to them')

        sliced_dict = {key: args_dict[key] for key in model_args}

        model_args_dicts = [{} for _ in range(len(model_names))]
        for key, values in sliced_dict.items():
            for dict, value in zip(model_args_dicts, values):
                dict[key] = value

        model_args_map = {dict['model_name']: dict for dict in model_args_dicts}
        return model_args_map

    def _init_tokenizers(self, models_args_map):
        result = {}
        for model_name in models_args_map:
            tokenizer_settings = json.loads(models_args_map[model_name]['tokenizer_settings'])
            if 'tokenizer_mode' in tokenizer_settings and tokenizer_settings['tokenizer_mode'] == 'RemoteTokenizer':
                result[model_name] = TokenizerClient(model_name, url=tokenizer_settings['tokenizer_url'])
            else:
                bpe_voc_path = tokenizer_settings['bpe_voc_path']
                result[model_name] = TokenizerWrapper(Tokenizer(bpe_voc_path.encode('utf-8')))
        return result

    def check_username(self, username):
        if self.args.no_auth:
            return True

        try:
            if username is None:
                return False

            result = requests.get(
                url='https://staff-api.yandex-team.ru/v3/persons',
                headers={
                    'Authorization': 'OAuth {}'.format(self.args.staff_token),
                },
                params={
                    '_one': 1,
                    '_query': 'accounts==match({{"type":"telegram","value_lower":"{}"}})and(official.is_dismissed==False)'.format(
                        username.lower())
                }
            ).json()

            return 'official' in result and not result['official'].get('is_dismissed', True)
        except Exception:
            return False

    def __call__(self, bot, update, chat_data):
        try:
            if not self.check_username(update.message.from_user.username):
                message = 'System: кажется, ты не можешь мне писать :( обратись к тому, кто дал тебе бота'
            else:
                with model_lock:
                    message = self.do_call(update.message.text, chat_data)
            update.message.reply_text(message, parse_mode=telegram.ParseMode.HTML)
        except Exception as ex:
            update.message.reply_text('System: что-то пошло не так, попробуй повторить запрос или напиши Артему')
            raise ex

    def maybe_init_model(self, model_name):
        if model_name not in self.models_map:
            print('Loading {}'.format(model_name))
            args = self.models_args_map[model_name]
            self.models_map[model_name] = load_model(
                args['model'],
                args['model_path'],
                args['token_to_id_voc_path'],
                args['token_to_id_voc_path'],
                args['hp']
            )
        return self.models_map[model_name]

    def maybe_init_chat_data(self, chat_data):
        if 'context' not in chat_data:
            chat_data['context'] = []

        default_options = {
            'topk': self.args.default_topk,
            'n_hypos': self.args.default_n_hypos,
            'temperature': self.args.default_temperature,
            'context_len': self.args.default_context_len,
            'max_len': self.args.default_max_len,
            'include_scores': True,
            'model': self.args.default_current_model_name,
            'eos_penalty_insertion_transformer': self.args.eos_penalty_insertion_transformer,
            'ingraph_mode': self.args.ingraph_mode,
        }

        default_options.update(chat_data.get('user_params', {}))
        chat_data['user_params'] = default_options

    def _format_user_params(self, dict):
        return '\n'.join('{}: <b>{}</b>'.format(key, dict[key]) for key in sorted(dict.keys()))

    def handle_user_params_request(self, text, user_params):
        matches = re.findall('([a-zA-Z_]+)=(.+)', text)

        if len(matches) == 0:
            return 'System: текущие настройки: \n{}'.format(self._format_user_params(user_params))

        for key, value in matches:
            if key not in user_params:
                return 'System: не существует настройки с ключом `{}`. Вот настройки: \n{}'.format(
                    key, self._format_user_params(user_params)
                )

            if key == 'model':
                model_names = list(self.models_args_map.keys())
                if value not in model_names:
                    return 'System: Неправильное имя модели, возможные значения: {}'.format(model_names)

            necessary_type = type(user_params[key])

            try:
                if necessary_type is bool:
                    if not value.lower() in ['true', 'false']:
                        raise ValueError()

                    new_param = value.lower() == 'true'
                else:
                    new_param = necessary_type(value)
            except Exception:
                return 'System: не могу сделать для ключа `{}` объект типа `{}` '\
                       'из значения `{}`. Вот настройки: \n{}'.format(key,
                                                                      str(necessary_type),
                                                                      value,
                                                                      self._format_user_params(user_params))

            user_params[key] = new_param

        return 'System: настройки изменены. Вот они: {}'.format(self._format_user_params(user_params))

    def update_context(self, context, tokenized_text, raw_text):
        new_context = {
            'tokenized': tokenized_text,
            'raw': raw_text
        }
        context.append(new_context)

        while len(context) > 10:
            context.pop(0)

    def gather_context_for_generation(self, context, context_len):
        return [el['tokenized'] for el in context[-context_len:]]

    def do_call(self, text, chat_data):
        self.maybe_init_chat_data(chat_data)

        user_params = chat_data['user_params']
        context = chat_data['context']

        if '/help' in text:
            return 'System: бот для общения с болталкой с генеративной моделью. ' \
                   '\nПросто пиши реплику и моделька постарается ответить на нее. ' \
                   'При этом реплики и твои, и бота записываются в контекст и будут использоваться в дальнейшем ' \
                   'диалоге. ' \
                   '\nВ боте есть разные настройки и может быть загружено несколько моделей, для их просмотра и ' \
                   'изменения используй /options. ' \
                   '\nЧтобы забыть контекст используй /obliviate, чтобы его просмотреть используй /context. ' \
                   '\nПросмотреть все загруженные модели можно через /models.'

        if '/obliviate' in text:
            chat_data['context'] = []
            return 'System: контекст сброшен'

        if '/context' in text:
            return '\n'.join(
                ['System: текущий контекст:'] +
                ['- {}'.format(ctx['raw']) for ctx in context[-user_params['context_len']:]]
            )

        if '/options' in text:
            return self.handle_user_params_request(text, user_params)

        if '/models' in text:
            return 'System: загруженные в бота модели: <b>{}</b>'.format(list(self.models_args_map.keys()))

        if '<>' in text:
            items = [t.strip() for t in text.split('<>')]
            text = items[0]
            additional_inputs = [self.preprocess_and_tokenize(t, self.tokenizers_map[user_params['model']]) for t in items[1:]]
        else:
            additional_inputs = []

        input_subcontexts = text.split('[]')  # user query may be with separators
        for input_subcontext in input_subcontexts:
            input_subcontext = input_subcontext.strip()
            self.update_context(
                context,
                self.preprocess_and_tokenize(
                    input_subcontext,
                    self.tokenizers_map[user_params['model']]
                ),
                input_subcontext
            )

        tokenized_context_list = self.gather_context_for_generation(context, user_params['context_len'])
        tokenized_context_string = ' {} '.format(self.args.separator_token).join(tokenized_context_list)

        inputs = get_default_input(self.models_args_map[user_params['model']]['mode'])

        for i, value in enumerate([tokenized_context_string] + additional_inputs):
            inputs[i] = value

        logging.info('Generation on: {}'.format(inputs))

        replies, scores, consumed_time = self.generate_replies(inputs, user_params, self.args.batch_size)

        logging.info('Got replies: {}'.format(replies))
        replies = self.postprocess_reply(replies)
        logging.info('postprocessed replies: {}'.format(replies))

        to_update_context = replies[0]
        self.update_context(
            context,
            self.preprocess_and_tokenize(
                to_update_context,
                self.tokenizers_map[user_params['model']]
            ),
            to_update_context
        )

        if user_params['include_scores']:
            replies = ['<b>{}</b> (score: {:.2f})'.format(reply, score) for reply, score in zip(replies, scores)]

        return '\n\n'.join(replies) + '\nSystem: generated in {:.2f}s'.format(consumed_time)

    def postprocess_reply(self, replies):
        replies = [re.sub("(?: ▁\\.| ▁)+$", "", reply) for reply in replies]
        replies = [reply.replace("[ NL ]", "\n") for reply in replies]
        replies = [reply.replace("[NL]", "\n") for reply in replies]
        replies = [reply.replace("[SEP]", "\n–") for reply in replies]
        replies = [reply.replace(" ", "") for reply in replies]
        replies = [reply.replace("▁", " ") for reply in replies]
        return replies

    def generate_replies(self, inputs, user_params, batch_size):
        model, session, graph = self.maybe_init_model(user_params['model'])

        start_time = time.time()

        with session.as_default(), graph.as_default():
            results = infer_model(
                model,
                [inputs],
                hypothesis_per_input=user_params['n_hypos'],
                batch_size=batch_size,
                unbpe=True,
                yield_score=True,
                max_len=user_params['max_len'],
                temperature=user_params['temperature'],
                sampling_top_k=user_params['topk'],
                eos_penalty=user_params['eos_penalty_insertion_transformer'],
                mode=user_params['ingraph_mode']
            )

        end_time = time.time()
        results, scores = zip(*[(r.out, r.score) for r in results])

        return results, scores, end_time - start_time

    def preprocess_and_tokenize(self, text, tokenizer):
        if self.args.enable_query_preprocess:
            text = preprocess_contexts([text])[0]

        return tokenizer.tokenize(text)

    def score(self, text):
        chat_data = {}
        self.maybe_init_chat_data(chat_data)
        user_params = chat_data['user_params']
        tokenized_text = self.preprocess_and_tokenize(text, self.tokenizers_map[user_params['model']])

        model, session, graph = self.maybe_init_model(user_params['model'])
        with session.as_default(), graph.as_default():
            for scores in gather(score(model, session, [model.make_feed_dict([('', tokenized_text)])], 'none')):
                return dict(scores=scores[0].tolist(), tokens=tokenized_text.split(' ') + ['</s>'])


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--host', default='localhost')
    parser.add_argument('--port', type=int, default=3667)
    parser.add_argument('--telegram-token')
    parser.add_argument('--staff-token')

    parser.add_argument('--no-auth', type=int, default=0)

    parser.add_argument('--disable-bot', action='store_true')
    parser.add_argument('--api-enable', type=int, default=0)
    parser.add_argument('--api-port', type=int, default=19900)

    parser.add_argument('--model-name', required=True, action='append')
    parser.add_argument('--model', required=True, action='append')
    parser.add_argument('--model-path', required=True, action='append')
    parser.add_argument('--mode', choices=['seq2seq', 'lm', 'insertion_transformer'], required=True, action='append')
    parser.add_argument('--ingraph-mode', choices=['sample', 'greedy', 'beam_search'], default='sample')
    parser.add_argument('--hp', type=lambda x: json.loads(x), required=True, action='append')
    parser.add_argument('--lock', action='store_true')

    parser.add_argument('--tokenizer-settings', required=True, action='append')
    parser.add_argument('--token-to-id-voc-path', required=True, action='append')

    parser.add_argument('--batch-size', type=int, default=1)
    parser.add_argument('--default-context-len', type=int, default=3)
    parser.add_argument('--default-max-len', default=200, type=int)
    parser.add_argument('--default-temperature', default=0.6)
    parser.add_argument('--default-topk', default=50)
    parser.add_argument('--default-n-hypos', default=1)
    parser.add_argument('--default-current-model-name', required=True)
    parser.add_argument('--enable-query-preprocess', type=int, default=1)
    parser.add_argument('--separator-token', default='[SPECIAL_SEPARATOR_TOKEN]')

    parser.add_argument('--eos-penalty-insertion-transformer', default=2.0)
    args = parser.parse_args()

    if args.mode == 'lm':
        raise ValueError('LM is not supported yet (in TODO)')  # TODO

    updater = Updater(token=vars(args)['telegram_token'], use_context=False)
    dispatcher = updater.dispatcher
    global global_handler
    global_handler = TransaliceBotHandler(args)

    if args.lock:
        for model_name in global_handler.models_args_map:
            global_handler.maybe_init_model(model_name)

    assert not args.disable_bot or args.api_enable

    if not args.disable_bot:
        dispatcher.add_handler(MessageHandler(Filters.text, global_handler, pass_chat_data=True))
        for command in ['options', 'models', 'obliviate', 'context', 'help']:
            dispatcher.add_handler(CommandHandler(command, global_handler, pass_chat_data=True))
        updater.start_polling()

    if args.api_enable:
        app.run(host="::", port=args.api_port, threaded=True)
    else:
        updater.idle()
