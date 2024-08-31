from alice.notification_creator.lib.notification import series_notification, music_notification, podcasts_notification, handcrafted_kinopoisk_notification

import yt.wrapper as yt

from flask import Flask, Response, request, current_app
import requests
import urllib


allowed_notification = {
    'music': music_notification,
    'series': series_notification,
    'podcasts': podcasts_notification,
    'handcrafted_kinopoisk': handcrafted_kinopoisk_notification
}


def send_notification(msg):
    url = urllib.parse.urljoin('https://' + current_app.app_config["notifier_host"], '/delivery')
    for i in range(3):
        try:
            r = requests.post(url, data=msg.SerializeToString())
            resp = r.json()
            if resp['code'] // 100 == 2 or resp['code'] // 100 == 4:
                return 'ok, status code = {}, attempt = {}'.format(r.status_code, i + 1)
        except Exception:
            current_app.logger.info('attempt = {} was failed'.format(i + 1))


def register_handlers(app):
    @app.route('/subscriptions/user_list', methods=['GET'])
    def user_list():
        url = urllib.parse.urljoin('https://' + app.app_config["notifier_host"], 'subscriptions/user_list')
        headers = {k: v for k, v in dict(request.headers).items() if k != 'Host'}
        headers['Host'] = app.app_config["notifier_host"]
        params=dict(request.args)

        r = requests.get(
            url,
            headers=headers,
            params=params
        )

        app.logger.info(f'proxy user_list to {r.url}')

        resp = Response(
            r.content,
            status=r.status_code,
            headers={k: v for k, v in dict(r.headers).items() if k != 'Content-Encoding'}   # Костыль, т.к. requests распаковывает gzip
        )
        return resp

    @app.route('/process_push/<string:push_type>/<string:table_name>', methods=['POST'])
    def process_push(push_type, table_name):
        if 'Authorization' not in request.headers:
            return {
                'code': 400,
                'error': 'need Authorization token'
            }
        if push_type not in allowed_notification.keys():
            return {
                'code': 400,
                'error': f'push type must be in {allowed_notification.keys()}'
            }

        token = request.headers.get('Authorization')
        client = yt.YtClient(
            proxy=app.app_config['proxy'],
            token=token if 'OAuth ' not in token else token.split('OAuth ')[1]
        )

        yt_dir = app.app_config['yt_dirs'][push_type]
        if not client.exists(yt_dir):
            app.logger.error(f'yt directory {yt_dir} does not exist')

        nodes = client.list(
            path=yt_dir,
            absolute=False,
            attributes=['type']
        )
        tables = [str(node) for node in nodes if node.attributes['type'] == 'table']
        if table_name not in tables:
            return {
                'code': 404,
                'error': f'table {table_name} not found in YT directory {yt_dir}'
            }

        table = yt.ypath_join(yt_dir, table_name)
        app.logger.info(f'start {table}')
        with client.Transaction():
            client.lock(
                table,
                waitable=True,
                mode='exclusive'
            )

            res_rows = []
            for i, row in enumerate(client.read_table(table)):
                app.logger.info(f'building {push_type} notification for {row["puid"]}')
                res_row, msg = allowed_notification[push_type](row)
                if not msg:
                    app.logger.info(f'empty message for {row["puid"]}, skip')
                    continue
                status = send_notification(msg)
                res_row['result'] = status
                app.logger.info(f'status for {row["puid"]}: {status}')
                res_rows.append(res_row)

            res_dir = app.app_config['yt_dirs'][push_type + '_result']
            res_table = request.args.get('destination') if 'destination' in request.args else yt.ypath_join(res_dir, table_name)

            app.logger.info(f'write result for {table} to {res_table}')
            client.write_table(res_table, res_rows, format='json')

            app.logger.info(f'remove {table}')
            client.remove(table)

        return {
            'code': 200,
            'push_type': push_type,
            'result_table': res_table
        }


def create_app(config, *args, **kwargs):
    app = Flask(*args, **kwargs)
    app.app_config = config
    register_handlers(app)
    return app
