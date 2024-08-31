# Quality Test for RNN Taggers

Tests quality of some specific taggers + compares responses for the current tagger and the old one

- The quality report can be found at `canondata/test.test_quality`
- The responses are collected at `canondata/test.test_medium`.


## How to run
1. Run `ya m -Ar -Z` to recanonize the results.
2. Check the quality and responses diff.
3. Check yourself (and try to retrain the tagger) when the quality is significantly worse than the old one (e.g., the mean metric is lower by a percent or so). Check the responses diff to understand the probable causes of that issue.
4. Commit recanonized results when you are sure that everything is fine.


## How to update datasets

The test measures quality on the resources, specified in `ya.make`. Those datasets originate from the [tagger validation sets](https://a.yandex-team.ru/arc/trunk/arcadia/alice/vins/apps/personal_assistant/personal_assistant/tests/validation_sets/tagger_validation) but aren't equal to them.

- Download current datasets without mocks from `3368552973` and `3368553743` resources
```
ya download sbr://3368552973
ya download sbr://3368553743
```
- Update those datasets as you wish. The lines order is of no importance
- Upload them back and change the sandbox-resources 2 lines above (so that the next person is able to update the datasets)
```
ya upload --ttl=inf medium_data_without_mocks.tsv
ya upload --ttl=inf quality_data_without_mocks.tsv
```
- Build and run `mocks_downloader` utility
```
./mocks_downloader -i medium_data_without_mocks.tsv -o ~/medium_data.tsv
./mocks_downloader -i quality_data_without_mocks.tsv -o ~/quality_data.tsv
```
- Upload the datasets with mocks and update the ids in `ya.make`
```
ya upload --ttl=inf medium_data.tsv
ya upload --ttl=inf quality_data.tsv
```
- Run `ya m -Ar -Z` to recanonize the results and check if it's ok 
- If you have time, refactor the whole thing for the datasets with mocks to contain the nlu_extra column, so that we don't need to store 2 tables per dataset (with `mock` and with `nlu_extra`)
