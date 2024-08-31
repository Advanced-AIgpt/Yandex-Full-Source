"""

ACHTUNG!

Most of this file is copy-pasta from mysql cache. Do not judge my codestyle.

"""


import logging
import time

import salt.payload
from salt.exceptions import SaltCacheError

try:
    # Trying to import psycopg2
    import psycopg2
    from psycopg2 import sql
    from psycopg2 import OperationalError
    from psycopg2 import Binary
except ImportError:
    psycopg2 = None


_DEFAULT_DATABASE_NAME = "salt_cache"
_DEFAULT_CACHE_TABLE_NAME = "cache"
_RECONNECT_INTERVAL_SEC = 0.050

log = logging.getLogger(__name__)
client = None
_postgres_kwargs = None
_table_name = None

# Module properties

__virtualname__ = "postgres_cache"
__func_alias__ = {"ls": "list"}


def __virtual__():
    """
    Confirm that a python postgres client is installed.
    """
    return bool(psycopg2), "No python postgres client installed." if psycopg2 is None else ""


def run_query(conn, query, vars=None, retries=3):
    """
    Get a cursor and run a query. Reconnect up to `retries` times if
    needed.
    Returns: cursor, affected rows counter
    Raises: SaltCacheError, AttributeError, OperationalError
    """
    log.debug("Running query %s with vars %s", query, vars)
    try:
        cur = conn.cursor()
        cur.execute(query, vars)
        out = cur.rowcount
        return cur, out
    except (AttributeError, OperationalError) as e:
        if retries == 0:
            raise
        # reconnect creating new client
        time.sleep(_RECONNECT_INTERVAL_SEC)
        if conn is None:
            log.debug("postgres_cache: creating db connection")
        else:
            log.info("postgres_cache: recreating db connection due to: %r", e)
        global client
        client = psycopg2.connect(**_postgres_kwargs)
        client.set_session(readonly=False, autocommit=True)
        return run_query(client, query, vars, retries - 1)
    except Exception as e:  # pylint: disable=broad-except
        raise SaltCacheError("Error running {}: {}".format(query.as_string(conn), e))


def _create_table():
    """
    Create table if needed
    """
    # Explicitly check if the table already exists as the library logs a
    # warning on CREATE TABLE
    query = """SELECT EXISTS(SELECT * FROM information_schema.tables
        WHERE table_catalog=%s AND table_name=%s)"""
    vars = (_postgres_kwargs["dbname"], _table_name)
    cur, _ = run_query(client, query, vars)
    r = cur.fetchone()
    cur.close()
    if r[0]:
        return

    query = sql.SQL(
        """CREATE TABLE IF NOT EXISTS {} (
      bank varchar(255),
      etcd_key varchar(255),
      data bytea,
      PRIMARY KEY(bank, etcd_key)
    );"""
    ).format(sql.Identifier(_table_name))
    log.info("postgres_cache: creating table %s", _table_name)
    cur, _ = run_query(client, query)
    cur.close()


def _init_client():
    """Initialize connection and create table if needed"""
    if client is not None:
        return

    global _postgres_kwargs, _table_name
    _postgres_kwargs = {
        "host": __opts__.get("postgres_cache.host", "127.0.0.1"),
        "user": __opts__.get("postgres_cache.user", None),
        "password": __opts__.get("postgres_cache.password", None),
        "dbname": __opts__.get("postgres_cache.database", _DEFAULT_DATABASE_NAME),
        "port": __opts__.get("postgres_cache.port", 5433),
        "connect_timeout": __opts__.get("postgres_cache.connect_timeout", None),
    }
    _table_name = __opts__.get("postgres_cache.table_name", _table_name)
    # TODO: handle SSL connection parameters

    kwargs_copy = {k:v for k, v in _postgres_kwargs.items() if v is not None}
    _postgres_kwargs = kwargs_copy.copy()
    kwargs_copy["password"] = "<hidden>"
    log.info("postgres_cache: Setting up client with params: %r", kwargs_copy)
    # The Postgres client is created later on by run_query
    _create_table()


def store(bank, key, data):
    """
    Store a key value.
    """
    _init_client()
    data = salt.payload.dumps(data)
    query = sql.SQL(
        """INSERT INTO {} (bank, etcd_key, data)
        VALUES (%s, %s, %s)
        ON CONFLICT (bank, etcd_key) DO UPDATE
        SET data = EXCLUDED.data;"""
    ).format(sql.Identifier(_table_name))
    vars = (bank, key, psycopg2.Binary(data))

    cur, cnt = run_query(client, query, vars)
    cur.close()
    if cnt not in (1, 2):
        raise SaltCacheError("Error storing {} {} returned {}".format(bank, key, cnt))


def fetch(bank, key):
    """
    Fetch a key value.
    """
    _init_client()
    query = sql.SQL("""SELECT data FROM {} WHERE bank=%s AND etcd_key=%s""").format(sql.Identifier(_table_name))
    vars = (bank, key)
    cur, _ = run_query(client, query, vars)
    r = cur.fetchone()
    cur.close()
    if r is None:
        return {}
    return salt.payload.loads(r[0])


def flush(bank, key=None):
    """
    Remove the key from the cache bank with all the key content.
    """
    _init_client()
    query = sql.SQL("""DELETE FROM {} WHERE bank=%s""").format(sql.Identifier(_table_name))
    vars = (bank,)
    if key is not None:
        query += " AND etcd_key=%s"
        vars = vars + (key,)

    cur, _ = run_query(client, query, vars)
    cur.close()


def ls(bank):
    """
    Return an iterable object containing all entries stored in the specified
    bank.
    """
    _init_client()
    query = sql.SQL("SELECT etcd_key FROM {} WHERE bank=%s").format(sql.Identifier(_table_name))
    vars = (bank,)
    cur, _ = run_query(client, query, vars)
    out = [row[0] for row in cur.fetchall()]
    cur.close()
    return out


def contains(bank, key):
    """
    Checks if the specified bank contains the specified key.
    """
    _init_client()
    query = sql.SQL("SELECT COUNT(data) FROM {} WHERE bank=%s AND etcd_key=%s").format(sql.Identifier(_table_name))
    vars = (bank, key)
    cur, _ = run_query(client, query, vars)
    r = cur.fetchone()
    cur.close()
    return r[0] == 1
