import os
import asyncio
import logging


# -------------------------------------------------------------------------------------------------
class RemoteCmd:
    def __init__(self, host, cmd):
        self.host = host
        self.cmd = f"ssh {host} '{cmd}'"
        self.proc = None

    async def launch(self, stdin_data=None):
        logging.debug(f"Run command on '{self.host}'...")

        self.proc = await asyncio.create_subprocess_shell(
            self.cmd,
            stdin=asyncio.subprocess.DEVNULL if stdin_data is None else asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE
        )

        if stdin_data is not None:
            logging.debug(f"Send {len(stdin_data)} bytes onto '{self.host}'...")
            self.proc.stdin.write(stdin_data)
            self.proc.stdin.write_eof()

    async def read_all_lines(self, receiver_callback=None):
        if self.proc is None:
            raise RuntimeError("Process not started")

        try:
            await self.proc.stdin.wait_closed()
            logging.debug(f"All data has been sent to {self.host}")

            count = 0
            while True:
                line = await self.proc.stdout.readline()
                if not line:
                    break
                receiver_callback(line.decode("utf-8").strip())
                count += 1
        except asyncio.CancelledError as err:
            logging.warning(f"Process on {self.host} is cancelled")
            self.proc.kill()
        except Exception as err:
            logging.warning(f"Exception in process on '{self.host}': {err}")
            self.proc.kill()
        finally:
            ret = await self.proc.wait()
            if ret != 0:
                logging.warning(f"Command on '{self.host}' failed: code={ret}")
            else:
                logging.debug(f"Command on '{self.host}' completed, got {count} lines")


# ------------------------------------------------------------------------------------------------
async def collect_logs(
    hosts,
    parser_bin,
    timeout=None,
    path="/logs/current-access_log-balancer-443",
    begin_ts=None,
    end_ts=None,
    uri=None,
    max=None
):
    if not os.path.exists(parser_bin):
        raise RuntimeError(f"Parser's binary {parser_bin} doesn't exist")

    opts = f"--path {path} "
    if begin_ts is not None:
        opts += f"--begin-ts {int(begin_ts)} "
    if end_ts is not None:
        opts += f"--end-ts {int(end_ts)} "
    if uri is not None:
        opts += f"--uri {uri} "
    if max is not None:
        opts += f"--max {max} "

    bin_name = os.path.basename(parser_bin)
    remote_cmd = f"""
        cat - > ./{bin_name}
        chmod 775 ./{bin_name}
        su loadbase -c "./{bin_name} --path /logs/current-access_log-balancer-443 {opts}"
    """
    logging.debug(f"Run next command on {len(hosts)} hosts:\n{remote_cmd}")

    with open(parser_bin, "rb") as f:
        data = f.read()

    records = []

    # def receive(line):
    #     records.append(line)

    aws = []
    for host in hosts:
        proc = RemoteCmd(host, remote_cmd)
        await proc.launch(stdin_data=data)
        aws.append(proc.read_all_lines(records.append))

    logging.info("All processes are started")

    done, pending = await asyncio.wait(aws, timeout=timeout, return_when=asyncio.ALL_COMPLETED)

    for d in done:
        try:
            await d
        except:
            pass
    logging.info(f"{len(done)} processes are done")

    if pending:
        logging.debug(f"Cancel {len(pending)} processes...")
        for p in pending:
            p.cancel()
            try:
                await p
            except:
                pass
        logging.info(f"{len(pending)} processes were cancelled")

    return records
