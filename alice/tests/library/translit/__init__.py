from transliterate import get_translit_function


_language = 'ru'
_translit_func = get_translit_function(_language)


def translit(text):
    text = _translit_func(text.strip(), reversed=True)
    return text.translate(str.maketrans(' ', '_', '\\/:*?\'"<>|+'))
