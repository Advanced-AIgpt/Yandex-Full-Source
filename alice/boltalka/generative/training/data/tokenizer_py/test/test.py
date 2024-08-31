# coding=utf-8

from tokenizer import Tokenizer


def test():
    tokenizer = Tokenizer('bpe.voc'.encode('utf-8'))
    assert tokenizer.tokenize('приветушки как дела ?'.encode('utf-8')).decode('utf-8') == 'привет `уш `ки как дела ?'
