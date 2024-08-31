#!/usr/bin/env bash

cat model_input.txt | ./infer \
    --problem "tfnn.task.cls.problems.ClassifierProblem" \
    --problem-opts '{"loss_type": "ce"}' \
    --voc token_to_id.voc \
    --model 'tfnn.task.cls.models.FullTransformerClassifier' \
    --model-name 'mod' \
    --model-path model.npz \
    --model-opts '{"num_classes": 2, "transformer_hp": {"res_dropout": 0.1, "vtype": "float16", "emb_out_device": "", "relu_dropout": 0.1, "attn_dropout": 0.1, "label_smoothing": 0.1, "normalize_out": true, "hid_size": 1280, "emb_size": 1280, "num_heads": 20, "ff_size": 5120, "rescale_emb": true, "num_layers": 24, "res_steps": "nlda", "emb_inp_device": "", "inp_emb_bias": false, "share_emb": true, "dwwt": true}}' \
    --batch-len 512 \
    --progress \
    --out-file output.json
