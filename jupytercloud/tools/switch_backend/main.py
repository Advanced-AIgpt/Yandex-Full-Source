import time
import click
from redis.sentinel import Sentinel

from library.python.vault_client.instances import Production as ProdYavClient


def sleep(t):
    with click.progressbar(length=t) as bar:
        for i in range(t):
            time.sleep(1)
            bar.update(1)


ENV = {
    "production": {
        "backend_semaphore": "prod/backend/semaphore",
        "redis_sentinels": [
            'man-e4n32zle4p444a1o.db.yandex.net',
            'sas-zykvti0f3q1mml0x.db.yandex.net',
            'vla-za5kqljlvd9cm1yd.db.yandex.net'
        ],
        "redis_password": "sec-01dhkemwckfe8tc5vbk1tps2yq"
    },
    "testing": {
        "backend_semaphore": "test/backend/semaphore",
        "redis_sentinels": [
            'man-e4n32zle4p444a1o.db.yandex.net',
            'sas-zykvti0f3q1mml0x.db.yandex.net',
            'vla-za5kqljlvd9cm1yd.db.yandex.net'
        ],
        "redis_password": "sec-01dh6emwya97r6z2w8pc88m7a2"
    }
}


@click.command("switch_backend")
@click.option("-w", "--wait", default=15, show_default=True)
@click.option(
    "--environment",
    "--env",
    "-e",
    "enviro",
    type=click.Choice(["production", "testing"]),
    default="testing",
)
@click.option("--dc", type=click.Choice(["iva", "myt", "vla", "sas", "man"], case_sensitive=False))
def switch_backend(wait, enviro, dc):
    env = ENV[enviro]

    yav_client = ProdYavClient()
    redis_password = yav_client.get_version(env["redis_password"])["value"]["redis_password"]

    sentinel = Sentinel([(ip, 26379) for ip in env["redis_sentinels"]])
    master = sentinel.master_for("jupyter_test_redis", password=redis_password)

    try:
        current_backend = master.get(env["backend_semaphore"]).decode("utf-8")
    except AttributeError:
        current_backend = "<EMPTY>"

    if enviro == "production":
        pretty_env = click.style(enviro, bold=True, fg="red")
    else:
        pretty_env = click.style(enviro, bold=True)
    click.echo(f"Current DC for {pretty_env}: " + click.style(current_backend, bold=True))

    if dc is None:
        return
    if dc == current_backend:
        click.echo("Not switching DC, restarting.")

    pretty_dc = click.style(dc, bold=True)
    confirm = click.confirm(f"Switch backend DC to {pretty_dc}?")
    if not confirm:
        return

    click.echo("Turning off old master")
    master.set(env["backend_semaphore"], "")
    sleep(wait)

    click.echo("Turning on new master")
    master.set(env["backend_semaphore"], dc)
    sleep(wait)

    new_backend = master.get(env["backend_semaphore"]).decode("utf-8")
    pretty_backend = click.style(new_backend, bold=True)
    click.echo(f"Success! New master is {pretty_backend}")


def main():
    switch_backend()
