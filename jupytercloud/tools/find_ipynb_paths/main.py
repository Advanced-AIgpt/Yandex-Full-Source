import csv

import click

from jupytercloud.tools.lib import parallel, utils, JupyterCloud, environment
from jupytercloud.backend.lib.clients import ssh

logger = None


def convert_result(data):
    result = []

    for user, filenames in data.iteritems():
        if not isinstance(filenames, str):
            continue
        for line in filenames.splitlines():
            line = line.strip()
            if not line:
                continue

            result.append(
                {"username": user, "path": line,}
            )

    result.sort()
    return result


@click.command('find_notebooks')
@click.option("--output", type=click.File("wb", lazy=True), default="result.csv")
@click.option("--env", default="production")
@click.option("--threads", "-j", type=int, default=None)
@click.option("--verbose", "-v", count=True, default=0)
def find_notebooks(output, env, threads, verbose):
    global logger
    logger = utils.setup_logging(__name__, verbose)

    with environment.environment(env):
        jupyter_cloud = JupyterCloud()
        qyp_vms = jupyter_cloud.get_qyp_vms()

        logger.info("going to analyze %d vms", len(qyp_vms))

        def _process_one(user):
            vm = qyp_vms[user]
            host = vm["host"]
            logger.info("processing %s", host)

            status_name = jupyter_cloud.get_vm_status(vm["cluster"], vm["id"])

            if status_name != "RUNNING":
                return

            try:
                with ssh.SSHClient(host=host) as client:
                    result = client.execute(
                        r'find /home -xdev -type f \( ! -path "*/.*" -iname "*.ipynb" \) '
                        r"-print 2>/dev/null "
                        r"|| true",
                        300,
                    )

                    return result.stdout
            except Exception:
                logger.exception("error while grepping host %s", host)
                return

        result = parallel.process_with_progress(_process_one, qyp_vms, threads)

    result = convert_result(result)

    writer = csv.DictWriter(output, ["username", "path"])
    writer.writeheader()
    writer.writerows(result)

    click.echo("ok")


def main():
    find_notebooks()
