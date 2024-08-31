import json
import base64
import google.protobuf.json_format


# -------------------------------------------------------------------------------------------------
class ListMatcher:
    def __init__(self, sample, exact_length=False, ignore_order=False):
        assert isinstance(sample, (tuple, list))

        self._sample = sample
        self.exact_length = exact_length
        self.ignore_order = ignore_order

    def match(self, actual, path=""):
        assert isinstance(actual, list), f"{path}: expected list but got {type(actual)}"

        if self.exact_length:
            assert len(self._sample) == len(
                actual
            ), f"{path}: expected exactly {len(self._sample)} items in list (has {len(actual)})"

        idx = 0
        matched_idxs = set()

        for sample in self._sample:
            if self.ignore_order:
                idx = 0
            while idx < len(actual):
                if idx in matched_idxs:
                    idx += 1
                    continue
                try:
                    match(actual[idx], sample, path=f"{path}/{idx}")
                    matched_idxs.add(idx)
                    idx += 1
                    break
                except AssertionError:
                    idx += 1
            else:
                assert False, f"{path}: could not find sample '{sample}' in {actual}"


# -------------------------------------------------------------------------------------------------
class EvlogRecords:
    def __init__(self, samples, ignore_order=False):
        self._samples = samples
        self._ignore_order = ignore_order

    def __repr__(self):
        return f"EvlogRecords({self._samples})"

    def match(self, actual, path=""):
        lm = ListMatcher(
            [{"EventBody": {"Type": r[0], "Fields": r[1]}} for r in self._samples], ignore_order=self._ignore_order
        )
        lm.match(actual, path)


# -------------------------------------------------------------------------------------------------
class Round:
    def __init__(self, sample, ndigits=6):
        self._sample = sample
        self._ndigits = ndigits

    def __repr__(self):
        return f"Round({self._sample}, ndigits={self._ndigits})"

    def match(self, actual, path=""):
        assert type(actual) in (int, float), f"{path}: actual value '{actual}' is not a number"
        rounded = round(actual, self._ndigits)
        assert rounded == self._sample, f"{path}: {rounded} != {self._sample}"


# -------------------------------------------------------------------------------------------------
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


# -------------------------------------------------------------------------------------------------
class StartsWith:
    def __init__(self, sample):
        self._sample = sample

    def __repr__(self):
        return f"StartsWith({repr(self._sample)})"

    def match(self, actual, path=""):
        assert isinstance(actual, (str, bytes)), f"{path}: '{actual}' is not string or bytes but {type(actual)}a"
        assert actual.startswith(self._sample)


# -------------------------------------------------------------------------------------------------
class Not:
    def __init__(self, *samples):
        self._samples = samples

    def __repr__(self):
        return f"NotEqual({repr(self._samples)})"

    def match(self, actual, path=""):
        for sample in self._samples:
            try:
                soft_check_item(actual, sample, path)
            except AssertionError:
                continue
            assert False, f"{path}: '{actual} matches unwanted '{sample}'"


# -------------------------------------------------------------------------------------------------
class And:
    def __init__(self, *samples):
        self._samples = samples

    def __repr__(self):
        return f"And({repr(self._samples)})"

    def match(self, actual, path=""):
        for sample in self._samples:
            soft_check_item(actual, sample, path)


# -------------------------------------------------------------------------------------------------
class ParseJson:
    def __init__(self, sample):
        self._sample = sample

    def __repr__(self):
        return f"ParseJson(sample={repr(self._sample)})"

    def match(self, actual, path=""):
        try:
            loaded_actual = json.loads(actual)
        except json.JSONDecodeError as e:
            assert False, f"{path}: json expected, but '{actual}' is not a valid json '{e}'"
        except UnicodeDecodeError as e:
            assert False, f"{path}: json expected, but '{actual[:32]}' is not a valid json '{e}'"
        soft_check_item(loaded_actual, self._sample, path + "[parsedJson]")


# -------------------------------------------------------------------------------------------------
class ParseProtobuf:
    def __init__(self, sample):
        self._sample = sample

    def __repr__(self):
        return f"ParseProtobuf(sample={repr(self._sample)})"

    def match(self, actual, path=""):
        protocls = type(self._sample)
        try:
            parsed = protocls(self._sample)()
            parsed.ParseFromString(actual)
        except Exception as e:
            assert False, f"{path}: serialized protobuf ({protocls}) expected: {type(e)} '{e}'"

        assert parsed == self._sample


# -------------------------------------------------------------------------------------------------
class ParseProtobufToDict:
    def __init__(self, protocls, sample):
        self._protocls = protocls
        self._sample = sample

    def __repr__(self):
        return f"ParseProtobufToDict(protocls={repr(self._protocls)}, sample={repr(self._sample)})"

    def match(self, actual, path=""):
        try:
            parsed = self._protocls()
            parsed.ParseFromString(actual)
        except Exception as e:
            assert False, f"{path}: serialized protobuf ({self._protocls}) expected: {type(e)} '{e}'"
        soft_check_item(
            google.protobuf.json_format.MessageToDict(parsed),
            self._sample,
            path + f"[protobufParsedToDict({self._protocls})]",
        )


# -------------------------------------------------------------------------------------------------
class HgramCount:
    def __init__(self, count):
        self._count = count

    def __repr__(self):
        return f"HgramCount(count={self._count})"

    def match(self, actual, path=""):
        assert isinstance(actual, list)
        actual_count = sum(b[1] for b in actual)
        assert self._count == actual_count, f"{path}: actual count is {actual_count} but {self._count} expected"


# -------------------------------------------------------------------------------------------------
class HgramCountAbove:
    def __init__(self, border, count):
        self._border = border
        self._count = count

    def __repr__(self):
        return f"HgramCountAbove(border={self._border}, count={self._count})"

    def match(self, actual, path=""):
        assert isinstance(actual, list)
        actual_count = sum(b[1] for b in actual if b[0] >= self._border)
        assert self._count == actual_count, f"{path}: actual count is {actual_count} but {self._count} expected"


# -------------------------------------------------------------------------------------------------
class ToLower:
    def __init__(self, sample):
        self._sample = sample.lower()

    def __repr__(self):
        return f"ToLower(sample={self._sample})"

    def match(self, actual, path=""):
        assert actual.lower() == self._sample


# -------------------------------------------------------------------------------------------------
class Base64Decode:
    def __init__(self, sample):
        self._sample = sample

    def __repr__(self):
        return f"Base64Decode(sample={repr(self._sample)})"

    def match(self, actual, path=""):
        if isinstance(actual, str):
            actual = actual.encode("ascii")
        assert isinstance(actual, bytes)
        soft_check_item(base64.decodebytes(actual), self._sample, path + "[base64Decoded]")


# -------------------------------------------------------------------------------------------------
class Exists:
    def __repr__(self):
        return "Exists"

    def match(self, actual, path=""):
        return True


# -------------------------------------------------------------------------------------------------
def soft_check_item(it, sample, path):
    if hasattr(sample, "match"):
        return sample.match(it, path)
    if isinstance(sample, dict):
        return check_tree_contain_sample(it, sample, path)
    elif isinstance(sample, list):
        return ListMatcher(sample).match(it, path)
    assert it == sample, f"{path}: {it} != {sample} (expected)"


# -------------------------------------------------------------------------------------------------
def check_tree_contain_sample(tree, sample, path=""):
    """partial comparing dicts"""
    assert isinstance(tree, dict), f"expected dict at {path}"
    for key, val in sample.items():
        if val is None:
            assert key not in tree, f"unwanted {path}/{key} exists in tree"
        else:
            assert key in tree, f"{path}/{key} not in tree"
            soft_check_item(tree[key], val, path + "/" + key)


# -------------------------------------------------------------------------------------------------
# just a short alias for soft_check_item
def match(actual, sample, path="", noexcept=False):
    try:
        soft_check_item(actual, sample, path)
        return True  # to allow write 'assert match(...)'
    except AssertionError:
        if noexcept:
            return False
        raise
