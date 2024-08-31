from transliterate import get_translit_function


class _OpusConsts(object):
    language = 'ru'
    file_extension = 'opus'
    filename_template = '{uid}.{ext}'


opus_consts = _OpusConsts()
_translit_func = get_translit_function(opus_consts.language)


def translit(text):
    text = _translit_func(text.strip().lower(), reversed=True)
    return text.translate(str.maketrans(' ', '_', '.,:;*+!?<>\\/\'"'))


def generate_opus_filename(text):
    return opus_consts.filename_template.format(
        uid=translit(text), ext=opus_consts.file_extension
    )
