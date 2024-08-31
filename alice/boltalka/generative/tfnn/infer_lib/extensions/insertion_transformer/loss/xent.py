import numpy as np
import tensorflow as tf
from tfnn.ops import nop
from tfnn.util import str_to_dtype

from tfnn.layers.dense import Dense
from tfnn.layers.xent import SequenceLossBase


class LossXentInsertion(SequenceLossBase):
    def __init__(
            self, name, rdo_size, voc, hp,
            matrix=None, bias=None,
            mixture_matrix=None, mixture_bias=None,
            mixture_weights_matrix=None, mixture_weights_bias=None,
            context_matrix=None, context_bias=None,
            location_matrix=None, location_bias=None,
            matrix_initializer=None, bias_initializer=tf.zeros_initializer(),
            mixture_matrix_initializer=None, mixture_bias_initializer=tf.zeros_initializer(),
            mixture_weights_matrix_initializer=None, mixture_weights_bias_initializer=tf.zeros_initializer(),
            location_matrix_initializer=None, location_bias_initializer=tf.zeros_initializer(),
            context_matrix_initializer=None, context_bias_initializer=tf.zeros_initializer(),
            dtype=tf.float32,
    ):
        """
          Dense: <name>/logits
        """
        self.name = name
        self.rdo_size = rdo_size
        self.voc_size = voc.size()
        self.n_adjacent = hp.get('n_adjacent', 1)
        self.n_mixtures = hp.get('n_mixtures', 2)
        self.use_mixture = hp.get('use_mixture', False)
        self.slot_rdo_size = 2 * rdo_size * self.n_adjacent
        self.label_smoothing = hp.get('label_smoothing', 0.0)
        self.vtype = str_to_dtype(hp.get('loss_vtype', dtype))
        self.use_context = hp.get('use_context', True)
        self.distribution_mode = hp.get('distribution_mode', 'factored')
        self.termination_mode = hp.get('termination_mode', 'slot_finalization')
        self.use_annealing = hp.get('use_annealing', False)
        if self.use_annealing:
            try:
                self.annealing_n_steps = float(hp.get('annealing_n_steps'))
                self.annealing_first_step = float(hp.get('annealing_first_step'))
            except KeyError:
                raise ValueError("if use_annealing then you must specify annealing_n_steps and annealing_first_step")
        self.termination_loss_coefficient = hp.get('termination_loss_coefficient', 1.0)
        # create additional output for end_of_slot token in slot_finalization mode
        self.bos, self.eos, self.eoslot = voc.BOS, voc.EOS, self.voc_size
        self.voc_size += self.termination_mode == 'slot_finalization'
        with tf.variable_scope(name):
            if self.use_mixture:
                self._rdo_to_mixture_weights = Dense(
                    'mixture_weights', self.slot_rdo_size, self.n_mixtures, activ=nop,
                    matrix=mixture_weights_matrix, bias=mixture_weights_bias,
                    matrix_initializer=mixture_weights_matrix_initializer,
                    bias_initializer=mixture_weights_bias_initializer,
                    dtype=dtype
                )
                self._rdo_to_mixture_components = Dense(
                    'mixture', self.slot_rdo_size, self.n_mixtures * self.slot_rdo_size, activ=tf.nn.relu,
                    matrix=mixture_matrix, bias=mixture_bias,
                    matrix_initializer=mixture_matrix_initializer, bias_initializer=mixture_bias_initializer,
                    dtype=dtype
                )
            if self.distribution_mode == 'joint':
                pass
            elif self.distribution_mode == 'factored':
                self._rdo_to_location = Dense(
                    'location', self.slot_rdo_size, 1, activ=nop,
                    matrix=location_matrix, bias=location_bias,
                    matrix_initializer=location_matrix_initializer, bias_initializer=location_bias_initializer,
                    dtype=dtype
                )
            else:
                raise ValueError("token-slot distribution can be either joint or factored")
            if self.use_context:
                self._rdo_to_context = Dense(
                    'context', self.slot_rdo_size, self.voc_size, activ=nop,
                    matrix=context_matrix, bias=context_bias,
                    matrix_initializer=context_matrix_initializer, bias_initializer=context_bias_initializer,
                    dtype=dtype
                )
            self._rdo_to_tokens = Dense(
                'tokens', self.slot_rdo_size, self.voc_size, activ=nop,
                matrix=matrix, bias=bias,
                matrix_initializer=matrix_initializer, bias_initializer=bias_initializer,
                dtype=dtype,
            )

    def __call__(self, rdo, batch):
        """
        :param rdo: [n_lines, (n_out + 2), hid_dim]
        :return: [1]
        """
        logits = self.rdo_to_logits(rdo)
        return self.logits_to_loss(logits, batch)

    def slots_rdo_to_token_logits(self, slots_rdo):
        """
        :param slots_rdo: [n_lines, (n_out + 1), Optional[n_mixtures], slot_rdo_size]
        :return: [n_lines, (n_out + 1), Optional[n_mixtures], voc_size]
        """
        return self._rdo_to_tokens(slots_rdo)

    def slots_rdo_to_context_logits(self, slots_rdo):
        """
        :param slots_rdo: [n_lines, (n_out + 1), Optional[n_mixtures], slot_rdo_size]
        :return: [n_lines, 1, Optional[n_mixtures], voc_size]
        """
        g = tf.reduce_max(slots_rdo, axis=1, keepdims=True)
        return self._rdo_to_context(g)

    def slots_rdo_to_location_logits(self, slots_rdo):
        """
        :param slots_rdo: [n_lines, (n_out + 1), Optional[n_mixtures], slot_rdo_size]
        :return: [n_lines, (n_out + 1), Optional[n_mixtures], 1]
        """
        return self._rdo_to_location(slots_rdo)

    def slots_rdo_to_mixture_weights(self, slots_rdo):
        """
        :param slots_rdo: [n_lines, (n_out + 1), slot_rdo_size]
        :return: [n_lines, (n_out + 1), n_mixtures]
        """
        if self.use_mixture:
            return self._rdo_to_mixture_weights(slots_rdo)
        raise ValueError('LogicError mixture of softmaxes')

    def slots_rdo_to_mixture_components(self, slots_rdo):
        """
        :param slots_rdo: [n_lines, (n_out + 1), slot_rdo_size]
        :return: [n_lines, (n_out + 1), n_mixtures, slot_rdo_size]
        """
        if self.use_mixture:
            n_lines, n_slots_max = tf.shape(slots_rdo)[0], tf.shape(slots_rdo)[1]
            return tf.reshape(self._rdo_to_mixture_components(slots_rdo), [n_lines, n_slots_max, self.n_mixtures, -1])
        raise ValueError('LogicError mixture of softmaxes')

    def slots_rdo_to_logits(self, slots_rdo):
        """
        Returns either tuple(Logits(c|l), Logits(l)) or tuple(Logits(c, l))
        :param slots_rdo: [n_lines, (n_out + 1), Optional[n_mixtures], slot_rdo_size]
        :return: Tuple[
                    [n_lines, (n_out + 1), Optional[n_mixtures], voc_size],
                    Optional[[n_lines, (n_out + 1), Optional[n_mixtures], 1]]
                ]
        """
        logits = self.slots_rdo_to_token_logits(slots_rdo)
        if self.use_context:
            logits += self.slots_rdo_to_context_logits(slots_rdo)
        if self.distribution_mode == 'factored':
            location_logits = self.slots_rdo_to_location_logits(slots_rdo)
            return logits, location_logits
        # Return List with only one element to have consistency and uniform logits processing
        # in _packed_logits_to_log_probas, logits_to_log_probas in factored and joint modes
        return [logits, ]

    def rdo_to_slots_rdo(self, rdo):
        """
        :param rdo: [n_lines, (n_out + 2), hid_dim]
        :return: [n_lines, (n_out + 1), slot_rdo_size]
        """
        rdo_padded = tf.pad(rdo, [[0, 0], [self.n_adjacent - 1, self.n_adjacent - 1], [0, 0]], constant_values=0.0)
        slices = [
            rdo_padded[:, idx:tf.shape(rdo_padded)[1] - 2 * self.n_adjacent + idx + 1]
            for idx in range(2 * self.n_adjacent)
        ]
        slots_rdo = tf.concat(slices, axis=-1)
        return slots_rdo

    def rdo_to_logits(self, rdo):
        """
        :param rdo: [n_lines, (n_out + 2), hid_dim]
        :return:
            If mixture of softmaxes is used returns either
                Tuple[Tuple[Logits(c|l), Logits(l)], log_weights] or just Tuple[Tuple[Logits(c, l)], log_weights]
            Otherwise returns either Tuple[Logits(c|l), Logits(l)] or just Tuple[Logits(c, l)]
            tf.shape(Logits(l)) == [n_lines, (n_out + 1), Optional[n_mixtures], 1]
            tf.shape(Logits(c, l)) == tf.shape(Logits(c|l)) == [n_lines, (n_out + 1), Optional[n_mixtures], voc_size]
        """
        if self.vtype:
            rdo = tf.cast(rdo, self.vtype)
        # tf.shape(slots_rdo_base) == [n_lines, n_out + 1, 2 * hid_dim * n_adjacent]
        slots_rdo_base = self.rdo_to_slots_rdo(rdo)
        if self.use_mixture:
            slots_rdo = self.slots_rdo_to_mixture_components(slots_rdo_base)
            log_mixture_weights = tf.nn.log_softmax(self.slots_rdo_to_mixture_weights(slots_rdo_base), axis=-1)
            return self.slots_rdo_to_logits(slots_rdo), log_mixture_weights
        else:
            return self.slots_rdo_to_logits(slots_rdo_base)

    def apply_log_probas_token_penalty(self, log_probas, token, penalty, operation=tf.subtract):
        n_lines, n_slots_max, n_tokens = tf.shape(log_probas)[0], tf.shape(log_probas)[1], tf.shape(log_probas)[2]
        # TODO: find more clear way to do log_probas[:, :, token] = op(log_probas[:, :, token], penalty)
        log_probas = tf.where(
            tf.tile(tf.equal(tf.range(n_tokens), token)[None, None, :], [n_lines, n_slots_max, 1]),
            operation(log_probas, penalty), log_probas
        )
        return log_probas

    def _packed_logits_to_log_probas(self, packed_logits, n_slots):
        """
        :param packed_logits:
            Tuple[
                [n_mixtures, n_lines, (n_out + 1), voc_size],
                Optional[[n_mixtures, n_lines, (n_out + 1), 1]
            ]
        :param n_slots: [n_lines]
        :return: [n_mixtures, n_lines, (n_out + 1), voc_size]
        """
        if self.distribution_mode == 'joint':
            [logits] = packed_logits
            n_mixtures, n_lines, n_slots_max, voc_size = (
                tf.shape(logits)[0], tf.shape(logits)[1], tf.shape(logits)[2], tf.shape(logits)[3]
            )
            mask = tf.sequence_mask(n_slots, n_slots_max)
            mask_expanded = tf.tile(mask[None, :, :, None], [n_mixtures, 1, 1, voc_size])
            logits = tf.where(mask_expanded, logits, tf.fill(tf.shape(logits), -np.inf))
            # Note: for each line exists al least one valid slot -> exists at least one non -inf value in each line ->
            #   tf.nn.log_softmax(...logits...) is a valid operation
            log_probas = tf.reshape(
                tf.nn.log_softmax(tf.reshape(logits, [n_mixtures, n_lines, -1]), axis=-1),
                tf.shape(logits)
            )
        else:
            [logits, location_logits] = packed_logits
            n_mixtures, n_lines, n_slots_max, voc_size = (
                tf.shape(logits)[0], tf.shape(logits)[1], tf.shape(logits)[2], tf.shape(logits)[3]
            )
            mask = tf.tile(tf.sequence_mask(n_slots, n_slots_max)[None, :, :, None], [n_mixtures, 1, 1, 1])
            mask_expanded = tf.tile(mask[:, :, :, :], [1, 1, 1, voc_size])
            # Note: for each line exists al least one valid slot -> exists at least one non -inf value in each line ->
            #   tf.nn.log_softmax(...location_logits_masked...) is a valid operation
            location_logits_masked = tf.where(
                mask, location_logits, tf.fill(tf.shape(location_logits), -np.inf)
            )
            log_probas_location_masked = tf.nn.log_softmax(location_logits_masked, axis=2)
            # Note: some slots are invalid ->  tf.nn.log_softmax(...logits_masked...) is NOT a valid operation ->
            #  we must interchange masking and tf.nn.log_softmax
            log_probas_base = tf.nn.log_softmax(logits, axis=-1)
            log_probas_base_masked = tf.where(mask_expanded, log_probas_base, tf.fill(tf.shape(logits), -np.inf))
            log_probas = log_probas_base_masked + log_probas_location_masked
        return log_probas

    def logits_to_log_probas(self, packed_logits, n_slots):
        """
        :param packed_logits:
            If mixture of softmaxes is used returns either
                Tuple[Tuple[Logits(c|l), Logits(l)], log_weights] or just Tuple[Tuple[Logits(c, l)], log_weights]
            Otherwise returns either Tuple[Logits(c|l), Logits(l)] or just Tuple[Logits(c, l)]
            tf.shape(Logits(l)) == [n_lines, (n_out + 1), Optional[n_mixtures], 1]
            tf.shape(Logits(c|l)) == tf.shape(Logits(c|l)) == [n_lines, (n_out + 1), Optional[n_mixtures], voc_size]
        :param n_slots: [n_lines]
        :return: [n_lines, (n_out + 1), voc_size]
        """
        if self.use_mixture:
            packed_logits, log_weights = packed_logits
            packed_logits_expanded = []
            # Transform all logits in the same structure with n_mixtures over first axis
            for chunk in packed_logits:
                packed_logits_expanded.append(tf.transpose(chunk, [2, 0, 1, 3]))
            log_weights_expanded = tf.transpose(log_weights[:, :, :, None], [2, 0, 1, 3])
            log_probas_components = self._packed_logits_to_log_probas(packed_logits_expanded, n_slots)
            log_probas_components = log_weights_expanded + log_probas_components
            n_mixtures, n_lines, n_slots_max, voc_size = (
                tf.shape(log_probas_components)[0], tf.shape(log_probas_components)[1],
                tf.shape(log_probas_components)[2], tf.shape(log_probas_components)[3]
            )
            mask = tf.tile(tf.sequence_mask(n_slots, n_slots_max)[:, :, None], [1, 1, voc_size])
            mask_expanded = tf.tile(mask[None, :, :, :], [n_mixtures, 1, 1, 1])
            save_log_probas_preprocessed = tf.where(
                mask_expanded, log_probas_components,
                tf.fill(tf.shape(log_probas_components), tf.float32.min)
            )
            save_log_probas = tf.reduce_logsumexp(save_log_probas_preprocessed, axis=0)
            log_probas = tf.where(
                mask, save_log_probas,
                tf.fill([n_lines, n_slots_max, voc_size], -np.inf)
            )
            return log_probas
        else:
            packed_logits_expanded = []
            # Transform all logits in the same structure with n_mixtures over first axis
            for chunk in packed_logits:
                packed_logits_expanded.append(tf.expand_dims(chunk, 0))
            log_probas_expanded = self._packed_logits_to_log_probas(packed_logits_expanded, n_slots)
            return tf.squeeze(log_probas_expanded, 0)

    def _get_annealing_coefficients(self):
        if self.use_annealing:
            graph = tf.get_default_graph()
            global_step = graph.get_tensor_by_name("global_step:0")
            global_step = tf.cast(global_step, tf.float32)
            main_coefficient, termination_coefficient = tf.cond(
                tf.logical_and(
                    tf.less_equal(self.annealing_first_step, global_step),
                    tf.less(global_step, self.annealing_first_step + self.annealing_n_steps)
                ),
                lambda: [1.0, (global_step - self.annealing_first_step) / self.annealing_n_steps],
                lambda: [1.0, 1.0]
            )
        else:
            main_coefficient, termination_coefficient = 1.0, 1.0
        return main_coefficient, termination_coefficient

    def logits_to_loss(self, logits, batch):
        """
        :param logits:
            If mixture of softmaxes is used:
                List[Tuple[log_weight, Logits(c|l), Logits(l)]] or List[Tuple[log_weight, Logits(c, l)]]
            Otherwise:
                Tuple[Logits(c|l), Logits(l)] or just Logits(c, l)
        :param batch:
        :return: [1]
        """
        log_probas = self.logits_to_log_probas(logits, batch['n_slots'])
        if isinstance(self.termination_loss_coefficient, str):
            if self.termination_loss_coefficient == 'n_slots':
                log_probas = self.apply_log_probas_token_penalty(
                    log_probas, self.eos, tf.cast(batch['n_slots'], tf.float32)[:, None, None], tf.divide
                )
                log_probas = self.apply_log_probas_token_penalty(
                    log_probas, self.eoslot, tf.cast(batch['n_slots'], tf.float32)[:, None, None], tf.divide
                )
            else:
                raise ValueError("termination loss coefficient must be either numeric or one of {'n_slots'}")
        else:
            log_probas = self.apply_log_probas_token_penalty(
                log_probas, self.eos, self.termination_loss_coefficient, tf.multiply
            )
            log_probas = self.apply_log_probas_token_penalty(
                log_probas, self.eoslot, self.termination_loss_coefficient, tf.multiply
            )
        # Because loss of one line is averaged with respect to number of slots perform division before gathering
        log_probas = log_probas / tf.cast(batch['n_slots'], tf.float32)[:, None, None]
        if self.termination_mode == 'slot_finalization':
            termination_loss = -tf.reduce_sum(tf.gather_nd(log_probas, batch['empty_slots'])[:, self.eoslot])
        elif self.termination_mode == 'sequence_finalization':
            termination_loss = -tf.reduce_sum(tf.gather_nd(log_probas, batch['finalizing_slots'])[:, self.eos])
        else:
            raise ValueError('termination mode must be either slot_finalization or sequence_finalization')
        if self.label_smoothing > 0:
            smooth_positives = tf.cast(1.0 - self.label_smoothing, tf.float32)
            # TODO: somehow fix negatives coefficient (because number of positives in each slot greater than 1)
            smooth_negatives = tf.cast(self.label_smoothing, tf.float32) / tf.cast(self.voc_size - 1, tf.float32)
            mask = tf.sequence_mask(batch['n_slots'], tf.shape(log_probas)[1])
            main_loss = -(
                tf.reduce_sum(tf.gather_nd(log_probas, batch['inserts']) * (
                    smooth_positives * batch['weights'] - smooth_negatives
                )) +
                tf.reduce_sum(tf.boolean_mask(log_probas, mask)) * smooth_negatives
            )
            termination_loss = termination_loss * (smooth_positives - smooth_negatives)
        else:
            main_loss = -tf.reduce_sum(tf.gather_nd(log_probas, batch['inserts']) * batch['weights'])
        main_coefficient, termination_coefficient = self._get_annealing_coefficients()
        # main_loss = tf.Print(main_loss, [
        #     main_loss, termination_loss, main_coefficient, termination_coefficient
        # ], "\nLOSS: ", summarize=1000000)
        return main_coefficient * main_loss + termination_coefficient * termination_loss

    def rdo_to_logits__predict(self, rdo):
        """
        Returns either tuple (p(c|l), p(l)) or just p(c, l)
        :param rdo: [n_lines, (n_out + 2), hid_dim]
        :return: [n_lines, (n_out + 1), voc_size], Optional[[n_lines, (n_out + 1), 1]]
        """
        return self.rdo_to_logits(rdo)
