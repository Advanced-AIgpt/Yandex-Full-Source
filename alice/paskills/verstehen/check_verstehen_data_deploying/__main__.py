import argparse
import logging
import os
import time

from infra.dctl.src.lib.yp_client import YpClient as DctlYpClient
import infra.dctl.src.lib.stage as stage_api
import yp.data_model as data_model

logging.basicConfig()
logger = logging.getLogger(__name__)

DEFAULT_YP_URL = "xdc.yp.yandex.net:8090"
YP_URL_TEMPLATE = "{dc_name}.yp.yandex.net:8090"


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--stage-id', required=True)
    parser.add_argument('--deploy-unit-id', required=True)
    parser.add_argument('--layer-id', required=True)
    parser.add_argument('--task-id', required=True)
    parser.add_argument('--attempt-count', required=True)
    parser.add_argument('--attempt-timeout', required=True)

    return parser.parse_args()


def is_resource_released(stage_id, client, deploy_unit_id, layer_id, task_id):
    stage = stage_api.get(stage_id, client)
    if deploy_unit_id not in stage.status.deploy_units:
        raise Exception("There's no deploy unit {}", deploy_unit_id)

    layers = stage.status.deploy_units[deploy_unit_id].current_target.replica_set.replica_set_template.pod_template_spec.spec.pod_agent_payload.spec.resources.layers
    for layer in layers:
        if layer.id == layer_id:
            return layer.meta.sandbox_resource.task_id == task_id

    raise Exception("There's no layer {} in deploy unit {}", layer_id, deploy_unit_id)


def is_resource_deployed(stage_id, client, deploy_unit_id):
    stage = stage_api.get(stage_id, client)
    if deploy_unit_id not in stage.status.deploy_units:
        raise Exception("There's no deploy unit {}", deploy_unit_id)
    return stage.status.deploy_units[deploy_unit_id].ready.status == data_model.CS_TRUE


def main():
    args = parse_args()
    stage_id = args.stage_id
    deploy_unit_id = args.deploy_unit_id
    layer_id = args.layer_id
    task_id = args.task_id

    token = os.environ['YP_TOKEN']
    if token is None:
        logger.error("$YP_TOKEN environment variable is not set")
        exit(1)
    client = DctlYpClient(url=DEFAULT_YP_URL, token=token)

    released = is_resource_released(stage_id, client, deploy_unit_id, layer_id, task_id)
    attempts, max_attempts = 1, 120
    while not released and attempts < max_attempts:
        released = is_resource_released(stage_id, client, deploy_unit_id, layer_id, task_id)
        attempts += 1
        time.sleep(60)

    if not released:
        raise Exception("Resource was not released after {} attempts", attempts)

    deployed = is_resource_deployed(stage_id, client, deploy_unit_id)
    attempts, max_attempts = 1, 120
    while not deployed and attempts < max_attempts:
        deployed = is_resource_deployed(stage_id, client, deploy_unit_id)
        attempts += 1
        time.sleep(60)

    if not deployed:
        raise Exception("Resource was not deployed after {} attempts", attempts)


if __name__ == '__main__':
    main()
