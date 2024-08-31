# coding: utf-8

class AbstractNerService(object):
    def get_ner(self, utterance):
        raise NotImplementedError()
