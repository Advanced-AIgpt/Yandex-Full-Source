class Cache:
    def __init__(self, factory):
        self.factory = factory
        self.cache = {}
        self.chosen = None
        self.key = None

    def choose(self, *key):
        key = tuple(el.key if isinstance(el, Cache) else el for el in key)
        self.key = key
        if not key in self.cache:
            self.cache[key] = self.factory(*key)
        self.chosen = self.cache[key]

    def __getattr__(self, attr):
        return getattr(self.chosen, attr)

    def get(self):
        return self.chosen
