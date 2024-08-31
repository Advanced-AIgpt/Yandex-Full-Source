class Locale(object):
    def __init__(self, name):
        self.name = name
        self.experiments = {}
        self.use_tanker = False
