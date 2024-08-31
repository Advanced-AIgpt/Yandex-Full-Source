import alice.boltalka.generative.training.data.nn.util as util
import vh
import yt.wrapper as yt


def extract_movie_columns(row):
    movie_columns = [
        'title', 'original_title', 'director_name', 'actor_name1', 'actor_name2',
        'actor_name3', 'genre1', 'genre2', 'genre3'
    ]
    for column in movie_columns:
        movie_data = row['movie_data']
        row['movie_{}'.format(column)] = (movie_data[column] or '') if movie_data is not None else ''

    yield row


def set_otzovik_source(row):
    row['source'] = 'otzovik'
    yield row


def set_irecommend_source(row):
    row['source'] = 'irecommend'
    yield row


def merge_context_with_md(row):
    row['context_merged'] = ' [MOVIE_DATA_SEP] '.join([row['movie_data_merged'], row['context_merged']])
    yield row


def generate_otzovik_dialogs_mapper(row):
    parsed_page = row['Result'][0]
    header_ = parsed_page['_comments_section']
    if header_ is None:
        return

    comments = header_['comments_pairs']

    if len(comments) == 0:
        return

    comments_formatted = []
    for comment in comments:
        pid = comment['parent_id']
        if pid is None:
            pid = 0

        comments_formatted.append({
            'user': comment['author'],
            'id': comment['comment_id'],
            'pid': pid,
            'content': comment['comment']
        })

    dialogs = util.build_dialogs(comments_formatted, {'movie_data': row['movie_data']})
    for dialog in dialogs:
        dialog['review_body'] = parsed_page['_review_contents']['review_body_sanitized']
        dialog['review_plus'] = parsed_page['_review_contents']['review_plus']
        dialog['review_minus'] = parsed_page['_review_contents']['review_minus']
        dialog['review_summary'] = parsed_page['_review_contents']['review_summary']
        dialog['object_name'] = parsed_page['_product_header']['product_name_label']
        yield dialog


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def otzovik_to_dialogs(input_table: vh.YTTable) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())
    yt.run_map(generate_otzovik_dialogs_mapper, input_table, out_table)
    return out_table


def generate_irecommend_dialogs_mapper(row):
    parsed_page = row['Result'][0]
    header_ = parsed_page['_comments_section']
    if header_ is None:
        return

    comments = header_['comments_pairs']

    if len(comments) == 0:
        return

    comments_formatted = []
    for comment in comments:
        pid = comment['parent_id']
        if pid is None:
            pid = 0

        comments_formatted.append({
            'user': comment['author'],
            'id': comment['comment_id'],
            'pid': pid,
            'content': comment['comment']
        })

    dialogs = util.build_dialogs(comments_formatted, {'movie_data': row['movie_data']})
    for dialog in dialogs:
        dialog['review_body'] = parsed_page['_review_contents']['review_body_sanitized']
        dialog['object_name'] = parsed_page['_product_header']['product_name_label']
        yield dialog


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def irecommend_to_dialogs(input_table: vh.YTTable) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())
    yt.run_map(generate_irecommend_dialogs_mapper, input_table, out_table)
    return out_table


def generate_rezka_dialogs_mapper(row):
    comments = row['Result'][0]['comments_pairs']

    if len(comments) == 0:
        return

    comments_formatted = []
    for comment in comments:
        pid = comment['parent_id']
        if pid is None:
            pid = 0

        comments_formatted.append({
            'user': comment['author'],
            'id': comment['comment_id'],
            'pid': pid,
            'content': comment['comment']
        })

    return util.build_dialogs(comments_formatted)


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def rezka_to_dialogs(input_table: vh.YTTable) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())
    yt.run_map(generate_rezka_dialogs_mapper, input_table, out_table)
    return out_table


class MovieDataMapper:
    def __init__(self, fields_to_add):
        self.field_token_map = {
            'movie_title': '[MOVIEDATA_TITLE]',
            'movie_original_title': '[MOVIEDATA_TITLE]',
            'movie_director_name': '[MOVIEDATA_DIRECTOR]',
            'movie_actor_name1': '[MOVIEDATA_ACTOR]',
            'movie_actor_name2': '[MOVIEDATA_ACTOR]',
            'movie_actor_name3': '[MOVIEDATA_ACTOR]',
            'movie_genre1': '[MOVIEDATA_GENRE]',
            'movie_genre2': '[MOVIEDATA_GENRE]',
            'movie_genre3': '[MOVIEDATA_GENRE]'
        }

        self.fields_to_add = fields_to_add
        assert all([item in self.field_token_map for item in self.fields_to_add])

    def __call__(self, row):
        result_strings = []
        for field in self.fields_to_add:
            movie_data_item = row[field]
            if movie_data_item is not None and movie_data_item != '':
                result_strings.append(self.field_token_map[field])  # add prefix token
                result_strings.append(movie_data_item)  # add the string

        row['movie_data_merged'] = ' '.join(result_strings)
        yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy.from_annotations
def build_movie_data(input_table: vh.YTTable, fields_to_add: vh.mkinput(str, nargs='*')) -> vh.YTTable:
    out_table = yt.TablePath(yt.create_temp_table())
    yt.run_map(MovieDataMapper(fields_to_add), input_table, out_table)
    return out_table
