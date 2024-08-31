import datetime
import json
import subprocess
import uuid

import click
import yt.wrapper


@click.command()
@click.option('--status-updater')
@click.option('--censored-tables')
@click.option('--yt-token')
def main(status_updater, censored_tables, yt_token):
    yt.wrapper.config['token'] = yt_token
    yt.wrapper.config.set_proxy('hahn')
    ts = datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S.%fZ')
    data = {'data': []}

    for user in yt.wrapper.read_table(censored_tables):
        if user['completed']:
            data['data'].append({'puid': user['puid'], 'service': 'Logs', 'ts': ts})

    filename = '/tmp/private_users_' + str(uuid.uuid4())
    with open(filename, 'w') as f:
        f.write(json.dumps(data))
    print(data)
    process = subprocess.run(
        [status_updater,
         '--single-input', filename,
         '--status', 'empty'])
    assert process.returncode == 0


if __name__ == '__main__':
    main()
