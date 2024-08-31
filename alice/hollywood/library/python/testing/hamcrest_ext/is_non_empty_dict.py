from hamcrest.core.base_matcher import BaseMatcher


class IsNonEmptyDict(BaseMatcher):

    def matches(self, item, mismatch_description=None):
        if not isinstance(item, dict):
            if mismatch_description:
                mismatch_description.append_text(f'but got object of type {type(item)}')
            return False

        if not item:
            if mismatch_description:
                mismatch_description.append_text(f'got {item}')
            return False

        return True

    def describe_to(self, description):
        description.append_text('a non empty dictionary')

    def describe_mismatch(self, item, mismatch_description):
        self.matches(item, mismatch_description)


def non_empty_dict():
    return IsNonEmptyDict()
