#!/usr/bin/env bash

python infer_model.py \
    --model "tfnn.task.seq2seq.models.transformer.Model" \
    --ivoc input_token_to_id.voc \
    --ovoc output_token_to_id.voc \
    --model-path path/to/your/model.npz \
    --hp '{"attn_beta": 0, "attn_dropout": 0.1, "beam_size": 12, "beam_spread": 3, "emb_inp_device": "", "emb_out_device": "", "emb_size": 512, "ff_size": 2048, "hid_size": 512, "inp_emb_bias": false, "label_smoothing": 0.1, "len_alpha": 1.0, "normalize_out": true, "num_heads": 8, "num_layers": 6, "relu_dropout": 0.1, "replace": true, "res_dropout": 0.1, "res_steps": "nlda", "rescale_emb": true, "summarize_preactivations": false}'
