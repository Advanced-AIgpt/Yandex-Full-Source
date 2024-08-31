from alice.tests.library.mark import Mark


def pytest_configure(config):
    for mark_name in Mark.names:
        config.addinivalue_line('markers', f'{mark_name}: ...')
