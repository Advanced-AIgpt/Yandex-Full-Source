import logging
import socket
import time

logger = logging.getLogger(__name__)


def wait_port(service_name, port, host='localhost', timeout_seconds=30, sleep_between_attempts_seconds=0.1,
              message_if_failure=None):
    start_time = time.time()
    logger.info(f'Waiting for {service_name} to start listening the port {port}...')
    while True:
        elapsed_seconds = time.time() - start_time
        if elapsed_seconds > timeout_seconds:
            if not message_if_failure:
                message_if_failure = f'Look for errors in the {service_name} log.'
            raise Exception(f'Service {service_name} seems to be not available on port {port}. {message_if_failure}')
        with socket.socket() as sock:
            try:
                sock.settimeout(timeout_seconds)
                sock.connect((host, port))
                logger.info(f'Service {service_name} listens the port {port}')
                break
            except (ConnectionRefusedError, socket.timeout):
                time.sleep(sleep_between_attempts_seconds)
