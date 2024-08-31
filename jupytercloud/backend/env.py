import os
from enum import Enum, auto


class JupyterCloudEnvironment(Enum):
    testing = auto()
    production = auto()
    devel = auto()

    @classmethod
    def get_by_name(cls, name):
        result = getattr(cls, name)
        assert isinstance(result, cls)
        return result

    @classmethod
    def get_from_env(cls):
        env_name = os.environ.get('JUPYTER_CLOUD_ENV', 'devel').lower()
        return cls.get_by_name(env_name)
