from alice.uniproxy.library.settings import config


class ConfigPatch:
    @staticmethod
    def config_patches(patch):
        for path, val in patch.items():
            if isinstance(val, dict):
                for child_path, child_val in ConfigPatch.config_patches(val):
                    yield path + "." + child_path, child_val
            else:
                yield path, val

    @staticmethod
    def swap_config_value(path, new_value):
        old_value = config
        for i in path.split("."):
            old_value = old_value.get(i)
            if old_value is None:
                break
        config.set_by_path(path, new_value)
        return old_value

    def __enter__(self):
        self.apply()
        return self

    def __exit__(self, *args, **kwargs):
        self.restore()

    def __init__(self, patch={}):
        self.patch = patch
        self.__old_values = None

    def apply(self):
        if self.__old_values is not None:
            raise SystemError("Config patch already applied")
        self.__old_values = {}
        for path, val in ConfigPatch.config_patches(self.patch):
            old_value = self.swap_config_value(path, val)
            self.__old_values[path] = old_value

    def restore(self):
        if self.__old_values is None:
            raise SystemError("Config patch wasn't applied")
        for path, val in self.__old_values.items():
            config.set_by_path(path, val)
