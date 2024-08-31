import json


class ListMatcher:
    def __init__(self, sample, exact_length=False):
        self._sample = sample
        self.exact_length = exact_length

    def match(self, actual, path=""):
        check_list_contain_sample(actual, self._sample, path, exact_length=self.exact_length)


class Contains:
    def __init__(self, *samples):
        self._samples = samples

    def __repr__(self):
        return f"Contains({repr(self._samples)})"

    def match(self, actual, path=""):
        for sample in self._samples:
            if isinstance(actual, list):
                for it in actual:
                    try:
                        return soft_check_item(it, sample, path)
                    except AssertionError:
                        pass
            assert sample in actual, f"{path}: '{actual}' does not contain '{sample}'"


class ParseJson:
    def __init__(self, sample):
        self._sample = sample

    def __repr__(self):
        return f"ParseJson(sample={repr(self._sample)})"

    def match(self, actual, path=""):
        soft_check_item(json.loads(actual), self._sample, path)


class HgramCount:
    def __init__(self, count):
        self._count = count

    def __repr__(self):
        return f"HgramCount(count={self._count})"

    def match(self, actual, path=""):
        assert isinstance(actual, list)
        actual_count = sum(b[1] for b in actual)
        assert self._count == actual_count, f"{path}: actual count is {actual} but {self._count} expected"


class HgramCountAbove:
    def __init__(self, border, count):
        self._border = border
        self._count = count

    def __repr__(self):
        return f"HgramCountAbove(border={self._border}, count={self._count})"

    def match(self, actual, path=""):
        assert isinstance(actual, list)
        actual_count = sum(b[1] for b in actual if b[0] >= self._border)
        assert self._count == actual_count, f"{path}: actual count is {actual} but {self._count} expected"


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
    assert it == sample, f"{path}: {it} != {sample} (expected)"


def check_tree_contain_sample(tree, sample, path=""):
    """partial comparing dicts"""
    assert isinstance(tree, dict), f"expected dict at {path}"
    for k in sorted(sample):
        assert k in tree, f"{path}/{k} not in response tree"
        soft_check_item(tree[k], sample[k], path + "/" + k)


def check_list_contain_sample(lst, sample, path="", exact_length=False):
    """partial comparing lists"""
    assert isinstance(lst, list), f"expected list at {path}"
    if exact_length:
        assert len(sample) == len(lst), f"expected exactly {len(sample)} items in list at {path} (has {len(lst)})"
    for i in range(0, len(sample)):
        assert i < len(lst), f"expected more ({len(sample)}) items at list at {path} (has {len(lst)})"
        soft_check_item(lst[i], sample[i], f"{path}[{i}]")


# just a short alias for soft_check_item
def match(actual, sample, path=""):
    soft_check_item(actual, sample, path)
    return True  # to allow write 'assert match(...)'
