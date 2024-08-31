# coding: utf-8

from liblemmer_python_binding import AnalyzeWord

from nlu_service.utils.string import ensure_unicode


def is_valid_word(word):
    word = ensure_unicode(word)
    infos = AnalyzeWord(word)
    if len(infos) == 0:
        return True
    else:
        info = infos[0]
        return (
            info.Bastardness == 0
            and 'obsc' not in info.LexicalFeature
        )
