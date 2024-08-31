#!/bin/bash
cat twitter_my.tsv | sed -re 's/&gt;/>/g; s/&lt;/</g; s/Ё|ё/е/g' > twitter_my.tsv.ltgt
cat twitter_my.tsv.ltgt | ~/arcadia/alice/boltalka/scripts/apply_translate.py --dict-file misspellcorrections.txt --bigrams --lower --separate-punctuation > twitter_my.tsv.corrected
