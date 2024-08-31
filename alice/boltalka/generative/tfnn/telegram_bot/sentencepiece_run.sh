#!/usr/bin/env bash

python3.7 -mpip install cherrypy sentencepiece==0.1.91 -i https://pypi.yandex-team.ru/simple &&
python3.7 sentencepiece_tokenizer_server.py --model-path tokenizer.model
