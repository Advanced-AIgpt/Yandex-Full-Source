from collections import namedtuple

import copy
import tensorflow as tf

import tfnn
import tfnn.ops
import tfnn.layers
import tfnn.layers.xent
from tfnn.layers.dense import Dense
from tfnn.layers.dropout import Dropout
from tfnn.layers.norm import LayerNorm

# ============================================================================
#                                  Transformer model
from tfnn.util import str_to_dtype


class ModelLM(tfnn.task.seq2seq.models.TranslateModelBase):

    def __init__(
        self, name, inp_voc, out_voc,
        share_emb=False, inp_emb_bias=False, rescale_emb=False,
        emb_inp_device='', emb_out_device='', emb_dropout=0,
        dst_rand_offset=False,
        **hp
    ):
        self.name = name
        self.inp_voc = inp_voc
        self.out_voc = out_voc
        self.dst_rand_offset = dst_rand_offset
        self.hp = hp

        self.dtype = str_to_dtype(hp.get('dtype', 'float32'))
        self.vtype = str_to_dtype(hp.get('vtype', hp.get('dtype', 'float32')))
        self.add_head = hp.get('add_head', False)

        hid_size = hp.get('hid_size', 512)
        emb_size = hp.get('emb_size', hid_size)
        max_voc_size = out_voc.size()

        with tf.variable_scope(self.name):
            assert share_emb is False

            self.emb_out = tfnn.layers.TransformerEmbedding(
                'emb_out', max_voc_size if share_emb else out_voc.size(), emb_size,
                rescale=rescale_emb,
                # matrix=self.emb_inp.mat if share_emb else None,
                matrix=None,
                dropout=emb_dropout,
                device=emb_out_device,
                dtype=self.dtype,
                timing_signal_trainable=hp.get('train_pos_emb', False),
                timing_signal_max_pos=hp.get('max_pos_emb', 1024),
                normalize_out=hp.get('normalize_embedding', False),
                out_size=hid_size,
            )

            # Model body
            hp_for_decoder = copy.deepcopy(hp)
            hp_for_decoder['hid_size'] = hid_size
            hp_for_decoder['emb_size'] = hid_size
            self.decoder = tfnn.layers.TransformerChain('dec',
                                                        **dict(hp_for_decoder, dtype=self.dtype))

            if self.add_head:
                activ = tfnn.ops.get_activation_function(hp.get('head_activation', 'relu'))
                self.rdo_transform = Dense(
                    'head_dense',
                    hid_size,
                    emb_size,
                    activ=activ,
                    dtype=self.dtype
                )
                self.rdo_norm = LayerNorm(
                    'head_layer_norm',
                    emb_size,
                    dtype=self.dtype
                )

            # Logits and loss
            self.loss = tfnn.layers.xent.LossXent(
                'loss_xent_lm',
                emb_size,
                out_voc.size(),
                hp,
                matrix=tf.transpose(self.emb_out.mat) if hp.get('dwwt', False) else None,
                bias=None if hp.get("loss_bias", False) else 0,
                dtype=self.dtype,
            )

        self.translate_model = TranslateModelFastLM(self.name, self, self.loss, self.inp_voc, self.out_voc)

    # Train interface
    def encode_decode(self, batch, is_train, score_info=False):
        with tf.name_scope(self.name):
            out, out_len = self._extract_output_from_batch(batch)

            # rdo: [batch_size * nout * emb_dim]
            rdo = self._decode_impl(out, out_len, is_train)
            if self.add_head:
                rdo = self.rdo_transform(rdo)
                rdo = self.rdo_norm(rdo)
        return rdo

    def _extract_input_from_batch(self, batch):
        inp = batch['inp']  # [batch_size * ninp]
        inp_len = batch.get('inp_len', tfnn.ops.infer_length(inp, self.inp_voc.EOS, time_major=False))  # [batch]
        return inp, inp_len

    def _extract_output_from_batch(self, batch):
        out = batch['out']  # [batch_size * nout]
        out_len = batch.get('out_len', tfnn.ops.infer_length(out, self.out_voc.EOS, time_major=False))  # [batch]
        return out, out_len

    def _get_batch_sample(self, is_train):
        return [("i saw a cat", "i write the code")]

    def _decode_impl(self, out, out_len,
                     is_train):
        with Dropout.enable_all(is_train):
            dec_inp = self.emb_out(out, shift_right=True, offset='random' if self.dst_rand_offset else 0, dtype=self.vtype)
            dec_attn_mask = tfnn.ops.make_decoder_attn_mask(out, dtype=self.dtype)  # [1 * 1 * nout * nout]
            dec_out, _ = self.decoder(dec_inp, self_attn_mask=dec_attn_mask)
            return dec_out

    # ======== TranslateModel for Inference ============
    def encode(self, batch, **flags):
        """
        :param batch: a dict of {string:symbolic tensor} that model understands.
            By default it should accept {'inp': int32 matrix[batch,time]}
        :return: initial decoder state
        """
        return self.translate_model.encode(batch, **flags)

    def decode(self, dec_state, words=None, **flags):
        """
        Performs decoding step given words and previous state.
        :param words: previous output tokens, int32[batch_size]. if None, uses zero embeddings (first step)
        :returns: next state
        """
        return self.translate_model.decode(dec_state, words, **flags)

    def sample(self, dec_state, base_scores, slices, k, **kwargs):
        return self.translate_model.sample(dec_state, base_scores, slices, k, **kwargs)

    def get_rdo(self, dec_state, **kwargs):
        return self.translate_model.get_rdo(dec_state, **kwargs)

    def get_attnP(self, dec_state, **kwargs):
        return self.translate_model.get_attnP(dec_state, **kwargs)


class TranslateModelFastLM(tfnn.task.seq2seq.models.TranslateModel):

    DecState = namedtuple("transformer_state", ['attnP', 'rdo', 'out_seq', 'offset', 'emb',
                                                'dec_layers', 'dec_dec_kv'])

    def __init__(self, name, model, loss, inp_voc, out_voc):
        """
        A translation model that performs quick (n^2) inference for transformer
        with manual implementation of 1-step decoding
        """
        self.name = name
        self.model = model
        self.loss = loss
        self.inp_voc = inp_voc
        self.out_voc = out_voc

    def _encode_impl(self, batch, **kwargs):
        out = batch['inp']

        batch_size = tf.shape(out)[0]
        nout = tf.shape(out)[1]
        offset = tf.zeros((batch_size, ), dtype=tf.int32) + nout

        dec_inp_t = self.model.emb_out(out, shift_right=kwargs.get('shift_right', True), offset='random' if self.model.dst_rand_offset else 0, dtype=self.model.vtype)
        dec_attn_mask = tfnn.ops.make_decoder_attn_mask(out, dtype=self.model.dtype)  # [1 * 1 * nout * nout]

        new_dec_dec_kv = []
        new_dec_layers = []
        aux = {}
        decoder = self.model.decoder
        for layer in range(decoder.num_layers):
            # multi-head self-attention: use only the newest time-step as query,
            # but all time-steps up to newest one as keys/values
            self_attn_layer = decoder.self_attn_layers[layer]
            next_dec_kv = self_attn_layer.kv_conv(self_attn_layer.preprocess(dec_inp_t))
            new_dec_dec_kv.append(next_dec_kv)
            dec_inp_t, aux[self_attn_layer.name] = self_attn_layer(dec_inp_t, dec_attn_mask, kv=new_dec_dec_kv[layer])

            dec_inp_t = decoder.ffn_layers[layer](dec_inp_t)

            new_dec_inp = dec_inp_t
            new_dec_layers.append(new_dec_inp)

        if decoder.out_norm is not None:
            dec_inp_t = decoder.out_norm(dec_inp_t)

        rdo = dec_inp_t[:, -1]
        if self.model.add_head:
            rdo = self.model.rdo_transform(rdo)
            rdo = self.model.rdo_norm(rdo)

        attnP = tf.ones([batch_size, nout], dtype=self.model.vtype) / tf.cast(nout, dtype=self.model.vtype)

        new_state = self.DecState(attnP, rdo, out, offset, dec_inp_t,
                                  new_dec_layers, new_dec_dec_kv)
        return new_state

    def encode(self, batch, is_train=False, **kwargs):
        """
        :param batch: a dict containing 'inp':int32[batch_size * ninp] and optionally inp_len:int32[batch_size]
        :param is_train: if True, enables dropouts
        """
        # perform initial decode (instead of force_bos) with zero embeddings
        with Dropout.enable_all(is_train), tf.name_scope(self.model.name):
            new_state = self._encode_impl(batch, **kwargs)
            new_state = self.decode(new_state, is_train=is_train)
            return new_state

    def _decode_impl(self, dec_state, words, **kwargs):
        """
        Performs decoding step given words and previous state.
        Returns next state.

        :param words: previous output tokens, int32[batch_size].
        """
        decoder = self.model.decoder
        attnP, rdo, out_seq, offset, prev_emb = dec_state[:5]
        prev_dec_layers = dec_state.dec_layers
        dec_dec_kv = dec_state.dec_dec_kv

        assert words is not None
        out_seq = tf.concat([out_seq, tf.expand_dims(words, 1)], 1)

        dec_inp_t = self.model.emb_out(words[:, None], offset=offset, dtype=self.model.vtype)  # [batch_size * 1 * emb_dim]

        # Apply dropouts
        if tfnn.ops.is_dropout_enabled():
            dec_inp_t = tf.nn.dropout(dec_inp_t, 1.0 - decoder.res_dropout)

        # Decoder
        new_emb = tf.concat([prev_emb, dec_inp_t], axis=1)
        dec_attn_mask = tfnn.ops.make_decoder_attn_mask(out_seq, dtype=self.model.vtype)[:, :, -1:, :]  # [1, 1, n_q=1, n_kv]

        new_dec_layers = []
        new_dec_dec_kv = []
        aux = {}

        for layer in range(decoder.num_layers):
            # multi-head self-attention: use only the newest time-step as query,
            # but all time-steps up to newest one as keys/values
            self_attn_layer = decoder.self_attn_layers[layer]
            next_dec_kv = self_attn_layer.kv_conv(self_attn_layer.preprocess(dec_inp_t))
            new_dec_dec_kv.append(tf.concat([dec_dec_kv[layer], next_dec_kv], axis=1))
            dec_inp_t, aux[self_attn_layer.name] = self_attn_layer(dec_inp_t, dec_attn_mask, kv=new_dec_dec_kv[layer])

            dec_inp_t = decoder.ffn_layers[layer](dec_inp_t)

            new_dec_inp = tf.concat([prev_dec_layers[layer], dec_inp_t], axis=1)
            new_dec_layers.append(new_dec_inp)

        if decoder.out_norm is not None:
            dec_inp_t = decoder.out_norm(dec_inp_t)

        rdo = dec_inp_t[:, -1]
        if self.model.add_head:
            rdo = self.model.rdo_transform(rdo)
            rdo = self.model.rdo_norm(rdo)

        new_state = self.DecState(attnP, rdo, out_seq, offset + 1, new_emb,
                                  new_dec_layers, new_dec_dec_kv)
        return new_state, aux

    def decode(self, dec_state, words=None, is_train=False, **kwargs):
        if words is None:
            return dec_state

        with Dropout.enable_all(is_train), tf.name_scope(self.model.name):
            new_state, aux = self._decode_impl(dec_state, words=words, **kwargs)
        return new_state

    def get_rdo(self, dec_state, **kwargs):
        return dec_state.rdo, dec_state.out_seq

    def get_attnP(self, dec_state, **kwargs):
        return dec_state.attnP
