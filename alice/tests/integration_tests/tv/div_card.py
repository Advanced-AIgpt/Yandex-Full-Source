from alice.tests.library.vins_response import DivWrapper, DivIterableWrapper
from cached_property import cached_property


class TvChannels(DivIterableWrapper):
    class _TvProgram(DivIterableWrapper):
        class _Item(DivWrapper):
            @property
            def name(self):
                return self.data[2].text

            def has_stream(self):
                return self.data[3].action_url and 'stream' in self.data[3].action_url

    class _Item(DivWrapper):
        @property
        def name(self):
            return self.data.title

        @cached_property
        def programs(self):
            return TvChannels._TvProgram(self.data.content.table)

    def __init__(self, data):
        super().__init__(data, data.tabs)


class IndividualTvChannel(DivWrapper):
    class _TvProgram(DivIterableWrapper):
        class _Item(DivWrapper):
            @property
            def name(self):
                return self.data[1].text

    @property
    def name(self):
        return self.data[1][0][1].text

    @cached_property
    def programs(self):
        return IndividualTvChannel._TvProgram(self.data[2])
