import json
import os

import requests
from tokenizer import Tokenizer
import argparse

import alice.boltalka.generative.tfnn.infer_lib.infer as infer
import alice.boltalka.generative.tfnn.preprocess as preprocess
from alice.boltalka.telegram_bot.lib.module import Module
from alice.boltalka.telegram_bot.lib.cache import Cache
from alice.boltalka.telegram_bot.lib.rpc import RpcSourceServer

ENTITYSEARCH_QUERY = 'http://entitysearch.yandex.net/get?obj=kin0{}'

# Will be filled on start. Defines what models are available for the server.
# (name -> model_folder)
MODELS_REGISTRY = dict()


def maybe_append_with_token(list, str, token):
    if str is not None and str != '':
        list.append(token)
        list.append(str)


def preprocess_text(text, tokenizer):
    preprocessed = preprocess.preprocess_contexts([text])
    text = preprocessed[0] if len(preprocessed) > 0 else ''
    return tokenizer.tokenize(text.encode('utf-8')).decode('utf-8')


class TfnnModelSource(Module):
    class Options:
        sampling_mode = 'sample'
        sampling_temperature = 0.6
        sampling_topk = 50
        model_name = 'md_all'

        entity_id = None

    def __init__(self):
        self.model_data = Cache(lambda name: self.load_model(MODELS_REGISTRY[name]))

    def load_model(self, folder):
        with open(os.path.join(folder, 'hp.json')) as f:
            model_hp = json.load(f)

        tokenizer = Tokenizer(os.path.join(folder, 'bpe.voc').encode('utf-8'))
        model, session, graph = infer.load_model(
            model='tfnn.task.seq2seq.models.transformer.Model',
            model_path=os.path.join(folder, 'model.npz'),
            ivoc_path=os.path.join(folder, 'token_to_id.voc'),
            hp=model_hp
        )
        return tokenizer, model, session, graph

    def set_args(self, args):
        super().set_args(args)
        self.model_data.choose(self.model_name)
        self.tokenizer, self.model, self.session, self.graph = self.model_data.get()

    def get_candidates(self, args: dict, context: list):
        self.set_args(args)
        context = [preprocess_text(ctx, self.tokenizer) for ctx in context]

        context = separator = ' {} '.format('[SPECIAL_SEPARATOR_TOKEN]').join(context)
        context.lstrip(separator)

        if self.entity_id is not None:
            query_path = ENTITYSEARCH_QUERY.format(self.entity_id)
            response = requests.get(query_path)
            if not response.ok:
                return [dict(text='Entity search returned {} code, generation failed', relevance=0)]
            entity_search_data = response.json()
            movie_name = preprocess_text(entity_search_data['cards'][0]['base_info']['name'], self.tokenizer)
            director, actor, genre = None, None, None
            for snippet in entity_search_data['cards'][0]['wiki_snippet']['item']:
                if snippet['name'] == 'Жанр':
                    genre = preprocess_text(snippet['value'][0]['text'], self.tokenizer)
                elif snippet['name'] == 'Режиссёр':
                    director = preprocess_text(snippet['value'][0]['text'], self.tokenizer)

            for obj in entity_search_data['cards'][0]['related_object']:
                if obj['type'] == 'team':
                    actor = preprocess_text(obj['object'][0]['name'], self.tokenizer)

            print('KP_ID: {}, movie_name: {}, director: {}, actor: {}, genre: {}'.format(
                self.entity_id, movie_name, director, actor, genre
            ))

            movie_data = []
            maybe_append_with_token(movie_data, movie_name, '[MOVIEDATA_TITLE]')
            maybe_append_with_token(movie_data, director, '[MOVIEDATA_DIRECTOR]')
            maybe_append_with_token(movie_data, actor, '[MOVIEDATA_ACTOR]')
            maybe_append_with_token(movie_data, genre, '[MOVIEDATA_GENRE]')
            movie_data.append('[MOVIE_DATA_SEP]')

            context = ' '.join(movie_data + [context])

        print('Context: {}'.format(context))

        with self.session.as_default(), self.graph.as_default():
            results = infer.infer_model(
                self.model,
                [context],
                temperature=self.sampling_temperature,
                sampling_top_k=self.sampling_topk,
                hypothesis_per_input=1,
                mode=self.sampling_mode,
                batch_size=1,
                unbpe=True,
                yield_score=True
            )
            reply, score = results[0]
            reply, score = preprocess.process_response(reply), float(score)

            print('Reply: {} (score: {:.2f})'.format(reply, score))

            return [dict(text=reply, relevance=0.0)]  # TODO include score


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--model-folders', required=True
    )
    parser.add_argument(
        '--model-names', required=True
    )
    parser.add_argument(
        '--port', type=int, required=True
    )
    args = parser.parse_args()

    model_folders = args.model_folders.split(',')
    model_names = args.model_names.split(',')
    if len(model_folders) != len(model_names):
        raise ValueError('--model-folders and --model-names should be the same size.')

    for name, folder in zip(model_names, model_folders):
        MODELS_REGISTRY[name] = folder

    server = RpcSourceServer('localhost', args.port, [TfnnModelSource])
    print('Starting server on port: {} with registry {}'.format(args.port, MODELS_REGISTRY))
    server.run()


if __name__ == '__main__':
    main()
