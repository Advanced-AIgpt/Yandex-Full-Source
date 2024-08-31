class RunInfo:
    @classmethod
    def load_from_file(cls, filename):
        if filename is None:
            return None

        with open(filename, 'r') as f:
            run_info = f.readline().split('\t')
            if len(run_info) != 3:
                raise Exception('Invalid run_info format')

        return cls(filename=filename, joker_bin=run_info[0], config_file=run_info[1], session_id=run_info[2])

    def __init__(self, filename, joker_bin, config_file, session_id):
        self.filename = filename
        self.joker_bin = joker_bin
        self.config_filename = config_file
        self.session_id = session_id

    def write(self):
        with open(self.filename, 'w') as f:
            f.write('{}\t{}\t{}'.format(self.joker_bin, self.config_filename, self.session_id))

    def joker_cmd(self, action):
        return [self.joker_bin, 'session', '--id', self.session_id, action, self.config_filename]
