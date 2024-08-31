from tensorboardX import SummaryWriter
import argparse
from flask import Flask, request
from os import path

TB_FOLDER = None
WRITERS = {}
STEPS = {}
app = Flask(__name__)


@app.route('/post', methods=['POST'])
def post():
    args = request.args
    name = args['name']
    step = int(args['step'])
    if name not in STEPS or step < STEPS[name]:
        WRITERS[name] = SummaryWriter(path.join(TB_FOLDER, name), purge_step=step)
    STEPS[name] = step
    writer = WRITERS[name]
    value = args['value']
    _type = args['type']
    if _type == 'add_scalar':
        value = float(value)
    getattr(writer, _type)(args['tag'], value, int(args['step']))
    return ''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--tb-folder', required=True, help='Path to tensorboard log folder')
    parser.add_argument('--port', default=5756)
    args = parser.parse_args()
    global TB_FOLDER
    TB_FOLDER = args.tb_folder
    app.run(host='::', port=args.port, threaded=True)

main()
