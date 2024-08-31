# Begemot taggers/binary classifiers test

Collects scores from binary classifiers and tagger parses for models in Begemot.

Run:
```bash
../../granet/data/ru/test/pool/load_dataset.sh random6v3.tsv
ya m -r
./test_begemot_parsing -i ../../granet/data/ru/test/pool/random6v3.tsv -o result.tsv --intent <your intent> --begemot-url <begemot url, e.g., http://hamzard.yandex.net:8891/wizard>
```

The first command downloads a pool with requests to Alice.

The `result.tsv` file contains three columns:
- request frequency,
- request text with tagger's markup,
- classifier's confidence that the request is from this intent.

E.g.:
```
188	какие новости 'в мире'(topic)	0.9978826642
94	расскажи новости 'футбола'(topic)	0.9944429994
197	новости	0.9776880145
```

It's sorted by the last column.
