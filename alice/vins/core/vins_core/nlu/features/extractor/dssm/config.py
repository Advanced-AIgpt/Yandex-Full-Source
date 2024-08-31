from vins_core.nlu.features.extractor.dssm.preprocessor import TextPreprocessor
from vins_core.nlu.features.extractor.dssm.utils import line_converter_for_lm


class DSSMApplierConfig:
    MODEL_FILES = None

    wbigram_dct = None
    keep_word_indices = None
    word_dct_size = None
    trigram_dct_size = None
    wbigram_dct_size = None
    keep_marks = ""
    strip_punctuation = False

    def __init__(self, **kwargs):
        for k, v in kwargs.items():
            setattr(self, k, v)


class Context1RNNVinsdict(DSSMApplierConfig):
    MODEL_FILES = [
        'model.mmap',
        'model_description',
        'encoders_list.pkl',
        'twitter.vins.104k.dict',
        'twitter.trigrams.30k.dict'
    ]
    preprocessor = TextPreprocessor
    encode_inner_context = False
    run_name = "no-split/context1rnnvinsdict"
    word_dct = 'twitter.vins.104k.dict'
    trigram_dct = 'twitter.trigrams.30k.dict'
    line_converter = staticmethod(line_converter_for_lm)
