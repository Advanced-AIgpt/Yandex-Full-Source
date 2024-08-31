import argparse
import cherrypy
import json

class GenaSentencePieceTokenizer():
    NEW_LINE = '[NL]'
    UNK = 0
    BOS = 1
    EOS = 2
    BOS_TOKEN = '<s>'
    EOS_TOKEN = '</s>'

    def __init__(self, vocab_file):
        import sentencepiece as spm

        self._tokenizer = spm.SentencePieceProcessor(model_file=vocab_file)
        self._vocab_words = self._get_vocab_words()
        self.vocab = {token: id for id, token in enumerate(self._vocab_words)}
        self.inverse_vocab = {id: token for id, token in enumerate(self._vocab_words)}
        self.inv_vocab = self.inverse_vocab  # dummy copy for some methods (like predict in pretrain)

    def tokenize(self, line):
        line = line.replace('\n', GenaSentencePieceTokenizer.NEW_LINE)
        return self._tokenizer.encode(line, out_type=str)

    def convert_tokens_to_ids(self, tokens):
        return self._tokenizer.piece_to_id(tokens)

    def convert_ids_to_tokens(self, ids):
        return [self.inverse_vocab[idx] for idx in ids]

    def get_tokens(self):
        return self._vocab_words

    def _get_vocab_words(self):
        indices = list(range(self._tokenizer.GetPieceSize()))
        return self._tokenizer.id_to_piece(indices)


@cherrypy.expose
class SentencePieceServer:

    def __init__(self, tokenizers):
        self.tokenizers = tokenizers

    @cherrypy.expose
    @cherrypy.tools.json_in()
    def default(self):
        data = json.loads(cherrypy.request.json)
        tokens = self.tokenizers[data['model_name']].tokenize(data['text'])
        return json.dumps(tokens)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)

    parser.add_argument('--port', type=int, default=7293)
    parser.add_argument('--models-paths', required=True)

    args = parser.parse_args()

    models = json.loads(args.models_paths)

    cherrypy.config.update({'server.socket_port': int(args.port)})
    tokenizers = {model_name: GenaSentencePieceTokenizer(models[model_name]) for model_name in models}
    cherrypy.quickstart(SentencePieceServer(tokenizers))