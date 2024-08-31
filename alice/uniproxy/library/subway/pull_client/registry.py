from alice.uniproxy.library.subway.common import UnisystemRegistry


g_registry = UnisystemRegistry()


def registry_instance():
    global g_registry
    return g_registry
