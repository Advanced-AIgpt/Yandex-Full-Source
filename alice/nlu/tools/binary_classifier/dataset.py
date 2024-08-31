import json
import copy
from alice.nlu.tools.binary_classifier.utils import (
    save_lines,
    load_tsv_simple,
    save_tsv_simple,
    crop_with_ellipsis,
    create_yt_client,
    is_yt_table_exists
)


# ==== Dataset ====

class Dataset(object):
    """
        Поля:
            header - список названий столбцов
            rows - список сэмплов (сэмпл - список строк)
            columns - название столбца -> его индекс
            it - индекс столбца text, либо None
            iw - индекс столбца weight, либо None
            im - индекс столбца mock, либо None
            ie - индекс столбца embeddings, либо None
    """

    def __init__(self, header=[], rows=[], path=''):
        if path:
            self.header, self.rows = load_tsv_simple(path)
        else:
            self.header = header.copy()
            self.rows = rows.copy()
        self.update_info()

    def update_info(self):
        self.columns = {column : i for i, column in enumerate(self.header)}
        self.it = self.columns.get('text', None)
        self.iw = self.columns.get('weight', None)
        self.im = self.columns.get('mock', None)
        self.ie = self.columns.get('embeddings', None)

    def deepcopy(self):
        return Dataset(copy.deepcopy(self.header), copy.deepcopy(self.rows))

    def __str__(self):
        return self.dump()

    def dump(self, indent=''):
        out = indent + 'Dataset:\n'
        out += indent + f'  header: {self.header}\n'
        out += indent + f'  row count: {len(self.rows)}\n'
        if self.rows:
            out += indent + '  row 0:\n'
            for key, value in zip(self.header, self.rows[0]):
                out += indent + f'    {key}: {crop_with_ellipsis(str(value), 80)}\n'
        return out

    @classmethod
    def load(cls, path, format='tsv', yt_client=None):
        if format == 'tsv':
            return cls.load_from_tsv(path)
        elif format == 'txt':
            return cls.load_from_txt(path)
        elif format == 'yt':
            return cls.load_from_yt(path, yt_client)
        else:
            raise ValueError('Unknown format of dataset: "%s".' % format)

    @classmethod
    def load_from_tsv(cls, path):
        header, rows = load_tsv_simple(path)
        return cls(header, rows)

    @classmethod
    def load_from_txt(cls, path):
        dataset = cls(header=['text'])
        with open(path) as file:
            for row in file:
                row = row.strip()
                if not row or row.startswith('#'):
                    continue
                dataset.rows.append([row])
        return dataset

    @classmethod
    def load_from_yt(cls, path, yt_client=None):
        if yt_client is None:
            yt_client = create_yt_client()
        if not is_yt_table_exists(path, yt_client):
            raise OSError('YT table not found: "%s"' % path)
        header = [column['name'] for column in yt_client.get(path + '/@schema')]
        dataset = cls(header=header)
        for row in yt_client.read_table(path):
            dataset.rows.append([row.get(column) for column in header])
        return dataset

    def save(self, path, format='tsv', yt_client=None):
        if format == 'tsv':
            return self.save_to_tsv(path)
        elif format == 'txt':
            return self.save_to_txt(path)
        elif format == 'yt':
            return self.save_to_yt(path, yt_client)
        else:
            raise ValueError('Unknown format of dataset: "%s".' % format)

    def save_to_tsv(self, path):
        save_tsv_simple(self.header, self.rows, path)

    def save_to_txt(self, path, sep='  '):
        save_lines(self._format_as_table_lines(sep), path)

    def save_to_yt(self, path, yt_client=None):
        if yt_client is None:
            yt_client = create_yt_client()
        if is_yt_table_exists(path, yt_client):
            yt_client.remove(path)
        # schema = [{'name': column, 'type': 'string'} for column in self.header]
        # yt_client.create("table", path, attributes={"schema": schema})
        yt_client.write_table(path, ({k: v for k, v in zip(self.header, row)} for row in self.rows))

    def format_as_table(self, sep='  '):
        return '\n'.join(self._format_as_table_lines(sep)) + '\n'

    def _format_as_table_lines(self, sep='  '):
        widths = []
        for i, name in enumerate(self.header):
            width = max(len(self._cell_str(row[i])) for row in self.rows)
            width = max(width, len(name))
            widths.append(width)
        def format_row(row):
            cells = [self._cell_str(cell).ljust(width) for cell, width in zip(row, widths)]
            return sep.join(cells)
        lines = []
        lines.append(format_row(self.header))
        for row in self.rows:
            lines.append(format_row(row))
        return lines

    @staticmethod
    def _cell_str(cell):
        return str(cell).replace('\t', ' ').replace('\r', ' ').replace('\n', ' ')

    def row_count(self):
        return len(self.rows)

    def has_weight(self):
        return self.iw is not None

    def get_weight(self, n):
        return float(self.rows[n][self.iw])

    def set_weight(self, weight, n):
        if isinstance(weight, float) and round(weight) == weight:
            weight = int(weight)
        self.rows[n][self.iw] = str(weight)

    def get_weight_sum(self):
        return sum(float(row[self.iw]) for row in self.rows)

    def get_text(self, n):
        return self.rows[n][self.it]

    def has_mock(self):
        return self.im is not None

    def get_mock(self, n):
        return json.loads(self.rows[n][self.im])

    def set_mock(self, mock, n):
        self.rows[n][self.im] = json.dumps(mock, ensure_ascii=False, separators=(',', ':'))

    def get_entities(self, n):
        return self._unpack_entities(self.get_mock(n)['Entities'])

    def set_entities(self, entities, n):
        mock = self.get_mock(n)
        mock['Entities'] = self._pack_entities(entities)
        self.set_mock(mock, n)

    @staticmethod
    def _unpack_entities(entities):
        keys, values2d = zip(*[(k, v) for k, v in entities.items()])
        return [dict(zip(keys, values)) for values in zip(*values2d)]

    @staticmethod
    def _pack_entities(entities):
        if entities:
            keys = entities[0].keys()
        else:
            keys = ["Begin", "End", "Flags", "LogProbability", "Quality", "Source", "Type", "Value"]
        return {key: [entity[key] for entity in entities] for key in keys}

    def has_embeddings(self):
        return self.ie is not None

    def get_embeddings(self, n):
        return json.loads(self.rows[n][self.ie])

    def set_embeddings(self, embeddings, n):
        self.rows[n][self.ie] = json.dumps(embeddings, ensure_ascii=False, separators=(',', ':'))

    def get_tokens(self, n):
        mock = self.get_mock(n)
        text = mock['Text'].encode('utf8')
        return [text[b:e].decode('utf8') for b, e in zip(mock['TokenBegin'], mock['TokenEnd'])]

    def select_columns(self, columns):
        indexes = [self.columns.get(column, None) for column in columns]
        new_rows = [[('' if i is None else row[i]) for i in indexes] for row in self.rows]
        return Dataset(columns, new_rows)

    def sort_by_weight(self):
        self.rows.sort(key=lambda row: (-float(row[self.iw]), row[self.it]))

    def sort_by_text(self):
        self.rows.sort(key=lambda row: row[self.it])

    def sorted_by_weight(self):
        result = self.deepcopy()
        result.sort_by_weight()
        return result

    def sorted_by_text(self):
        result = self.deepcopy()
        result.sort_by_text()
        return result

    def merge_duplicated(self, aggregate_weight_by=sum):
        text_to_index = {}
        for i, row in enumerate(self.rows):
            text = row[self.it]
            if text not in text_to_index:
                text_to_index[text] = i
                continue
            j = text_to_index[text]
            if self.has_weight():
                weight = aggregate_weight_by((self.get_weight(j), self.get_weight(i)))
                self.set_weight(weight, j)
        self.rows = [self.rows[i] for i in sorted(text_to_index.values())]

    def merge_duplicated_return(self, *args, **kwargs):
        result = self.deepcopy()
        result.merge_duplicated(*args, **kwargs)
        return result
