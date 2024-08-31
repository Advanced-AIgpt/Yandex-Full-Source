import os
import psutil
import yatest.common
from yatest.common.network import PortManager
from library.python.testing.recipe import declare_recipe, set_env


def start(argv):
    pm = PortManager()
    port = pm.get_port_range(None, 2)
    with open("recipe.port", "w") as f:
        f.write(str(port))
    set_env("RECIPE_PORT", str(port + 1))
    os.environ["ARCADIA_ROOT"] = yatest.common.source_path()

    pid = os.fork()
    if pid == 0:
        yatest.common.execute(
            [
                yatest.common.binary_path("alice/hollywood/scripts/run/run-hollywood-bin"),
                "-c", yatest.common.source_path("alice/hollywood/shards/all/dev/hollywood.pb.txt") ,
                "--app-host-config-port", str(port)
            ],
            check_exit_code=True
        )
    else:
        with open("recipe.pid", "w") as f:
            f.write(str(pid))


def stop(argv):
    with open("recipe.pid") as f:
        pid = int(f.read())
#        os.kill(pid, 9)
        p = psutil.Process(pid)
        p.terminate()

if __name__ == "__main__":
    declare_recipe(start, stop)
