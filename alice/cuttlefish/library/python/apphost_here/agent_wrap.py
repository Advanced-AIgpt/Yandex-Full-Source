from .utils import read_json, write_json, remove_dir, Process
from .apphost_utils import Graph
import os
import logging


class HorizonAgentWrap:
    DEFAULT_CONFIGURATION = "ctype=prod;geo=sas"

    @classmethod
    def set_backend_property(*backends, horizon_data_dir, name, value):
        for b in backends:
            p = os.path.join(horizon_data_dir, f"{b}.json")
            cfg = read_json(p)
            cfg[name] = value
            write_json(cfg, p)

    def __init__(self, agent_bin_path, local_arcadia_path):
        self.logger = logging.getLogger("HorizonAgentWrap")
        self.agent_bin_path = agent_bin_path
        self.local_arcadia_path = local_arcadia_path

    def copy_graphs(self, vertical, workdir="./"):
        for g in Graph.list_graphs(os.path.join(self.local_arcadia_path, "apphost/conf/verticals", vertical)):
            dst_dir = os.path.join(workdir, "horizon-data/graphs")
            self.logger.debug(f"Copy graph {g.name} to {dst_dir}")
            g.save(dir=dst_dir)

    async def generate_local(
        self, patch_path, vertical, configuration=DEFAULT_CONFIGURATION, workdir="./", timeout=120
    ):
        args = [
            self.agent_bin_path,
            "--local-arcadia-path",
            self.local_arcadia_path,
            "--vertical",
            vertical,
            "--configuration",
            configuration,
            "--patch-file-path",
            patch_path,
            "run",
            "local-watch",
        ]

        output_dir = os.path.join(workdir, "horizon-data")
        remove_dir(output_dir)

        async with Process(
            args=args,
            workdir=workdir,
            stdout=Process.FILE(os.path.join(workdir, "agent.out")),
            stderr=Process.FILE(os.path.join(workdir, "agent.err")),
            logger=self.logger,
        ) as proc:
            await proc.wait(timeout=timeout)

        if not os.path.isdir(output_dir):
            raise RuntimeError(f"agent couldn't create data (output_dir): {output_dir}")
