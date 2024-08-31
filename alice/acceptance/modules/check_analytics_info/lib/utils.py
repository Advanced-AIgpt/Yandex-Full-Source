def collect_dump_data(checker, rows):
    row_dumps = [checker.process_row(row) for row in rows]
    result = checker.dump_result()
    return {
        'result': result,
        'row_dumps': row_dumps
    }


def generate_file_name(checker):
    return checker.NAME
