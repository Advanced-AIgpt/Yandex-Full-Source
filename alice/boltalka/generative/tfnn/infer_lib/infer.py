import tensorflow as tf

import tfnn
from tfnn.data.saveload import load_tf_vars_from_npz, get_model_variables
from tfnn.task.seq2seq.load import load_model as tfnn_load_model
from tfnn.task.seq2seq.problems.default import DefaultProblem
from tfnn.util import nested_map, nested_flatten
from ml_data_reader.iterators.batching import form_batches


def load_model(model, model_path, ivoc_path, ovoc_path=None, hp={}, model_prefix_name='mod', device=''):
    if ovoc_path is None:
        ovoc_path = ivoc_path

    graph = tf.Graph()
    with graph.as_default():
        model = tfnn_load_model(model_prefix_name, model, ivoc_path, ovoc_path, hp)

        # Create session
        session = tfnn.session.create_session(device, gpu_allow_growth=True)

        with session.as_default():
            load_tf_vars_from_npz(model_path, get_model_variables())

    return model, session, graph


def infer_model(model,
                input_texts,
                temperature=0.6,
                sampling_top_k=50,
                sampling_nucleus_p=None,
                hypothesis_per_input=1,
                batch_size=1,
                unbpe=False,
                max_len=500,
                mode='sample',
                yield_score=False,
                unique_hypotheses_per_input=False,
                **kwargs):
    if unique_hypotheses_per_input:
        assert mode == 'sample', 'Unique hypotheses can be used only in sample mode'

    if isinstance(input_texts, str):
        input_texts = [input_texts]

    data = []
    for text in input_texts:
        data.extend([text] * hypothesis_per_input)

    batches = form_batches(data, batch_size=batch_size)

    # TODO improve code for unique_hypotheses_per_input
    if unique_hypotheses_per_input:
        unique_storage = [set() for _ in range(len(input_texts))]

    input_results = [[] for _ in range(len(input_texts))]
    while True:
        for i_batch, batch in enumerate(batches):
            results = model.translate_lines(
                batch,
                ingraph='ingraph',
                unbpe=unbpe,
                dumper=None,
                yield_score=yield_score,
                ingraph_mode=mode,
                sampling_temperature=temperature,
                max_len=max_len,
                sampling_top_k=sampling_top_k,
                nucleus_prob=sampling_nucleus_p,
                **kwargs
            )

            for i, result in enumerate(results):
                input_i = (i + i_batch * batch_size) // hypothesis_per_input

                if unique_hypotheses_per_input:
                    reply = result[0]
                    # if we did not saw this example and we did not reach the desired size, append to results
                    if reply not in unique_storage[input_i] and len(unique_storage[input_i]) < hypothesis_per_input:
                        unique_storage[input_i].add(reply)
                        input_results[input_i].append(result)
                else:
                    # normal generation, just adding output to the results
                    input_results[input_i].append(result)

        # check whether we should generate examples
        if not unique_hypotheses_per_input or \
                all([len(unique_results) == hypothesis_per_input for unique_results in unique_storage]):
            break

    return [result for input_hypotheses in input_results for result in input_hypotheses]


def _init_or_get_scoring_problem(model, temperature):
    if hasattr(model, 'scoring_problem') and model.scoring_problem[0].temperature == temperature:
        return model.scoring_problem

    problem = DefaultProblem({'mod': model}, loss_normalization_type='sequence_wise', temperature=temperature)

    # Counter magic.
    item_inp = nested_map(lambda e: tf.placeholder(dtype=tf.as_dtype(e.dtype), shape=[None] * len(e.shape)),
                          problem.make_feed_dict(['FAKE_INPUT', 'FAKE_OUTPUT']))

    # in batch_counters the losses are initialized
    problem.batch_counters(item_inp, is_train=False)
    batch_of_losses = tfnn.losses.get_losses()[0].raw_value

    model.scoring_problem = problem, item_inp, batch_of_losses

    return model.scoring_problem


def score_model(model, text_pairs, temperature=0.6, batch_size=50):
    problem, item_inp, batch_of_losses = _init_or_get_scoring_problem(model, temperature)

    loss_values = []
    # Score everything.
    for i in range(0, len(text_pairs), batch_size):
        text_batch = text_pairs[i:i + batch_size]
        # Compute.
        feed_dict = dict(zip(nested_flatten(item_inp), nested_flatten(problem.make_feed_dict(text_batch))))
        # losses = tf.get_default_session().run(item_loss_op, feed_dict=feed_dict)
        losses = tf.get_default_session().run(batch_of_losses, feed_dict=feed_dict)
        loss_values.extend(losses)

    return loss_values
