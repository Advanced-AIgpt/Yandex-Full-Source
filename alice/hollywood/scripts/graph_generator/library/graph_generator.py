import os
import errno
import json
import logging
from jinja2 import Template
from io import TextIOWrapper

from apphost.lib.python_util.conf import (
    iterate_all_nodes_in_dir,
    node_backend_name,
    node_type,
)


logger = logging.getLogger(__name__)


def generate_graph(jinja2_template_str, context_dict):
    template = Template(jinja2_template_str)
    json_graph_str = template.render(**context_dict)
    _validated_json_loads(
        json_graph_str, 'Generated json graph is invalid, reason: {}'
    )
    return json_graph_str


def generate_graph_fs(
    jinja2_template_file,
    jinja2_context_file,
    jinja2_context_override_dict,
    out_json_graph_file,
):
    logger.info('Generating graph from template {}'.format(
        jinja2_template_file)
    )
    with open(jinja2_template_file, 'r') as f:
        jinja2_template_str = f.read()

    with open(jinja2_context_file, 'r') as f:
        jinja2_context_str = f.read()
    context_dict = json.loads(jinja2_context_str)

    for key, val in jinja2_context_override_dict.items():
        context_dict[key] = val

    json_graph_str = generate_graph(jinja2_template_str, context_dict)

    out_json_graph_dir = os.path.split(out_json_graph_file)[0]
    _mkdirs(out_json_graph_dir)

    with open(out_json_graph_file, 'w') as f:
        f.write(json_graph_str)


def _generate_backend(
    dev_null_port,
):
    """
    Generates a single backend.
    """
    return \
        {
            'instances': [
                {
                    'ip': '[::1]',
                    'host': 'localhost',
                    'port': dev_null_port,
                    'weight': 1,
                    'min_weight': 1
                }
            ]
        }


def _generate_backends_for_graphs(
    graphs_dir,
    dev_null_port,
):
    """
    Generates backends for graphs located in the given directory recursively.
    """
    backends = dict()

    for node_name, node, _ in iterate_all_nodes_in_dir(graphs_dir):
        if node_type(node) != 'DEFAULT':
            continue
        backend_name = node_backend_name(node, node_name) + '.json'
        backends[backend_name] = _generate_backend(dev_null_port)

    return backends


def generate_backends_patch(
    graphs_dir,
    local_app_host_dir,
    dev_null_port,
):
    """
    Generates a patch file for apphost backends.
    """

    # Generate the backends.
    backends = _generate_backends_for_graphs(
        graphs_dir, dev_null_port
    )

    # Create the path to the patch file.
    patch_path = os.path.join(local_app_host_dir, 'ALICE')
    _mkdirs(patch_path)
    patch_path = os.path.join(patch_path, 'backends_patch.json')

    # Dump the backends to the file.
    with open(patch_path, 'w') as patch_file:
        json.dump({'backends': backends}, patch_file, indent=4, sort_keys=True)


def _validated_json_loads(source, message):
    """
    Performs json.loads(source) and checks for errors.
    """
    try:
        if type(source) is TextIOWrapper:
            return json.load(source)
        else:
            return json.loads(source)
    except ValueError as err:
        logger.error(message.format(err))
        raise


def _mkdirs(path):
    if not os.path.exists(path):
        try:
            os.makedirs(path)
        except OSError as err:
            if err.errno != errno.EEXIST:
                raise
