#!/usr/bin/env bash

printf "меня зовут варя [SPECIAL_SEPARATOR_TOKEN] как тебя зовут?\tменя зовут" | ./infer \
    --model "alice.boltalka.generative.tfnn.infer_lib.extensions.insertion_transformer.model.Model" \
    --ivoc model_it/token_to_id.voc \
    --ovoc model_it/token_to_id.voc \
    --full-bpe-voc model_it/bpe.voc \
    --model-path model_it/model.npz \
    --hp '{"attn_beta": 0, "attn_dropout": 0.1, "beam_size": 12, "beam_spread": 3, "emb_inp_device": "", "emb_out_device": "", "emb_size": 512, "ff_size": 2048, "hid_size": 512, "inp_emb_bias": false, "label_smoothing": 0.1, "len_alpha": 1.0, "normalize_out": true, "num_heads": 8, "num_layers": 6, "relu_dropout": 0.1, "replace": true, "res_dropout": 0.1, "res_steps": "nlda", "rescale_emb": true, "summarize_preactivations": false, "use_context": false}' \
    --infer-model-kwargs '{"eos_penalty": 2.0}'
