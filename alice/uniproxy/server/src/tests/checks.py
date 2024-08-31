class ListMatcher:
    def __init__(self, sample, exact_length=False):
        self._sample = sample
        self.exact_length = exact_length

    def match(self, actual, path=""):
        check_list_contain_sample(actual, self._sample, path, exact_length=self.exact_length)


def soft_check_item(it, sample, path):
    if hasattr(sample, "match"):
        sample.match(it, path)
        return
    if isinstance(sample, dict):
        check_tree_contain_sample(it, sample, path)
        return
    elif isinstance(sample, list):
        check_list_contain_sample(it, sample, path)
        return
    assert it == sample, '{} {} != {} (expected {})'.format(path, it, sample, sample)


def check_tree_contain_sample(tree, sample, path=''):
    """ partial comparing dicts
    """
    assert isinstance(tree, dict), 'expected dict at path={}'.format(path)
    for k in sorted(sample):
        assert k in tree, '{}/{} not in response tree'.format(path, k)
        soft_check_item(tree[k], sample[k], path + '/' + k)


def check_list_contain_sample(lst, sample, path='', exact_length=False):
    """ partial comparing lists
    """
    assert isinstance(lst, list), 'expected list at path={}'.format(path)
    if exact_length:
        assert len(sample) == len(lst), 'expected exactly {} items in list path={} (has {})'.format(
            len(sample), path, len(lst))
    for i in range(0, len(sample)):
        assert i < len(lst), 'expected more ({}) items at list path={} (has {})'.format(
            len(sample), path, len(lst))
        soft_check_item(lst[i], sample[i], '{}[{}]'.format(path, i))


# just a short alias for soft_check_item
def match(actual, sample, path=""):
    soft_check_item(actual, sample, path)
    return True  # to allow write 'assert match(...)'
