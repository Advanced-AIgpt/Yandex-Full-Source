# Respect Rewriter Lib

Rewrites sentences to add more (or less) respect for boltalka.  
- `to_plural=True` adds respect, e.g., "ты уверен?" should be rewritten to "вы уверены?"
- `to_plural=False` reduces respect
    - with `to_gender=masc`, it rewrites sentences to masculine gender, e.g., "вы уверены?" should be rewritten to "ты уверен?"
    - with `to_gender=femn`, it rewrites sentences to feminine gender, e.g., "вы уверены?" should be rewritten to "ты уверена?"

Works on top of a syntactic parser (with optional classifier or tagger for rewritable positions prediction).

Check the [unit tests](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/py_libs/respect_rewriter/ut/test.py) to understand the supported cases.

Check also the [yt applier](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/tools/yt_respect_rewriter).
