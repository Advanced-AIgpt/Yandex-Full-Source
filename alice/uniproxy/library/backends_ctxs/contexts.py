class ContextFutures:
    def __init__(self):
        self._futures = []
        self._name2index = {}

    def __getitem__(self, name):
        return self._name2index[name]

    def __setitem__(self, name, future):
        self._name2index[name] = len(self._futures)
        self._futures.append(future)

    def __len__(self):
        return len(self._futures)

    @property
    def values(self):
        return self._futures
