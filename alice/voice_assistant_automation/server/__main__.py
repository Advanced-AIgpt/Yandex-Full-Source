# -*- coding: utf-8 -*-
import argparse
import flask
import os
import subprocess
import threading
import time

RUNNING = False
PROCESS = None
SHUTDOWN = False
LOCK = threading.Lock()
LOG_FILE_NAME = "{}.log".format(time.time())
SCRIPT_LOG = open(LOG_FILE_NAME, "w+")

def shutdown_server():
    func = flask.request.environ.get("werkzeug.server.shutdown")
    if func is None:
        raise RuntimeError("Not running with the Werkzeug Server")
    func()


def run_server_thread(port, script_path):
    def _run():
        app = flask.Flask(__name__)

        @app.route("/run")
        def route_run():
            global LOCK
            global RUNNING
            global PROCESS

            with LOCK:
                if RUNNING:
                    return flask.Response("Script is already running")
                RUNNING = True

                try:
                    SCRIPT_LOG.truncate()
                    SCRIPT_LOG.seek(0)
                    PROCESS = subprocess.Popen("bash {}".format(script_path), shell=True, stdout=SCRIPT_LOG, stderr=SCRIPT_LOG, env=os.environ)
                    return flask.Response("")
                except:
                    RUNNING = False
                    PROCESS = None
                    raise

        @app.route("/status")
        def route_status():
            global LOCK
            global RUNNING
            global PROCESS

            with LOCK:
                status = ""
                try:
                    with open(LOG_FILE_NAME, "r") as f:
                        status = f.read()
                except:
                    pass

                if RUNNING:
                    return flask.Response("Status of current run:\n{}".format(status))
                else:
                    return flask.Response("Nothing is running\n{}".format(status))

        @app.route("/ping")
        def route_ping():
            return flask.Response("pong")

        @app.route("/shutdown")
        def route_shutdown():
            global LOCK
            global RUNNING
            global PROCESS
            global SHUTDOWN

            with LOCK:
                shutdown_server()
                if RUNNING:
                    PROCESS.wait()
                SHUTDOWN = True
                return flask.Response("")
        app.run(host="::", port=port)

    thread = threading.Thread(name="http_server", target=_run)
    thread.setDaemon(True)
    thread.start()


def main(arguments):
    run_server_thread(arguments.port, arguments.script)

    global LOCK
    global RUNNING
    global PROCESS
    global SHUTDOWN

    while True:
        with LOCK:
            if SHUTDOWN:
                break
            if RUNNING and PROCESS.poll() is not None:
                RUNNING = False
                PROCESS = None

        time.sleep(1)


def parse_arguments():
    parser = argparse.ArgumentParser(description="YP eviction alerter")
    parser.add_argument("script", help="Path to script")
    parser.add_argument("--port", required=True, help="Http server port")

    return parser.parse_args()


if __name__ == "__main__":
    main(parse_arguments())
