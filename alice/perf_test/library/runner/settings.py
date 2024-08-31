class Settings:
    def __init__(self, binary_path, force_update=0, fetch_if_not_exists=0, workdir=None):
        self.binary_path = binary_path
        self.force_update = force_update
        self.fetch_if_not_exists = fetch_if_not_exists
        self.workdir = workdir

    def get_binary_path(self):
        return self.binary_path

    def get_workdir(self, default):
        if self.workdir:
            return self.workdir
        return default

    def is_force_update_enabled(self):
        return self.force_update

    def is_fetch_if_not_exists_enabled(self):
        return self.fetch_if_not_exists
