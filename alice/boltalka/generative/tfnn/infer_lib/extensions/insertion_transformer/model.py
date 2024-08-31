from collections import namedtuple

import tensorflow as tf
import tfnn

import tfnn.layers
import tfnn.layers.xent
import tfnn.ops
from tfnn.layers.dropout import Dropout
from tfnn.ops import infer_length
from ml_data_reader.parsers import BatchParser
from ml_data_reader.text_processing.parsers import SplitVocParser
from .data import SplitVocParserOutInsertion, SplitVocParserConditionInsertion
from .decoder import InsertionDecoder
from .loss.xent import LossXentInsertion
from .simple_sliced_argmax import sliced_argmax, sliced_sample
from .util import str_to_dtype, take_along_axis


class Model(tfnn.task.seq2seq.models.TranslateModelBase):
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

        # If True disable encoder and decoder training
        # Only variables inside self.loss will be marked as trainable
        self.train_enc_dec = hp.get('train_enc_dec', True)

        emb_size = hp.get('emb_size', hp.get('hid_size', 512))
        max_voc_size = max(inp_voc.size(), out_voc.size())

        self.line_sampling_mode = hp.get('line_sampling_mode', 'insertion')
        self.n_tokens = hp.get('n_tokens', 0)
        self.inslot_smoothing_mode = hp.get('inslot_smoothing_mode', 'uniform')
        self.inslot_smoothing_temperature = hp.get('inslot_smoothing_temperature', 1.0)

        with tf.variable_scope(self.name):
            # Embeddings
            self.emb_inp = tfnn.layers.TransformerEmbedding(
                'emb_inp', max_voc_size if share_emb else inp_voc.size(), emb_size,
                rescale=rescale_emb, bias=inp_emb_bias,
                dropout=emb_dropout,
                device=emb_inp_device,
                dtype=self.dtype,
            )

            self.emb_out = tfnn.layers.TransformerEmbedding(
                'emb_out', max_voc_size if share_emb else out_voc.size(), emb_size,
                rescale=rescale_emb, matrix=self.emb_inp.mat if share_emb else None,
                dropout=emb_dropout,
                device=emb_out_device,
                dtype=self.dtype,
            )

            # Model body
            self.encoder = tfnn.layers.TransformerChain('enc', **dict(hp, dtype=self.dtype))
            self.decoder = tfnn.layers.TransformerChain('dec', attn_inputs=['enc'], **dict(hp, dtype=self.dtype))

            # Logits and loss
            bias_mode = None if hp.get('loss_bias', False) else 0.0
            self.loss = LossXentInsertion(
                'loss_xent_it',
                hp['hid_size'],
                out_voc,
                hp,
                matrix=tf.transpose(self.emb_out.mat) if hp.get('dwwt', False) else None, bias=bias_mode,
                mixture_matrix=None, mixture_bias=bias_mode,
                mixture_weights_matrix=None, mixture_weights_bias=bias_mode,

                context_matrix=None, context_bias=bias_mode,
                location_matrix=None, location_bias=bias_mode,

                dtype=self.dtype
            )

        self.translate_model = TranslateModelInsertion(self.name, self, self.loss, self.inp_voc, self.out_voc)

    # Train interface
    def encode_decode(self, batch, is_train, score_info=False):
        """
        :return: [n_lines, (n_out + 2), hid_dim]
        """
        with tf.name_scope(self.name):
            inp, inp_len = self._extract_input_from_batch(batch)
            out_sublines, n_slots = self._extract_output_from_batch(batch)
            enc_out, enc_attn_mask = self._encode_impl(inp, inp_len, is_train)
            rdo = self._decode_impl(out_sublines, n_slots, enc_out, enc_attn_mask, is_train)
        if not self.train_enc_dec:
            rdo = tf.stop_gradient(rdo)
        return rdo

    def _extract_input_from_batch(self, batch):
        """
        :return: [n_lines, n_inp], [n_lines]
        """
        inp, inp_len = batch['inp'], batch['inp_len']
        return inp, inp_len

    def _extract_output_from_batch(self, batch):
        """
        :return: [n_lines, (n_out + 2)], [n_lines]
        """
        out_sublines, n_slots = batch['out_sublines'], batch['n_slots']
        return out_sublines, n_slots

    def _get_batch_sample(self, is_train=True):
        # TODO currently condition is needed, therefore is_train pair would not work. Need to fix this for training
        # if is_train:
        #     # [('input', 'output')]
        #     return [("i saw a cat", "i write the code")]
        # # [('input', 'output', 'conditioning')]
        return [("i saw a cat", "i write the code", "i the code")]

    def make_batch_parser(self, **kwargs):
        add_inp_words = kwargs.get('add_inp_words', False)
        input_parser, condition_parser = [
            parser_class(
                name,
                voc=voc,
                force_bos=self.hp.get('force_bos', False),
                merge_unks=self.hp.get('merge_unks', False),
                add_words=add_inp_words and name == 'inp'
            )
            for parser_class, name, voc in zip(
                (SplitVocParser, SplitVocParserConditionInsertion),
                ('inp', 'condition'),
                (self.inp_voc, self.out_voc)
            )
        ]
        output_parser = SplitVocParserOutInsertion(
            'out',
            voc=self.out_voc,
            line_sampling_mode=self.line_sampling_mode,
            n_tokens=self.n_tokens,
            inslot_smoothing_mode=self.inslot_smoothing_mode,
            temperature=self.inslot_smoothing_temperature
        )
        return BatchParser([input_parser, output_parser, condition_parser])

    def make_feed_dict(self, batch, is_train=True, **kwargs):
        return self.make_batch_parser(**kwargs)(batch, is_train=is_train)

    def _encode_impl(self, inp, inp_len, is_train):
        """
        :return: [n_lines, n_inp, hid_dim], [n_lines, 1, 1, n_inp]
        """
        with Dropout.enable_all(is_train):
            enc_inp = self.emb_inp(inp, dtype=self.vtype)
            enc_attn_mask = tfnn.ops.make_encoder_attn_mask(inp, inp_len, dtype=self.dtype)
            enc_out, _ = self.encoder(enc_inp, self_attn_mask=enc_attn_mask)
            return enc_out, enc_attn_mask

    def _decode_impl(self, out_sublines, n_slots, enc_out, enc_attn_mask, is_train):
        """
        :return: [n_lines, (n_out + 2), hid_dim]
        """
        with Dropout.enable_all(is_train):
            # shift_false == False because we do not predict next token given prefix
            dec_inp = self.emb_out(
                out_sublines, shift_right=False, offset='random' if self.dst_rand_offset else 0, dtype=self.vtype
            )
            with tf.name_scope("decoder_mask"):
                # Allow information flow from each token to all another but mask padding EOS
                n_lines, n_slots_max = tf.shape(out_sublines)[0], tf.shape(out_sublines)[1]
                up_down_ids = tf.tile(tf.range(n_slots_max)[None, None, :, None], [n_lines, 1, 1, n_slots_max])
                left_right_ids = tf.tile(tf.range(n_slots_max)[None, None, None, :], [n_lines, 1, n_slots_max, 1])
                dec_attn_mask = tf.logical_and(
                    tf.less_equal(up_down_ids, n_slots[:, None, None, None]),
                    tf.less_equal(left_right_ids, n_slots[:, None, None, None]),
                )
                # ^--- [n_lines, 1, (n_out + 2), (n_out + 2)]
            dec_out, _ = self.decoder(
                dec_inp, self_attn_mask=dec_attn_mask, enc_out=enc_out, enc_attn_mask=enc_attn_mask
            )
            return dec_out

    # ======== TranslateModel for Inference ============
    def encode(self, batch, is_train=False, **flags):
        """
        :param batch: a dict of {string:symbolic tensor} that model understands.
            By default it should accept {'inp': int32 matrix[batch,time]}
        :param is_train:
        :return: initial decoder state
        """
        return self.translate_model.encode(batch, is_train, **flags)

    def decode(self, dec_state, outputs=None, n_slots=None, is_train=False, **flags):
        """
        Performs decoding step given words and previous state.
        :returns: next state
        """
        return self.translate_model.decode(dec_state, outputs, n_slots, is_train, **flags)

    def sample(
            self, dec_state, outputs=None, n_slots=None, eos_penalty=None, base_scores=None, slices=None,
            k=1, decoding_strategy='greedy', sampling_strategy='greedy', sampling_temperature=None, **kwargs
            # TODO kwargs can be removed?
    ):
        return self.translate_model.sample(
            dec_state, outputs, n_slots, eos_penalty, base_scores, slices,
            k, decoding_strategy, sampling_strategy, sampling_temperature, **kwargs
        )

    def get_rdo(self, dec_state, **kwargs):
        return self.translate_model.get_rdo(dec_state, **kwargs)

    def get_attnP(self, dec_state, **kwargs):
        return self.translate_model.get_attnP(dec_state, **kwargs)

    def symbolic_translate(self, batch_placeholder, mode='beam_search', **flags):
        return InsertionDecoder(
            model=self.get_translate_model(),
            batch_placeholder=batch_placeholder,
            **flags
        )

    def get_translate_model(self):
        if hasattr(self, 'translate_model'):
            return self.translate_model

        return self


class TranslateModelInsertion(tfnn.task.seq2seq.models.TranslateModel):
    DecState = namedtuple("transformer_state", ['enc_out', 'enc_attn_mask', 'rdo'])

    def __init__(self, name, model, loss, inp_voc, out_voc, **hp):
        """
        A translation model that performs inference in insertion transformer.
        Complexity of generating n tokens ~O(n^2 logn)
        """
        super().__init__(name, inp_voc, out_voc, loss, **hp)
        self.loss = loss  # type: tfnn.layers.xent.LossXentInsertion
        self.model = model  # type: Model

    def _encode_impl(self, batch, is_train, **kwargs):
        inp, inp_len = self.model._extract_input_from_batch(batch)

        enc_out, enc_attn_mask = self.model._encode_impl(inp, inp_len, is_train)

        new_state = self.DecState(enc_out, enc_attn_mask, rdo=tf.zeros([0, 0, 0], dtype=enc_out.dtype))

        return new_state

    def encode(self, batch, is_train=False, **kwargs):
        """
        :param batch: a dict containing 'inp': int32[n_lines * n_inp] and optionally inp_len: int32[n_lines]
        :param is_train: if True, enables dropouts
        """
        with Dropout.enable_all(is_train), tf.name_scope(self.model.name):
            new_state = self._encode_impl(batch, is_train, **kwargs)
            return new_state

    def _decode_impl(self, dec_state, outputs, n_slots, is_train, **kwargs):
        """
        Performs decoding step given words and previous state.
        Returns next state.
        """
        enc_out, enc_attn_mask, _ = dec_state
        new_rdo = self.model._decode_impl(outputs, n_slots, enc_out, enc_attn_mask, is_train)

        return self.DecState(enc_out, enc_attn_mask, new_rdo)

    def decode(self, dec_state, outputs=None, n_slots=None, is_train=False, **kwargs):
        with Dropout.enable_all(is_train), tf.name_scope(self.model.name):
            new_state = self._decode_impl(dec_state, outputs, n_slots, is_train, **kwargs)
            return new_state

    def get_rdo(self, dec_state, **kwargs):
        return dec_state.rdo

    def get_attnP(self, dec_state, **kwargs):
        attn_p = dec_state.enc_attn_mask[:, 0, 0, :]  # [n_lines, n_inp]
        attn_p /= tf.reduce_sum(attn_p, axis=1, keepdims=True)
        return attn_p

    def sample(
            self, dec_state, outputs=None, n_slots=None, eos_penalty=None, base_scores=None, slices=None,
            k=1, top_k=None, decoding_strategy='greedy', sampling_strategy='greedy', sampling_temperature=None,
            sampling_with_replacement=True, **kwargs  # TODO kwargs can be removed?
    ):
        """
        Samples top-K new words for each hypothesis from a beam.
        Decoder states and base_scores of hypotheses for different inputs are concatenated like this:
            [x0_hypo0, x0_hypo1, ..., x0_hypoN, x1_hypo0, ..., x1_hypoN, ..., xM_hypoN

        :param dec_state: nested structure of tensors, batch-major
        :param outputs:
        :param n_slots:
        :param eos_penalty:
        :param base_scores: [n_lines], log-probabilities of hypotheses in dec_state with additive penalties applied
        :param slices: start indices of each input
        :param k: [], int, how many hypotheses to sample per input
        :param top_k: int
        :param decoding_strategy:
        :param sampling_strategy:
        :param sampling_temperature:
        :param sampling_with_replacement:
        :returns: finished, best_hypos, best_insertion, best_delta_scores,
            finished: True if line should be finished and final output returned, [n_lines], bool
            best_hypos: in-beam hypothesis index for each sampled token, [n_slices, k], int
            best_insertion: new insertions for each hypo, [n_slices, k, n_insertions_max, 2], int
            best_delta_scores: log P(insertions | best_hypos), [n_slices, k], float32
        """
        rdo = self.get_rdo(dec_state)
        logits = self.loss.rdo_to_logits__predict(rdo)
        log_probas = tf.cast(self.loss.logits_to_log_probas(logits, n_slots), dtype=tf.float32)

        n_lines, n_slots_max, n_tokens, n_slices = (
            tf.shape(log_probas)[0], tf.shape(log_probas)[1], tf.shape(log_probas)[2], tf.shape(slices)[0]
        )

        # Apply EOS penalty
        if self.loss.termination_mode == 'sequence_finalization':
            fin_token = self.loss.eos
        elif self.loss.termination_mode == 'slot_finalization':
            fin_token = self.loss.eoslot
        else:
            raise ValueError("Termination mode must be either 'sequence_finalization' or 'slot_finalization'")
        log_probas = self.loss.apply_log_probas_token_penalty(log_probas, fin_token, eos_penalty)

        flatten_log_probas = tf.reshape(log_probas, [n_lines, -1]) + base_scores[:, None]

        # Mask for forbidden (virtual) slots
        slots_mask = tf.sequence_mask(n_slots, n_slots_max)

        # Determine hypos that have already finished
        slots_predictions = tf.argmax(log_probas, axis=-1)

        if self.loss.termination_mode == 'sequence_finalization':
            # Note: outside the mask we thread predictions as non EOS token
            finished = tf.reduce_any(
                tf.logical_and(slots_mask, tf.equal(slots_predictions, self.loss.eos)), axis=-1
            )
        elif self.loss.termination_mode == 'slot_finalization':
            # Note: outside the mask we thread predictions as EOSlot token
            def logical_implication(a, b):
                return tf.logical_or(tf.logical_not(a), b)

            finished = tf.reduce_all(
                logical_implication(slots_mask, tf.equal(slots_predictions, self.loss.eoslot)), axis=-1
            )
            # Restrict locations whose maximum-probability decision is EOSlot
            slots_mask = tf.logical_and(slots_mask, tf.not_equal(slots_predictions, self.loss.eoslot))
        else:
            raise ValueError("Termination mode must be either 'sequence_finalization' or 'slot_finalization'")

        # Mask finished sequences
        lines_mask = tf.tile(~finished[:, None], [1, n_slots_max])
        # Mask EOS, EOSlot tokens to avoid generation of virtual tokens
        # Do not affect if 'greedy' sampling mode but 'sample' do
        token_mask = tf.logical_and(
            tf.not_equal(tf.range(n_tokens), self.loss.eos),
            tf.not_equal(tf.range(n_tokens), self.loss.eoslot)
        )
        token_mask = tf.tile(token_mask[None, None, :], [n_lines, n_slots_max, 1]),
        # Mask that combines all forbidden slot-token insertions
        mask = tf.tile(tf.logical_and(slots_mask, lines_mask)[:, :, None], [1, 1, n_tokens])
        mask = tf.logical_and(mask, token_mask)
        # tf.shape(mask) == [n_lines, n_slots_max, n_tokens]

        # Get/Sample k the most probable updates over all non masked token-slots for each line
        # If there is less than k such updates arrays are padded with -inf/-1
        if decoding_strategy == 'greedy':
            # tf.shape(flatten_mask) == [n_lines, n_slots_max * n_tokens]
            flatten_mask = tf.reshape(mask, [n_lines, -1])

            # tf.shape(best_scores) == tf.shape(best_indices) == [n_slices, k]
            if sampling_strategy == 'greedy':
                best_scores, best_indices = sliced_argmax(
                    flatten_log_probas, slices, k, flatten_mask
                )
            elif sampling_strategy == 'sample':
                sampling_temperature = sampling_temperature if sampling_temperature else 1.0
                flatten_log_probas /= sampling_temperature
                best_scores, best_indices = sliced_sample(
                    flatten_log_probas, slices, k, flatten_mask,
                    top_k=top_k, replacement=sampling_with_replacement
                )
            else:
                raise ValueError("Sampling strategy must be either 'greedy' or 'sample'")
            # Ids of hypos that might be updated further
            # tf.shape(best_hypos) == [n_slices, k]
            best_hypos = tf.where(
                tf.not_equal(best_indices, -1),
                tf.floordiv(best_indices, n_slots_max * n_tokens) + slices[:, None],
                tf.fill(tf.shape(best_indices), -1)
            )

            # Raveled ids of best insertions for each of hypos
            # Note: tf.unravel_index(-(n_tokens + 1), [n_slots_max, n_tokens]) == [-1, -1]
            # tf.shape(best_insertions) == [n_slices, k]
            best_insertions = tf.where(
                tf.not_equal(best_indices, -1),
                tf.mod(best_indices, n_slots_max * n_tokens),
                tf.fill(tf.shape(best_indices), -(n_tokens + 1))
            )
            flatten_best_insertions = tf.reshape(best_insertions, [-1])
            flatten_unravel_best_insertions = tf.unravel_index(flatten_best_insertions, [n_slots_max, n_tokens])
            unravel_best_insertion = tf.reshape(tf.transpose(flatten_unravel_best_insertions), [n_slices, k, 1, 2])

            best_delta_scores = best_scores - tf.gather(base_scores, tf.maximum(0, best_hypos))
        elif decoding_strategy == 'parallel':
            if k > 1:
                # TODO: Implement multiple predictions (for beam search)
                raise NotImplementedError("Multiple predictions per hypothesis in parallel mode is not supported yet")
            # This implementation suggests that k == 1 and slices == tf.range(tf.shape(outputs)[0])
            # I.e. one hypo per slice and one prediction per slice
            with tf.control_dependencies([tf.assert_equal(slices, tf.range(tf.shape(outputs)[0]))]):
                n_slots_total = n_lines * n_slots_max
            # tf.shape(flatten_log_probas) == tf.shape(flatten_mask) == [n_lines * n_slots_max, n_tokens]
            flatten_log_probas = tf.reshape(flatten_log_probas, [n_slots_total, -1])
            flatten_mask = tf.reshape(mask, [n_slots_total, -1])

            # tf.shape(best_scores) == tf.shape(best_indices) == [n_lines * n_slots_max, 1]
            # Note: unlike 'greedy' decoding_strategy we select token ids for each slot right away
            if sampling_strategy == 'greedy':
                best_scores, best_tokens = sliced_argmax(
                    flatten_log_probas, tf.range(n_slots_total), 1, flatten_mask
                )
            elif sampling_strategy == 'sample':
                sampling_temperature = sampling_temperature if sampling_temperature else 1.0
                flatten_log_probas /= sampling_temperature
                best_scores, best_tokens = sliced_sample(
                    flatten_log_probas, tf.range(n_slots_total), 1, flatten_mask,
                    top_k=top_k, replacement=sampling_with_replacement
                )
            else:
                raise ValueError("Sampling strategy must be either 'greedy' or 'sample'")

            best_scores = tf.reshape(best_scores, [n_lines, n_slots_max])
            best_tokens = tf.reshape(best_tokens, [n_lines, n_slots_max])

            # Move all insertions corresponding masked slots to the end of array to satisfy make_insertion requirements
            slot_ids_base = tf.tile(tf.range(n_slots_max)[None, :], [n_lines, 1])
            slot_ids_base = tf.where(
                tf.equal(best_tokens, -1), tf.fill(tf.shape(slot_ids_base), n_slots_max), slot_ids_base
            )

            # Rearrange scores and tokens according new ids
            # TODO: rewrite with stable tf API (tf.__version__ >= 1.13)
            best_ids = tf.contrib.framework.argsort(slot_ids_base)
            best_scores = take_along_axis(best_scores, best_ids)
            best_tokens = take_along_axis(best_tokens, best_ids)

            # Mask scores and ids corresponding masked slots
            masked_best_scores = tf.where(tf.equal(best_tokens, -1), tf.fill(tf.shape(best_scores), 0.0), best_scores)
            masked_best_ids = tf.where(tf.equal(best_tokens, -1), tf.fill(tf.shape(best_ids), -1), best_ids)

            # Combine all insertions into one array for update
            unravel_best_insertion = tf.concat(
                [masked_best_ids[:, None, :, None], best_tokens[:, None, :, None]], axis=-1
            )
            best_delta_scores = tf.reduce_sum(masked_best_scores, axis=-1)
            best_hypos = tf.range(tf.shape(outputs)[0])
        else:
            raise ValueError("Decoding strategy must be either 'greedy' or 'parallel'")

        return finished, best_hypos, unravel_best_insertion, best_delta_scores

    def make_insertions(self, outputs, n_slots, updates, reshape_outputs=True):
        """
        Padding insertions must be in format [-1, -1].
        Insertions in each line must be sorted by slot number
        Example:
            outputs = tf.constant([
                [0,  1,  2,  3,  4,  5,  6, EOS, EOS, EOS],\n
                [0,  6,  7,  8,  9,  7,  3,   6, EOS, EOS],\n
                [0, 11, 12, 13, 14, 15,  2,   3,   5, EOS]\n
            ])\n
            #      0   1   2   3   4   5   6   7   8   <-- slot numbers\n
            n_slots = tf.constant([7, 8, 9])\n
            updates = tf.constant([
                [[2, 5], [4, 3], [6, 1], [-1, -1], [-1, -1]],\n
                [[0, 5], [1, 6], [4, 1], [5,   4], [-1, -1]],\n
                [[3, 1], [4, 7], [5, 2], [-1, -1], [-1, -1]],\n
            ])\n
            result (reshape_outputs == False):\n
                [[ 0  1  2  5  3  4  3  5  6  1 EOS EOS EOS EOS EOS]
                 [ 0  5  6  6  7  8  9  1  7  4   3   6 EOS EOS EOS]\n
                 [ 0 11 12 13  1 14  7 15  2  2   3   5 EOS EOS EOS]]\n
                [10, 12, 12]\n
            result (reshape_outputs == True):\n
                [[ 0  1  2  5  3  4  3  5  6  1 EOS EOS EOS]
                 [ 0  5  6  6  7  8  9  1  7  4   3   6 EOS]\n
                 [ 0 11 12 13  1 14  7 15  2  2   3   5 EOS]]\n
                [10, 12, 12]\n
        :param outputs: [n_lines, (n_out + 2))
        :param n_slots: [n_lines]
        :param updates: [n_lines, n_inserts, 2]
        :param reshape_outputs: drop unnecessary EOS tokens if True
        :returns final_outputs, new_n_slots
        """

        n_lines, n_inserts_max, out_len_max = (
            tf.shape(updates)[0],
            tf.shape(updates)[1],
            tf.shape(outputs)[1]
        )

        updates_ids, updates_values = updates[:, :, :1], updates[:, :, 1:]
        flatten_updates_ids = tf.reshape(updates_ids, [-1])
        flatten_updates_values = tf.reshape(updates_values, [n_lines, -1])
        # Change values corresponding padding in updates matrix onto EOS tokens
        flatten_updates_values = tf.where(
            tf.equal(flatten_updates_values, -1),
            tf.fill(tf.shape(flatten_updates_values), self.out_voc.EOS), flatten_updates_values
        )
        outputs_expanded = tf.concat([outputs, flatten_updates_values], axis=-1)

        insertion_ids = tf.concat(
            [tf.tile(tf.range(n_lines)[:, None, None], [1, n_inserts_max, 1]), updates_ids], axis=-1
        )
        flatten_insertion_ids = tf.reshape(tf.cast(insertion_ids, dtype=tf.int64), [-1, 2])
        flatten_insertion_ids = tf.boolean_mask(flatten_insertion_ids, tf.not_equal(flatten_updates_ids, -1), axis=0)

        biases_blank = tf.sparse_tensor_to_dense(
            tf.SparseTensor(
                flatten_insertion_ids, tf.ones([tf.shape(flatten_insertion_ids)[0]], dtype=tf.int32),
                dense_shape=tf.shape(outputs, out_type=tf.int64),
            ), validate_indices=False
        )
        biases = tf.cumsum(biases_blank, exclusive=True, axis=-1) + tf.range(out_len_max)[None, :]

        updates_len = tf.reduce_sum(
            tf.cast(tf.not_equal(flatten_updates_values, self.out_voc.EOS), dtype=tf.int32), axis=-1
        )
        mask = tf.sequence_mask(updates_len, n_inserts_max)
        new_values_ids = tf.where(
            mask, updates_ids[:, :, 0] + tf.range(n_inserts_max)[None, :] + 1,
            tf.tile(tf.range(start=out_len_max, limit=out_len_max + n_inserts_max)[None, :], [n_lines, 1])
        )

        final_ids = tf.concat([biases, new_values_ids], axis=-1)
        # TODO: rewrite with stable tf API (tf.__version__ >= 1.13)
        final_ids = tf.contrib.framework.argsort(final_ids, axis=-1)
        final_outputs = take_along_axis(outputs_expanded, final_ids)

        if reshape_outputs:
            new_out_len_max = tf.reduce_max(infer_length(final_outputs, self.out_voc.EOS))
            final_outputs = final_outputs[:, :new_out_len_max]

        return final_outputs, n_slots + updates_len
