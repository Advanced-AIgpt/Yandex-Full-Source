# Mocks Downloader

Download mocks for the tagger's quality test.

Expects a tsv file with `markup` and `intent` or only `markup` columns.

The `intent` can also be passed with command line argument `--intent`.

Creates a tsv file with `markup`, (optionally) `intent` and `mock` columns that can be used to run the `canonize_applier` on.
