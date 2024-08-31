import requests
import re
from tensorboard_logger import Logger
import argparse
from flask import Flask, request
from os import path
import shutil
import subprocess
from multiprocessing import Process
from time import sleep, time

DATABASE_FNAME = "tensorboard.txt"
TB_FOLDER = None
app = Flask(__name__)

def parse_log(destination, data):
    destination = path.join(TB_FOLDER, destination)
    print(destination)
    shutil.rmtree(destination, ignore_errors=True)
    logger = Logger(destination)
    last_checkpoint = 0
    offset = 0
    iteration = 0
    train = []
    val = []
    for s in data.split("\n"):
        match = re.search("(\d+)\ttrain score: (\d+\.\d+)", s)
        if match:
            num = int(match.group(1))
            if num < iteration:
                offset += last_checkpoint
                last_checkpoint = 0
                train.clear()
                val.clear()
            iteration = num
            train.append((float(match.group(2)), offset + iteration))
        match = re.search(r"validation score ?: (\d+\.\d+)", s)
        if match:
            val.append((float(match.group(1)), offset + iteration))
        match = re.search("Model saved", s)
        if match:
            last_checkpoint = iteration
            for el, it in train:
                logger.log_value("train", el, it)
            for el, it in val:
                logger.log_value("val", el, it)
            train.clear()
            val.clear()
    for el, it in train:
        logger.log_value("train", el, it)
    for el, it in val:
        logger.log_value("val", el, it)


@app.route("/upload/<run>", methods=["POST"])
def upload(run):
    data = request.get_data(as_text=True)
    parse_log(run, data)
    return ""

@app.route("/register", methods=["GET", "POST"])
def register():
    if request.method == "POST":
        with open(DATABASE_FNAME, "a") as f:
            f.write("{} {}\n".format(request.form["run"], request.form["data"]))
            data = requests.get(request.form["data"], verify=False).text
            parse_log(request.form["run"], data)
    return '''
<form action="/register" method="POST">
Run name: <input type="text" name="run"/>
<br/>
Stdout download link: <input type="text" name="data"/>
<br/>
<input type="submit"/>
</form>
'''

def update_logs(args):
    while True:
        with open(DATABASE_FNAME) as f:
            for line in f:
                run, link = line.split()
                response = requests.get(link, verify=False)
                if response.ok:
                    parse_log(run, response.text)
        tb_process = subprocess.Popen([args.tb_binary, "--logdir", args.tb_folder])
        sleep(args.update_interval * 60)
        tb_process.terminate()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--tb-folder', required=True, help='Path to tensorboard log folder')
    parser.add_argument('--tb-binary', required=True, help='Path to tensorboard binary')
    parser.add_argument('--update-interval', type=int, required=True, help='Update interval in minutes')
    args = parser.parse_args()
    global TB_FOLDER
    TB_FOLDER = args.tb_folder
    process = Process(target=update_logs, args=(args,))
    process.start()
    app.run(host='0.0.0.0', port=5757, threaded=True)
    process.join()

main()
