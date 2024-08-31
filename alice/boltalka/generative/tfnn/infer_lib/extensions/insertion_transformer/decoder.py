from collections import namedtuple

import tensorflow as tf

from tfnn.util import nested_map


class InsertionDecoder:
    Stack = namedtuple('Stack',
                       ['outputs', 'n_slots', 'scores', 'finished', 'dec_state', 'attnP'])

    def __init__(
            self, model, batch_placeholder, eos_penalty=None, max_len=None, back_prop=True, swap_memory=False,
            top_k=None, decoding_strategy='greedy', sampling_strategy='greedy', sampling_temperature=None,
            sampling_with_replacement=True, **flags
    ):
        """
        :param model:
        :param batch_placeholder:
        :param eos_penalty:
        :param max_len:
        :param back_prop:
        :param swap_memory:
        :param decoding_strategy: {'greedy', 'parallel'}
        :param sampling_strategy: {'greedy', 'sample'}
        :param sampling_temperature:
        :param flags:
        """
        self.top_k = top_k
        self.eos_penalty = eos_penalty
        self.batch_placeholder = batch_placeholder
        self.decoding_strategy = decoding_strategy
        self.sampling_strategy = sampling_strategy
        self.sampling_temperature = sampling_temperature
        self.sampling_with_replacement = sampling_with_replacement

        inp_len = batch_placeholder['inp_len']
        max_len = max_len if max_len is not None else (2 * inp_len + 3)

        first_stack = self.create_initial_stack(model, batch_placeholder, **flags)
        shape_invariants = nested_map(lambda v: tf.TensorShape([None for _ in v.shape]), first_stack)

        def should_continue_translating(*stack):
            # If all hypothesis have finished then stop decoding
            stack = self.Stack(*stack)
            out_len = stack.n_slots - 1
            return tf.reduce_all(tf.less(out_len, max_len)) & tf.reduce_any(~stack.finished)

        def inference_step(*stack):
            stack = self.Stack(*stack)
            return self.greedy_step(model, stack, **flags)

        final_stack = tf.while_loop(
            cond=should_continue_translating,
            body=inference_step,
            loop_vars=first_stack,
            shape_invariants=shape_invariants,
            swap_memory=swap_memory,
            back_prop=back_prop,
        )  # type: InsertionDecoder.Stack

        self.best_out = final_stack.outputs
        self.best_scores = final_stack.scores / tf.cast(final_stack.n_slots, dtype=tf.float32)
        self.attn_p = final_stack.attnP

    def translate_batch(self, batch_data, **optional_feed):
        """
        Translates NUMERIC batch of data
        :param batch_data: dict {'inp':np.array int32[batch,time]}
        :param optional_feed: additional values to be fed into graph.
                                e.g. if you used placeholder for max_len at __init__
        :return: best hypotheses' outputs[batch, out_len] and attnP[batch, out_len, inp_len]
        """
        feed_dict = {placeholder: batch_data[k] for k, placeholder in self.batch_placeholder.items()}
        for k, v in optional_feed.items():
            feed_dict[k] = v

        out_ids, attn_p, best_scores = tf.get_default_session().run(
            [self.best_out, self.attn_p, self.best_scores], feed_dict=feed_dict
        )

        return out_ids, attn_p, best_scores

    def create_initial_stack(self, model, batch_placeholder, **flags):
        inp = batch_placeholder['inp']
        condition = batch_placeholder['condition']
        condition_n_slots = batch_placeholder['condition_n_slots']

        n_lines, n_inp_max = tf.shape(inp)[0], tf.shape(inp)[1]

        pre_initial_state = model.encode(batch_placeholder, **flags)

        initial_outputs = tf.concat([
            tf.fill([n_lines, 1], tf.cast(model.out_voc.BOS, dtype=inp.dtype)),
            condition,
            tf.fill([n_lines, 1], tf.cast(model.out_voc.EOS, dtype=inp.dtype))
        ], axis=1)

        initial_n_slots = condition_n_slots

        initial_state = model.decode(pre_initial_state, initial_outputs, initial_n_slots, **flags)

        initial_scores = tf.zeros([n_lines], dtype='float32')
        initial_finished = tf.zeros_like([n_lines], dtype='bool')

        initial_attn_p = tf.concat(
            [
                tf.zeros([n_lines, tf.shape(initial_outputs)[1], n_inp_max]),
                model.get_attnP(initial_state)[:, None, :]
            ], axis=1
        )

        return self.Stack(
            initial_outputs, initial_n_slots, initial_scores, initial_finished, initial_state, initial_attn_p
        )

    def greedy_step(self, model, stack, **flags):
        """
        :type model: tfnn.task.seq2seq.inference.translate_model.TranslateModel
        :param stack: beam search stack
        :return: new beam search stack
        """
        outputs, n_slots, scores, finished, dec_state, attn_p = stack

        n_lines = tf.shape(outputs)[0]

        finished, best_hypos, best_insertion, best_delta_scores = model.sample(
            dec_state, outputs, n_slots, eos_penalty=self.eos_penalty,
            base_scores=scores, slices=tf.range(n_lines), k=1, top_k=self.top_k,
            decoding_strategy=self.decoding_strategy, sampling_strategy=self.sampling_strategy,
            sampling_temperature=self.sampling_temperature, sampling_with_replacement=self.sampling_with_replacement
        )

        # Reshape values because k == 1
        best_hypos = tf.reshape(best_hypos, [n_lines])
        best_insertion = tf.reshape(best_insertion, [n_lines, -1, 2])
        best_delta_scores = tf.reshape(best_delta_scores, [n_lines])

        # Update hypothesises
        new_outputs, new_n_slots = model.make_insertions(outputs, n_slots, best_insertion, reshape_outputs=True)
        new_scores = scores + tf.where(tf.equal(best_hypos, -1), tf.zeros_like(best_delta_scores), best_delta_scores)

        # Update decoder state with respect to new data
        new_dec_state = model.decode(dec_state, new_outputs, new_n_slots, **flags)
        # TODO: stack attn in correct order, that corresponds performed updates
        n_new_tokens_max = tf.shape(new_outputs)[1] - tf.shape(outputs)[1]
        new_attn_p = tf.concat(
            [attn_p, tf.tile(model.get_attnP(new_dec_state)[:, None, :], [1, n_new_tokens_max, 1])], axis=1
        )

        # new_outputs = tf.Print(new_outputs, [best_insertion, best_delta_scores], "\nUPDATES:", summarize=1000000)
        return self.Stack(new_outputs, new_n_slots, new_scores, finished, new_dec_state, new_attn_p)
