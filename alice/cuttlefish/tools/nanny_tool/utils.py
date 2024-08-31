import json
import logging
import difflib
import os
import subprocess
import tempfile
import asyncio


# -------------------------------------------------------------------------------------------------
def write_json(content, f=None, fname=None):
    def do(dst):
        json.dump(content, dst, indent=4, sort_keys=True)

    if f is not None:
        do(f)
    if fname is not None:
        with open(fname, "w") as f:
            do(f)
            logging.debug(f"Content was written to '{fname}' as JSON")


# -------------------------------------------------------------------------------------------------
def get_diff(old, new, color=False):
    with tempfile.TemporaryDirectory() as dname:
        old_fname = os.path.join(dname, "old.json")
        new_fname = os.path.join(dname, "new.json")
        write_json(old, fname=old_fname)
        write_json(new, fname=new_fname)
        args = ["diff"]
        if color:
            args.append("--color=always")
        args += [old_fname, new_fname]
        proc = subprocess.run(args, check=False, stdout=subprocess.PIPE, encoding="utf-8")
        return proc.stdout


# -------------------------------------------------------------------------------------------------
def confirm_diff(old, new, title="Patch"):
    diff = get_diff(old, new, color=True)
    color_print(title + ":", Colors.BG_BLUE)
    print(diff)
    resp = input("Is it OK? ")
    return resp.lower() in ("y", "yes", "true", "+", "1", "ok", "on")


# -------------------------------------------------------------------------------------------------
def print_diff(old, new):
    diff = get_diff(old, new, color=True)
    print(diff)


# -------------------------------------------------------------------------------------------------
def apply_patch(content, patch_file):
    with tempfile.TemporaryDirectory() as dname:
        orig_fname = os.path.join(dname, "orig.json")
        patched_fname = os.path.join(dname, "patched.json")
        with open(orig_fname, "w") as f:
            write_json(content, f)

        args = ["patch", "-i", patch_file, "-o", patched_fname, orig_fname]
        logging.debug(f"Call: {args}")
        subprocess.check_call(args)
        with open(patched_fname, "r") as f:
            return json.load(f)


# -------------------------------------------------------------------------------------------------
def make_patch(old, new, f):
    with tempfile.TemporaryDirectory() as dname:
        old_fname = os.path.join(dname, "old.json")
        new_fname = os.path.join(dname, "new.json")
        write_json(old, fname=old_fname)
        write_json(new, fname=new_fname)

        args = ["diff", old_fname, new_fname]
        proc = subprocess.run(args, check=False, stdout=subprocess.PIPE, encoding="utf-8")
        logging.debug(f"Patch:\n{proc.stdout}")
        f.write(proc.stdout)


# -------------------------------------------------------------------------------------------------
def vault_secrets(val):
    if not isinstance(val, dict):
        return

    for k, v in val.items():
        if isinstance(v, list):
            for item in v:
                for sec in vault_secrets(item):
                    yield sec
            continue

        if not isinstance(v, dict):
            continue

        if k == "vaultSecret" and ("delegationToken" in v) and ("secretId" in v) and ("secretVer" in v):
            if v.get("secretId"):
                yield v
            continue

        for sec in vault_secrets(v):
            yield sec

# -------------------------------------------------------------------------------------------------
def run_command(cmd, hosts, parallel_count=10):

    async def run(cmd, host):
        cmd = cmd.format(HOST=host)
        logging.debug(f"Run '{cmd}' for host '{host}'...")

        attempts = 4
        while True:
            proc = await asyncio.subprocess.create_subprocess_shell(cmd)
            rc = await proc.wait()
            if rc == 0:
                logging.info(f"Run for '{host}' completed")
                return True
            attempts -= 1
            if attempts == 0:
                logging.warning(f"Run for '{host}' failed, code={rc}")
                return False
            logging.debug(f"Run for '{host}' failed, code={rc}, retry...")

    async def run_in_parallel():
        tasks = set()
        i = 0
        return_when = asyncio.FIRST_COMPLETED
        failed = []
        while i < len(hosts):
            while len(tasks) < parallel_count:
                if i >= len(hosts):
                    return_when = asyncio.ALL_COMPLETED
                    break
                host = hosts[i]
                tasks.add(asyncio.create_task(run(cmd, host), name=host))
                i += 1

            done, tasks = await asyncio.wait(tasks, return_when=return_when)
            logging.debug(f"{len(done)} tasks done")
            for d in done:
                if not d.result():
                    failed.append(d.get_name())
        logging.info(f"All is done, failed={failed}")

    asyncio.run(run_in_parallel())


def run_via_ssh(cmd, hosts, parallel_count=10):
    cmd = "ssh -q -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null {HOST} '" + cmd + "'" 
    run_command(cmd, hosts, parallel_count=parallel_count)


# -------------------------------------------------------------------------------------------------
class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    BG_BLUE = '\033[104m'


def color_print(text, *flags):
    print(*flags, text, Colors.ENDC, sep="")

def print_green(text):
    color_print(text, Colors.GREEN)

def print_blue(text):
    color_print(text, Colors.BLUE)
