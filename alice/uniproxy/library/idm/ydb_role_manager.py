from .role_manager import RoleManagerProxy, CachingRoleManager, safe_wrapper

from ydb.tornado import as_tornado_future
import ydb

from concurrent.futures import TimeoutError
import logging
import os


class YdbRoleManagerProxy(RoleManagerProxy):
    def __init__(self, endpoint, database, table, page_size, logger=None, token=None):
        self.database = database
        self.endpoint = endpoint
        self.page_size = page_size
        self.initialized = False
        self.logger = logger or logging.getLogger('YdbRoleManagerProxy')
        self.table = '{}/{}'.format(self.database, table)
        self.token = token or os.getenv('YDB_TOKEN')

    def _init_driver(self):
        self.logger.debug('Initializing YdbClient')
        connection_params = ydb.DriverConfig(self.endpoint, database=self.database, auth_token=self.token)
        self.driver = ydb.Driver(connection_params)
        try:
            self.driver.wait(timeout=5)
            self.session_pool = ydb.SessionPool(driver=self.driver, size=100)
            self.initialized = True
            self.logger.debug('YdbClient initialized')
        except TimeoutError:
            details = self.driver.discovery_debug_details()
            raise RuntimeError('Failed to establish connection to YDB. Errors:\n{}'.format(details))

    @classmethod
    def _encode_query_params(cls, parameters):
        for key, value in parameters.items():
            if isinstance(value, str):
                parameters[key] = bytes(value, encoding='utf-8')
        return parameters

    @classmethod
    def _decode_row(cls, row):
        return tuple(map(lambda bts: bts.decode('utf-8'), (row.login, row.user_type, row.role)))

    async def _execute_query(self, query, parameters=None):
        if not self.initialized:
            self._init_driver()

        if parameters is not None:
            parameters = self._encode_query_params(parameters)

        with self.session_pool.async_checkout() as async_session:
            session = await as_tornado_future(async_session)
            prepared_query = await as_tornado_future(session.async_prepare(query))
            with session.transaction() as transaction:
                return await as_tornado_future(transaction.async_execute(
                    prepared_query,
                    parameters,
                    commit_tx=True,
                    settings=ydb.BaseRequestSettings().with_timeout(1),
                ))

    @property
    def add_query_template(self):
        return '''
            DECLARE $login AS String;
            DECLARE $user_type AS String;
            DECLARE $role AS String;

            UPSERT INTO [{table_name}]
            (login, user_type, role)
            VALUES
            ($login, $user_type, $role);
        '''.format(table_name=self.table)

    @property
    def search_query_template(self):
        return '''
            DECLARE $login AS String;
            DECLARE $user_type AS String;
            DECLARE $role AS String;

            SELECT login, user_type, role
            FROM [{table_name}]
            WHERE login = $login AND user_type = $user_type AND role = $role;
        '''.format(table_name=self.table)

    def list_query_template(self, offset, limit):
        return '''
            SELECT login, user_type, role
            FROM [{table_name}]
            LIMIT {limit} OFFSET {offset};
        '''.format(table_name=self.table, offset=offset, limit=limit)

    @property
    def remove_query_template(self):
        return '''
            DECLARE $login AS String;
            DECLARE $user_type AS String;
            DECLARE $role AS String;

            DELETE FROM [{table_name}]
            WHERE login = $login AND user_type = $user_type AND role = $role;
        '''.format(table_name=self.table)

    @safe_wrapper
    async def try_add_role(self, login, user_type, role):
        await self._execute_query(self.add_query_template, {
            '$login': login,
            '$user_type': user_type,
            '$role': role,
        })

    @safe_wrapper
    async def try_remove_role(self, login, user_type, role):
        await self._execute_query(self.remove_query_template, {
            '$login': login,
            '$user_type': user_type,
            '$role': role,
        })

    @safe_wrapper
    async def try_get_roles(self, offset, limit):
        results = await self._execute_query(self.list_query_template(offset, limit))
        return list(map(self._decode_row, results[0].rows))

    @safe_wrapper
    async def try_get_all_roles(self):
        roles = list()
        offset = 0
        while True:
            results = await self._execute_query(self.list_query_template(offset, self.page_size))
            prev_size = len(roles)

            for row in results[0].rows:
                roles.append(self._decode_row(row))

            if len(roles) == prev_size:
                return roles

            offset += self.page_size

    @safe_wrapper
    async def try_is_granted(self, login, user_type, role):
        '''Not used by CachingRoleManager'''
        results = await self._execute_query(self.search_query_template, {
            '$login': login,
            '$user_type': user_type,
            '$role': role,
        })
        for row in results[0].rows:
            if (login, user_type, role) == self._decode_row(row):
                return True
        return False


def make_ydb_role_manager(role_info, *args, **kwargs):
    ydb_role_manager = YdbRoleManagerProxy(*args, **kwargs)
    return CachingRoleManager(role_info, ydb_role_manager, logger=ydb_role_manager.logger)
