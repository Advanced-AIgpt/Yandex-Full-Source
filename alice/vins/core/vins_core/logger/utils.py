import operator


def dump_sequence(seq, column_names, field_getter=operator.itemgetter, header=True, prefix=''):
    widths = []
    for column_name in column_names:
        column = map(field_getter(column_name), seq)
        if len(column) == 0:
            continue
        width = max(map(lambda x: len(str(x)), column))
        if header:
            width = max(width, len(column_name))
        widths.append(width)

    column_separator = ' | '
    row_template = prefix + column_separator.join(['{{{i}:<{w}}}'.format(i=i, w=w) for i, w in enumerate(widths)])

    result_rows = []
    if header:
        result_rows.append(row_template.format(*column_names))
        result_rows.append('-' * (sum(widths) + len(column_separator) * (len(widths) - 1)))
    for item in seq:
        row_fields = []
        for column_name in column_names:
            row_fields.append(str(field_getter(column_name)(item)))
        result_rows.append(row_template.format(*row_fields))

    return '\n'.join(result_rows)


def dump_object_sequence(seq, column_names, header=True, prefix=''):
    def item_getter(column_name):
        def get_item(obj):
            return getattr(obj, column_name)
        return get_item
    return dump_sequence(seq, column_names, item_getter, header, prefix)
