import pytest
import asyncio
import os
import json
import filecmp
import logging
import shutil
from copy import deepcopy
import yatest.common

from alice.cuttlefish.library.python.apphost_here.agent_wrap import HorizonAgentWrap
from alice.cuttlefish.library.python.apphost_here import BackendPatcher
from alice.cuttlefish.library.python.apphost_here.utils import read_json, write_json


CONFIGURATION = "ctype=testing;geo=sas"
VERTICAL = "VOICE"
CONF_DIR = "apphost/conf"
AGENT_BIN_PATH = "apphost/daemons/horizon/agent/horizon-agent"
THIS_DIRECTORY = os.path.dirname(__file__)
PRESERVED_HORIZON_DATA_DIR = os.path.join(THIS_DIRECTORY, "horizon-data")


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="session")
def arcadia_path():
    return yatest.common.source_path("")


@pytest.fixture(scope="session")
def output_dir():
    output_dir = yatest.common.output_path("apphost")
    os.makedirs(output_dir)
    return output_dir


@pytest.fixture(scope="session")
def agent(arcadia_path):
    return HorizonAgentWrap(agent_bin_path=yatest.common.build_path(AGENT_BIN_PATH), local_arcadia_path=arcadia_path)


@pytest.fixture(scope="session")
def patcher(arcadia_path):
    return BackendPatcher(vertical=VERTICAL, configuration=CONFIGURATION, conf_dir=os.path.join(arcadia_path, CONF_DIR))


def sort_all_lists(content):
    if isinstance(content, list):
        if len(content) == 0:
            return

        for i in content:
            sort_all_lists(i)

        key = None
        if isinstance(content[0], (dict, list)):
            key = lambda x: json.dumps(x, sort_keys=True)

        content.sort(key=key)
        return

    if isinstance(content, dict):
        for val in content.values():
            sort_all_lists(val)
        return

    return


def sort_all_jsons(path):
    logging.debug(f"Sort content of JSONs in '{path}'...")
    for fname in os.listdir(path):
        fpath = os.path.join(path, fname)
        if os.path.isdir(fpath):
            sort_all_jsons(fpath)
            continue
        if os.path.splitext(fpath)[1] != ".json":
            logging.debug(f"Skip '{fpath}' for it's not a JSON")
            continue

        logging.debug(f"Sort content of '{fpath}'...")
        orig_content = read_json(fpath)

        sorted_content = deepcopy(orig_content)
        sort_all_lists(sorted_content)

        if sorted_content != orig_content:
            logging.debug(f"Content of {fpath} was sorted")

        write_json(sorted_content, fpath, sort_keys=True)


# -------------------------------------------------------------------------------------------------
async def build_apphost_data(patcher, agent, output_dir):
    patch_path = os.path.join(output_dir, "backends_patch.json")

    patch = patcher.make_backends_patch(
        overrides={  # force all backends to `localhost:1` endpoint
            backend.name: ("localhost", 1) for backend in patcher.backends
        }
    )
    with open(patch_path, "w") as f:
        json.dump(patch, f, indent=4)

    await agent.generate_local(
        patch_path=patch_path, vertical=VERTICAL, configuration=CONFIGURATION, workdir=output_dir, timeout=300
    )

    horizon_data_dir = os.path.join(output_dir, "horizon-data")
    sort_all_jsons(horizon_data_dir)
    return horizon_data_dir


def compare_dirs(expected, actual):
    logging.debug(f"Compare '{expected}' and '{actual}'...")
    dcmp = filecmp.dircmp(expected, actual, ignore=["README"])

    if dcmp.left_only:
        assert False, f"Items {dcmp.left_only} do not exist in actual data"
    if dcmp.right_only:
        assert False, f"Items {dcmp.right_only} do not exist in expected data"
    if dcmp.funny_files:
        assert False, f"Items {dcmp.funny_files} can not be compared"
    if dcmp.diff_files:
        assert False, f"Files {dcmp.diff_files} differ"

    for subdir in dcmp.common_dirs:
        compare_dirs(os.path.join(expected, subdir), os.path.join(actual, subdir))


# -------------------------------------------------------------------------------------------------
def test_all_needed_backends(patcher):
    for be in patcher.backends:
        for loc in patcher.settings.required_locations:
            assert be.has_configuration(loc)


def test_all_required_graphs(patcher):
    for g in patcher.settings.required_graphs:
        assert g in patcher.graphs
        logging.debug(f"Required graph '{g}' exists")


def test_horizon_data(patcher, agent, output_dir):
    dst_dir = yatest.common.get_param("preserve-dir", "")

    preserved_horizon_data = yatest.common.source_path(PRESERVED_HORIZON_DATA_DIR)

    if not os.path.isdir(preserved_horizon_data):
        if not dst_dir:
            assert False, (
                f"Preserved data doesn't exists ({PRESERVED_HORIZON_DATA_DIR}). "
                "Probably you need to run `save.sh` script first"
            )

    try:
        actual_horizon_data = asyncio.run(build_apphost_data(patcher, agent, output_dir))
    except json.JSONDecodeError as ex:
        logging.error(f"Error while decoding JSON: doc={ex.doc} line={ex.lineno} col={ex.colno}")
        raise

    logging.debug(f"Actual horizon-data is ready ({actual_horizon_data})")

    try:
        compare_dirs(expected=preserved_horizon_data, actual=actual_horizon_data)
    except AssertionError:
        if not dst_dir:
            logging.exception("horizon-data changed")
            raise RuntimeError(
                "\n"
                "+------------------------------------------------------------------------------+\n"
                "| This exception doesn't mean that something is really broken. It happens      |\n"
                "| because VOICE graphs or related backends were changed. Hence the vertical's  |\n"
                "| tests must be run.                                                           |\n"
                "|                                                                              |\n"
                "| Run `alice/cuttlefish/tests/apphost/save.sh` script and add changes occurred |\n"
                "| in `alice/cuttlefish/tests/apphost/horizon-data` into your commit. It'll     |\n"
                "| preserve current output of `horizon_agent` and trigger our tests in CI.      |\n"
                "+------------------------------------------------------------------------------+\n"
                "\n"
            )

        logging.info(f"Preserve actual horizon-data ({dst_dir})...")
        if os.path.isdir(dst_dir):
            shutil.rmtree(dst_dir)
        shutil.copytree(src=actual_horizon_data, dst=dst_dir)
