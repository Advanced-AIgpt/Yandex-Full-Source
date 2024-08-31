import click
import logging
import yt.wrapper


class MarkupConverter:
    def __init__(self, input_markup_column: str, input_query_column: str, output_column: str, concatenate_strings: bool, ignore_errors: bool):
        self._input_markup_column = input_markup_column
        self._input_query_column = input_query_column
        self._output_column = output_column
        self._concatenate_strings = concatenate_strings
        self._ignore_errors = ignore_errors

    def _log_or_fail(self, msg):
        if self._ignore_errors:
            logging.warning(f'skipping query: {msg}')
        else:
            raise ValueError(msg)

    def __call__(self, row):
        query = row[self._input_query_column]

        if "\'" in query:
            self._log_or_fail(f'Query contains single quotes: {query}')
            return

        nlu_markup_parts = []
        present_types = set()
        current_offset = 0

        tagged_segments = row[self._input_markup_column]
        if tagged_segments is None:
            tagged_segments = []

        for segment in tagged_segments:
            segment_start = segment['offset']
            segment_end = segment['offset'] + segment['length']

            if query[segment_start:segment_end] != segment['segment']:
                self._log_or_fail(f'Segment and offset with length are unaligned within query: query={query}, segment={segment}')
                return

            nlu_markup_parts.append(query[current_offset:segment['offset']])

            if self._concatenate_strings and segment['label'] in present_types:
                slot_type = f"+{segment['label']}"
            else:
                slot_type = segment['label']

            nlu_markup_parts.append(f"\'{segment['segment']}\'({slot_type})")
            present_types.add(segment['label'])
            current_offset = segment_end

        nlu_markup_parts.append(query[current_offset:])

        row[self._output_column] = ''.join(nlu_markup_parts)
        yield row


@click.command()
@click.option('--proxy', required=True)
@click.option('--input-table', required=True)
@click.option('--output-table', required=True)
@click.option('--input-markup-column', default='taged_segment')
@click.option('--input-query-column', default='query')
@click.option('--output-nlu-markup-column', default='markup')
@click.option('--do-not-concatenate-strings', is_flag=True, help='do not consider separate values of a slot as chunks of a single value')
@click.option('--ignore-errors', is_flag=True)
def main(proxy, input_table, output_table, input_markup_column, input_query_column, output_nlu_markup_column, do_not_concatenate_strings, ignore_errors):
    client = yt.wrapper.YtClient(proxy=proxy)

    schema = yt.wrapper.get_attribute(input_table, 'schema', client=client)
    if schema is not None:
        schema.append({'name': output_nlu_markup_column, 'type': 'string'})

    output_table = yt.wrapper.TablePath(output_table, attributes={'schema': schema}, client=client)

    mapper = MarkupConverter(input_markup_column, input_query_column, output_nlu_markup_column, not do_not_concatenate_strings, ignore_errors)
    yt.wrapper.run_map(mapper, input_table, output_table, ordered=True, client=client)


if __name__ == "__main__":
    main()
