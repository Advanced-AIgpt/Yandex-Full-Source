# Syntax Parser

This is a syntax parser implementation based on paper [Exploring pretrained models for joint morpho-syntactic parsing](http://www.dialog-21.ru/media/5069/anastasyevdg-147.pdf).

Currently, it uses LSTM-based encoder with ELMo-based word embeddings, though a BERT-based is also available (check the arc history to find out how it was used before).

To download model, run:
```bash
ya m -r model
```

Use it this way:
```python
from alice.nlu.py_libs.syntax_parser.parser import Parser

parser = Parser.load('model')
# or use Parser.load_from_archive('model.tar')

parse = parser.parse('мама мыла раму', predict_syntax=True, return_embeddings=False)
assert parse == {
    'tokens': ['мама', 'мыла', 'раму'],
    'lemmas': ['мама', 'мыть', 'рама'],
    'heads': [2, 0, 2],
    'head_tags': ['nsubj', 'root', 'obj'],
    'grammar_values': [
        'NOUN|Animacy=Anim|Case=Nom|Gender=Fem|Number=Sing',
        'VERB|Aspect=Imp|Gender=Fem|Mood=Ind|Number=Sing|Tense=Past|VerbForm=Fin|Voice=Act',
        'NOUN|Animacy=Inan|Case=Acc|Gender=Fem|Number=Sing'
    ]
}
```

Use `return_embeddings = True` to collect contextualized word-level embeddings used to predict morpho-syntactic values (as it is used in [Annotated Span Normalization as a Sequence Labelling Task](http://www.dialog-21.ru/media/5234/anastasyevdg147.pdf)).
