import os


class Settings:
    _envname_runinfo_file = 'JOKER_MOCKER_SESSION_INFO_FILE'
    _envname_workdir = 'JOKER_MOCKER_WORK_DIR'
    _envname_force_update = 'JOKER_MOCKER_FORCE_UPDATE'
    _envname_fetch_if_not_exists = 'JOKER_MOCKER_FETCH_IF_NOT_EXISTS'
    _envname_session_id = 'JOKER_MOCKER_SESSION_ID'
    _envname_arcadia_path = 'JOKER_MOCKER_ARCADIA_PATH'

    def get_runinfo_file(self):
        return os.environ.get(Settings._envname_runinfo_file)

    def set_runinfo_file(self, filename, env):
        env[Settings._envname_runinfo_file] = filename

    def set_workdir(self, dir, env):
        env[Settings._envname_workdir] = dir

    def get_workdir(self, default):
        return os.environ.get(Settings._envname_workdir, default)

    def is_force_update_enabled(self):
        return os.environ.get(Settings._envname_force_update, 0)

    def enable_force_update(self, env):
        env[Settings._envname_force_update] = "1"

    def is_fetch_if_not_exists_enabled(self):
        return os.environ.get(Settings._envname_fetch_if_not_exists, 0)

    def enable_fetch_if_not_exists(self, env):
        env[Settings._envname_fetch_if_not_exists] = "1"

    def get_session_id(self):
        return os.environ.get(Settings._envname_session_id, 'default')

    def set_session_id(self, session_id, env):
        env[Settings._envname_session_id] = session_id

    def get_arcadia_root(self, default):
        return os.environ.get(Settings._envname_arcadia_path, default)

    def set_arcadia_root(self, path, env):
        env[Settings._envname_arcadia_path] = path
