import os
import time

from tokenizer import Tokenizer

import alice.boltalka.generative.tfnn.preprocess as preprocess
import yt.wrapper as yt
from alice.boltalka.generative.tfnn.infer_lib.infer import load_model, infer_model, score_model


def process_output_string(str, is_mapper):
    """
    For mapper we need to cast to bytes whenever we output strings
    """
    return bytes(str, encoding='utf-8') if is_mapper else str


def extract_contexts_auto(row, context_columns_prefix='context'):
    contexts = []
    i = 0
    while True:
        current_context = '{}_{}'.format(context_columns_prefix, i)
        if bytes(current_context, encoding='utf-8') not in row:
            if i == 0:
                raise ValueError('Not a single context is present in the row')
            break  # if it is not the first context, then exiting just normally
        contexts.append(row[bytes(current_context, encoding='utf-8')].decode('utf-8'))

        i += 1
    return list(reversed(contexts))


def load_inference_model(model, model_path, ivoc_path, ovoc_path=None, hp={}, model_prefix_name='mod', device=''):
    hp = dict(hp)
    hp['label_smoothing'] = 0.0  # Label smoothing may affect scoring, which is not desirable
    hp['vtype'] = 'float32'  # Float32 is way more precise for score computations

    model, session, graph = load_model(
        model, model_path, ivoc_path, ovoc_path, hp,
        model_prefix_name=model_prefix_name, device=device
    )
    return model, session, graph


def batch_iterator(iterator, batch_size=1, max_items=None):
    batch = []
    for i, row in enumerate(iterator):
        if max_items is not None and i >= max_items:
            break

        if len(batch) < batch_size:
            batch.append(row)
        else:
            yield batch
            batch = [row]

    if len(batch) != 0:
        yield batch


def row_iterator_batched(input_table, batch_size=1, max_rows=None):
    rows = yt.read_table(input_table, yt.YsonFormat(encoding=None))

    rows_to_process = yt.row_count(input_table)

    if max_rows is not None:
        rows_to_process = min(max_rows, rows_to_process)

    yield from batch_iterator(rows, batch_size=batch_size, max_items=rows_to_process)


def process_table_in_batches(fn, input_table, output_table, batch_size, yt_proxy, model_params, bpe_voc_path,
                             process_n_first_rows=None):
    yt.config.set_proxy(yt_proxy)
    model, session, graph = load_inference_model(**model_params)

    tokenizer = Tokenizer(bpe_voc_path.encode('utf-8'))

    result_rows = []
    with session.as_default(), graph.as_default():
        items_processed = 0
        for row_batch in row_iterator_batched(input_table, batch_size, process_n_first_rows):
            start_time = time.time()
            results = fn(row_batch, model, tokenizer)
            items_processed += len(row_batch)
            print('#{}... ({:.2f}sec):'.format(items_processed, time.time() - start_time))

            assert len(results) == len(row_batch)

            for row, item_result in zip(row_batch, results):
                for res in item_result:
                    new_row = dict(row)
                    new_row.update(res)
                    result_rows.append(new_row)

    yt.write_table(yt.TablePath(output_table), result_rows)


@yt.aggregator
class BucketGeneratorMapper:
    def __init__(self, model_params, bpe_voc_path, fn, batch_size):
        self.model_params = model_params
        self.tokenizer = None
        self.bpe_voc_path = bpe_voc_path
        self.fn = fn
        self.batch_size = batch_size
        self.model, self.session, self.graph = None, None, None

    def __call__(self, rows):
        self.tokenizer = Tokenizer(self.bpe_voc_path.encode('utf-8'))
        self.model, self.session, self.graph = load_inference_model(**self.model_params)

        with self.session.as_default(), self.graph.as_default():
            for row_batch in batch_iterator(rows, self.batch_size):
                batch_results = self.fn(row_batch, self.model, self.tokenizer)
                for i, item_results in enumerate(batch_results):
                    for result in item_results:
                        to_yield = dict(row_batch[i])
                        to_yield.update(result)
                        yield to_yield

    @staticmethod
    def get_yt_spec(yt_pool='nirvana-dialogs', job_count=-1):
        spec = dict(
            mapper=dict(gpu_limit=1, layer_paths=[
                '//porto_layers/delta/gpu/driver/418.67',
                '//porto_layers/ubuntu-xenial-base.tar.xz'
            ]),
            pool_trees=['gpu_tesla_v100'],
            pool=yt_pool
        )
        if job_count != -1:
            spec['job_count'] = job_count
        return spec


def generate_bucket(
        input_table, output_table, yt_proxy,
        model_class, model_path, token_to_id_voc_path, bpe_voc_path, hp, separator_token,
        model_prefix_name='mod', device='', batch_size=100, sampling_hypothesis_per_input=1,
        # todo customize batch size
        sampling_temperature=0.6, sampling_topk=50, unique_hypotheses_per_input=False,
        process_n_first_rows=None, contexts_columns=None,
        max_input_len=None, max_output_len=None, use_mapper=False, mapper_jobs=-1, yt_pool='nirvana-dialogs'
):
    def func(row_batch, model, tokenizer):
        if contexts_columns is None or len(
                contexts_columns) == 0:  # TODO more elegant check for contexts_columns required
            contexts = [extract_contexts_auto(row) for row in row_batch]
        else:
            contexts = [[row[bytes(column, encoding='utf-8')].decode('utf-8') for column in contexts_columns] for row in
                        row_batch]

        input_strings = [
            preprocess.preprocess_contexts_and_tokenize(context, separator_token, tokenizer) for context in contexts
        ]

        if max_input_len is not None:
            input_strings = [' '.join(str.split(' ')[:max_input_len]) for str in input_strings]

        kwargs = dict()
        if max_output_len is not None:
            kwargs['max_len'] = max_output_len

        infer_results = infer_model(
            model,
            input_strings,
            temperature=sampling_temperature,
            sampling_top_k=sampling_topk,
            hypothesis_per_input=sampling_hypothesis_per_input,
            mode='sample',
            yield_score=True,
            batch_size=batch_size,
            unique_hypotheses_per_input=unique_hypotheses_per_input,
            **kwargs
        )

        assert len(infer_results) == len(input_strings) * sampling_hypothesis_per_input

        results = []
        for i in range(0, len(input_strings) * sampling_hypothesis_per_input, sampling_hypothesis_per_input):
            item_results = []
            for (reply, score) in infer_results[i:i + sampling_hypothesis_per_input]:
                score = float(score)
                processed_reply = preprocess.process_response(reply)
                num_tokens = len(reply.split()) + 1  # counting _EOS_ token

                if not use_mapper:
                    print('\t{} (Full score: {:.8f}, Score: {:.8f})'.format(processed_reply, score, score / num_tokens))

                item_results.append({
                    b'tokenized_reply': process_output_string(reply, is_mapper=use_mapper),
                    b'reply': process_output_string(processed_reply, is_mapper=use_mapper),
                    b's2s_num_tokens': num_tokens,
                    b's2s_num_words': len(reply.replace(' `', '').split()),
                    b's2s_score': score / num_tokens,
                    b's2s_full_score': score
                })
            results.append(item_results)
        return results

    if use_mapper:
        if yt_pool is None:
            raise ValueError('YT pool for mapper should be specified, to use GPUs in mapper')

        yt.run_map(
            BucketGeneratorMapper(
                model_params=dict(
                    model=model_class, model_path=os.path.basename(model_path),
                    ivoc_path=os.path.basename(token_to_id_voc_path), ovoc_path=os.path.basename(token_to_id_voc_path),
                    hp=hp, model_prefix_name=model_prefix_name, device=device
                ),
                bpe_voc_path=os.path.basename(bpe_voc_path),
                fn=func,
                batch_size=batch_size
            ),
            input_table,
            output_table,
            spec=BucketGeneratorMapper.get_yt_spec(yt_pool=yt_pool, job_count=mapper_jobs),
            local_files=[bpe_voc_path, token_to_id_voc_path, model_path],
            memory_limit=32 * yt.common.GB,
            format=yt.YsonFormat(encoding=None)
        )
    else:
        process_table_in_batches(
            func, input_table, output_table, batch_size, yt_proxy,
            model_params=dict(
                model=model_class, model_path=model_path,
                ivoc_path=token_to_id_voc_path, ovoc_path=token_to_id_voc_path,
                hp=hp, model_prefix_name=model_prefix_name, device=device
            ),
            bpe_voc_path=bpe_voc_path,
            process_n_first_rows=process_n_first_rows
        )


def score_bucket(
        input_table, output_table, yt_proxy,
        model_class, model_path, token_to_id_voc_path, bpe_voc_path, hp, separator_token,
        model_prefix_name='mod', device='', sampling_temperature=0.6, reply_column='tokenized_reply',
        tokenize_reply_column=False, context_columns_prefix=None, process_n_first_rows=None, batch_size=50
):
    def func(row_batch, model, tokenizer):
        if context_columns_prefix is not None:
            input_strings = [
                preprocess.preprocess_contexts_and_tokenize(
                    extract_contexts_auto(row, context_columns_prefix), separator_token, tokenizer
                ) for row in row_batch
            ]
        else:
            input_strings = ['' for _ in row_batch]

        output_strings = []
        for row in row_batch:
            output_string = row[bytes(reply_column, encoding='utf-8')].decode('utf-8')
            if tokenize_reply_column:
                output_string = preprocess.punct_separate(output_string)
                # TODO remove the "-" removal when it is fixed in production
                output_string = output_string.replace(' - ', ' ')
                output_string = tokenizer.tokenize(output_string.encode('utf-8')).decode('utf-8')
            output_strings.append(output_string)

        scores = score_model(
            model, list(zip(input_strings, output_strings)), temperature=sampling_temperature, batch_size=batch_size
        )

        results = []
        for input_string, output_string, score in zip(input_strings, output_strings, scores):
            score = float(score)
            num_tokens = len(output_string.split()) + 1  # counting _EOS_ token

            print('Input: {}, Output: {}, (Full score: {:.8f}, Score: {:.8f})'.format(
                input_string, output_string, -score, -score / num_tokens
            ))

            results.append([{
                b's2s_num_tokens': num_tokens,
                b's2s_num_words': len(output_string.replace(' `', '').split()),
                b's2s_full_score': -score,
                b'score': -score / num_tokens,
                b's2s_score': -score / num_tokens,
                b'inv_score': -score / num_tokens,
                b'model_score': -score / num_tokens,
                b'full_score': -score
            }])
        return results

    process_table_in_batches(
        func, input_table, output_table, batch_size, yt_proxy,
        model_params=dict(
            model=model_class, model_path=model_path,
            ivoc_path=token_to_id_voc_path, ovoc_path=token_to_id_voc_path,
            hp=hp, model_prefix_name=model_prefix_name, device=device
        ),
        bpe_voc_path=bpe_voc_path,
        process_n_first_rows=process_n_first_rows
    )
