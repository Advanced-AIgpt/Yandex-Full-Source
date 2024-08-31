from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.helpers.wrap_matcher import wrap_matcher
from hamcrest.library.collection.isdict_containingentries import IsDictContainingEntries


class IsDictContainingOnlyEntries(BaseMatcher):

    def __init__(self, matchers_dict):
        self._has_entries = IsDictContainingEntries(matchers_dict)
        self._matchers_dict = matchers_dict

    def matches(self, item, mismatch_description=None):
        extra_keys = self._has_extra_entries(item)
        if extra_keys:
            if mismatch_description:
                mismatch_description.append_text(f'has extra keys {extra_keys} in dictionary {item}')
            return False

        return self._has_entries.matches(item, mismatch_description)

    def describe_to(self, description):
        description.append_text('dict matches given matchers and has no extra entries')

    def describe_mismatch(self, item, mismatch_description):
        self.matches(item, mismatch_description)

    def _has_extra_entries(self, item):
        diff = set(item.keys()).difference(set(self._matchers_dict.keys()))
        return diff


def has_only_entries(matchers_dict):
    return IsDictContainingOnlyEntries({
        key: wrap_matcher(val)
        for key, val in matchers_dict.items()
    })
