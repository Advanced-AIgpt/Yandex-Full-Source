"""Database utilities for JupyterCloud."""
# Based on jupyterhub.dbutils, used under the BSD license.

import argparse
import logging
import os
import shutil
import sys
import time
from contextlib import contextmanager
from datetime import datetime
from subprocess import check_call
from tempfile import TemporaryDirectory
from urllib.parse import urlparse

import pkg_resources
from sqlalchemy import create_engine, event
from sqlalchemy.engine import Engine

from . import orm


_here = os.path.abspath(os.path.dirname(__file__))

ALEMBIC_INI_TEMPLATE_PATH = 'alembic.ini'
ALEMBIC_DIR = os.path.join(_here, 'alembic')


def write_alembic_ini(alembic_ini='alembic.ini', db_url='sqlite:///jupytercloud.sqlite'):
    """Write a complete alembic.ini from our template.

    Parameters
    ----------
    alembic_ini: str
        path to the alembic.ini file that should be written.
    db_url: str
        The SQLAlchemy database url, e.g. `sqlite:///jupyterhub.sqlite`.

    """
    try:
        with open(ALEMBIC_INI_TEMPLATE_PATH) as f:
            alembic_ini_tpl = f.read()
    except OSError:
        alembic_ini_tpl = pkg_resources.resource_string(__package__, ALEMBIC_INI_TEMPLATE_PATH).decode()

    with open(alembic_ini, 'w') as f:
        f.write(
            alembic_ini_tpl.format(
                alembic_dir=ALEMBIC_DIR,
                # If there are any %s in the URL, they should be replaced with %%, since ConfigParser
                # by default uses %() for substitution. You'll get %s in your URL when you have usernames
                # with special chars (such as '@') that need to be URL encoded. URL Encoding is done with %s.
                # YAY for nested templates?
                db_url=str(db_url).replace('%', '%%'),
            ),
        )


@contextmanager
def _temp_alembic_ini(db_url):
    """Context manager for temporary JupyterHub alembic directory.

    Temporarily write an alembic.ini file for use with alembic migration scripts.

    Context manager yields alembic.ini path.

    Parameters
    ----------
    db_url: str
        The SQLAlchemy database url, e.g. `sqlite:///jupyterhub.sqlite`.

    Returns
    -------
    alembic_ini: str
        The path to the temporary alembic.ini that we have created.
        This file will be cleaned up on exit from the context manager.

    """
    with TemporaryDirectory() as td:
        alembic_ini = os.path.join(td, 'alembic.ini')
        write_alembic_ini(alembic_ini, db_url)
        yield alembic_ini


def check_call_alembic(db_url, args):
    with _temp_alembic_ini(db_url) as alembic_ini:
        check_call(
            [sys.executable, '-c', alembic_ini] + args,
            env={
                'Y_PYTHON_ENTRY_POINT': 'jupytercloud.backend.db.alembic.main:main',
            },
        )


def upgrade(db_url, revision='head'):
    """Upgrade the given database to revision.

    db_url: str
        The SQLAlchemy database url, e.g. `sqlite:///jupyterhub.sqlite`.
    revision: str [default: head]
        The alembic revision to upgrade to.

    """

    check_call_alembic(db_url, ['upgrade', revision])


def backup_db_file(db_file, log=None):
    """Backup a database file if it exists."""
    timestamp = datetime.now().strftime('.%Y-%m-%d-%H%M%S')
    backup_db_file = db_file + timestamp
    for i in range(1, 10):
        if not os.path.exists(backup_db_file):
            break
        backup_db_file = f'{db_file}.{timestamp}.{i}'
    #
    if os.path.exists(backup_db_file):
        raise OSError(f'backup db file already exists: {backup_db_file}')
    if log:
        log.info('Backing up %s => %s', db_file, backup_db_file)
    shutil.copy(db_file, backup_db_file)


def get_safe_db_url(db_url):
    urlinfo = urlparse(db_url)
    if urlinfo.password:
        # avoid logging the database password
        urlinfo = urlinfo._replace(
            netloc='{}:[redacted]@{}:{}'.format(
                urlinfo.username, urlinfo.hostname, urlinfo.port,
            ),
        )
        db_log_url = urlinfo.geturl()
    else:
        db_log_url = db_url

    return db_log_url


def get_yc_connect_args(cluster_id, hosts, port, user, password, dbname):
    # password with some
    assert not any(symbol in password for symbol in (':', '/', '@')), 'bad symbol at db password'

    cname = f'c-{cluster_id}.rw.db.yandex.net'

    proto = 'postgresql+psycopg2'

    # raw url with cname will be given to c.JupyterCloudDB.db_url/c.JupyterHub.db_url
    # to make whole thing with alembic workable
    db_url = f'{proto}://{user}:{password}@{cname}:{port}/{dbname}?sslmode=verify-full'

    if get_safe_db_url(db_url) != db_url.replace(password, '[redacted]'):
        raise RuntimeError('db password is not suitable because of bad symbols')

    # but if we give host and another args as kwargs to psycopg2 it will take priority
    # over the db_ul
    db_kwargs = {
        'connect_args': {
            'sslmode': 'verify-full',
            # XXX this is requires libpq5>10 package, which is not available for
            # xenial by default
            'target_session_attrs': 'read-write',
            'host': cname,
            'port': port,
            'user': user,
            'password': password,
            'database': dbname,
        },
    }

    return db_url, db_kwargs


def set_yc_connect_args(db_configurable, **kwargs):
    """Set YandexCloud connection args.

    Because JupyterCloudDB and Jupyterhub both have db_url/db_kwargs members,
    we can set it in same way.
    """
    db_configurable.db_url, db_configurable.db_kwargs = get_yc_connect_args(
        **kwargs,
    )


def setup_execute_logging(logger_name='sqlalchemy'):
    logger = logging.getLogger(logger_name)

    @event.listens_for(Engine, 'before_cursor_execute')
    def before_cursor_execute(
        conn, cursor, statement, parameters, context, executemany,
    ):
        conn.info.setdefault('query_start_time', []).append(time.time())
        logger.debug('start query: %s', statement)

    @event.listens_for(Engine, 'after_cursor_execute')
    def after_cursor_execute(
        conn, cursor, statement, parameters, context, executemany,
    ):
        total = time.time() - conn.info['query_start_time'].pop(-1)
        logger.debug('query complete within %f seconds', total)


def upgrade_if_needed(db_url, backup=True, log=None):
    """Upgrade a database if needed.

    If the database is sqlite, a backup file will be created with a timestamp.
    Other database systems should perform their own backups prior to calling this.
    """
    # run check-db-revision first
    engine = create_engine(db_url)
    try:
        orm.check_db_revision(engine)
    except orm.DatabaseSchemaMismatch:
        # ignore mismatch error because that's what we are here for!
        pass
    else:
        # nothing to do
        return

    log.info('Upgrading %s', get_safe_db_url(db_url))
    # we need to upgrade, backup the database
    if backup and db_url.startswith('sqlite:///'):
        db_file = db_url.split(':///', 1)[1]
        backup_db_file(db_file, log=log)
    upgrade(db_url)


def shell(args, config):
    """Start an IPython shell hooked up to the jupyerhub database."""
    from jupytercloud.backend.services.idm.app import IDMIntegrationApp

    idm = IDMIntegrationApp()
    idm.config_file = config
    db_url = idm.jupyter_cloud_db.db_url

    db = orm.new_session_factory(db_url, **idm.jupyter_cloud_db.db_kwargs)()
    ns = {'db': db, 'db_url': db_url, 'orm': orm}

    import IPython

    IPython.start_ipython(args, user_ns=ns)


def _alembic(args, config):
    """Run an alembic command with a temporary alembic.ini."""
    from jupytercloud.backend.services.idm.app import IDMIntegrationApp

    idm = IDMIntegrationApp()
    idm.config_file = config
    idm.load_config_file()

    db_url = idm.jupyter_cloud_db.db_url

    check_call_alembic(db_url, args)


def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]

    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('command', choices=['shell', 'alembic'])
    parser.add_argument('--config', required=True)

    args, new_args = parser.parse_known_args(argv)

    assert os.path.exists(args.config)

    {
        'shell': shell,
        'alembic': _alembic,
    }[args.command](new_args, args.config)


if __name__ == '__main__':
    sys.exit(main())
