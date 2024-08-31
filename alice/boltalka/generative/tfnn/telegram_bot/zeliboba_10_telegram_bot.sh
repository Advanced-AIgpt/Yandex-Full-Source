#!/usr/bin/env bash

# staff-token - your OAUTH token
# telegram-token - ask khr2@

./telegram_bot \
    --telegram-token "*****" \
    --batch-size 1 \
    --model-name "v1bot" \
    --model "alice.boltalka.generative.tfnn.infer_lib.extensions.lm.model.ModelLM" \
    --tokenizer-settings '{"tokenizer_mode": "RemoteTokenizer", "tokenizer_url": "http://127.0.0.1:7293/"}' \
    --token-to-id-voc-path tokenizer.voc \
    --hp '{"res_dropout": 0.1, "vtype": "float16", "emb_out_device": "", "relu_dropout": 0.8, "attn_dropout": 0.8, "label_smoothing": 0.1, "normalize_out": true, "hid_size": 2048, "emb_size": 1024, "num_heads": 32, "ff_size": 8192, "rescale_emb": false, "num_layers": 18, "res_steps": "nlda", "emb_inp_device": "", "inp_emb_bias": false, "dwwt": true, "normalize_embedding": true, "train_pos_emb": true, "add_head": true, "loss_bias": true, "max_pos_emb": 2048, "max_input_len": -1800}' \
    --model-path zeliboba.model \
    --mode 'seq2seq' \
    --default-current-model-name "v1bot" \
    --staff-token "*****" \
    --default-context-len 1 \
    --enable-query-preprocess 0
