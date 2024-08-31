from contextlib import contextmanager
from copy import deepcopy

from traitlets import Any, Bool, Dict, Unicode
from traitlets.config.configurable import SingletonConfigurable

from jupytercloud.backend.lib.db import orm
from jupytercloud.backend.lib.db.util import get_safe_db_url


class JupyterCloudDB(SingletonConfigurable):
    db_url = Unicode(
        'sqlite:///jupytercloud.sqlite', config=True,
        help='url for the database. e.g. `sqlite:///jupytercloud.sqlite`',
    )
    debug_db = Bool(
        False, config=True,
        help='log all database transactions. This has A LOT of output',
    )
    reset_db = Bool(
        False, config=True,
        help='Purge and reset the database.',
    )
    db_kwargs = Dict(
        config=True,
        help="""Include any kwargs to pass to the database connection.
        See sqlalchemy.create_engine for details.
        """,
    )
    session_factory = Any()

    _db = None

    @property
    def db(self):
        if self._db is None:
            self.init_db()

        return self._db

    @property
    def query(self):
        return self.db.query

    @property
    def add(self):
        return self.db.add

    @property
    def bulk_save_objects(self):
        return self.db.bulk_save_objects

    @contextmanager
    def transaction(self):
        assert self.db.autocommit, (
            'this pattern expects the session to be in autocommit mode. '
            'This assertion can be removed for SQLAlchemy 1.4.'
        )
        if not self.db.transaction:
            with self.db.begin():
                yield
        else:
            yield

    def init_db(self):
        self.log.debug('Connecting to db: %s', get_safe_db_url(self.db_url))
        self.session_factory = orm.new_session_factory(
            self.db_url,
            reset=self.reset_db,
            echo=self.debug_db,
            **self.db_kwargs,
        )
        self._db = self.session_factory()

    def get_user_idm_state(self, login):
        user = orm.User.find(self.db, login)

        idm_state = user.idm_state if user else None
        return idm_state or {}

    def get_patched_user_idm_state(self, login):
        # XXX: Тут мы патчим idm-state пользователя, разрешая ему использовать помимо
        # инстансов с hdd еще инстансы в ssd.
        # Надо бы подумать, как это сделать по-нормальному, например, раздавать раздельные
        # роли под hdd и ssd.
        # Делаем это отдельным методом, чтобы не задеть сам IDM, и то, что мы ему возвращаем.
        idm_state = self.get_user_idm_state(login)

        for role_name, role_info in list(idm_state.items()):
            if role_name.startswith('/role/quota/vm/'):
                new_role_name = role_name.replace('hdd', 'ssd')
                new_role_info = deepcopy(role_info)
                new_role_info['role']['vm'] = new_role_info['role']['vm'].replace('hdd', 'ssd')
                idm_state[new_role_name] = new_role_info

        return idm_state

    def add_or_update_user(self, login, *, idm_state=None):
        with self.transaction():
            user = orm.User.find(self.db, login)

            if user:
                if idm_state is not None:
                    user.idm_state = idm_state
                    self.log.debug('user %s: setting idm state to %s', login, idm_state)
            else:
                self.log.debug('creating user %s and setting idm state to %s', login, idm_state)
                user = orm.User(name=login, idm_state=idm_state or {})
                self.db.add(user)

        return user

    def add_or_update_pillar(self, minion_id, name, value):
        self.log.debug('minion %s: setting pillar %s', minion_id, name)

        with self.transaction():
            pillar = orm.Pillar.find(self.db, minion_id, name)

            if pillar:
                pillar.value = value
            else:
                pillar = orm.Pillar(
                    minion_id=minion_id,
                    name=name,
                    value=value,
                )
                self.db.add(pillar)

    def get_oauth_token(self, login, token_name):
        token = orm.OAuthToken.find(self.db, user_name=login, name=token_name)

        return token.value if token else None

    def add_or_update_token(self, login, token_name, token_value):
        self.log.debug('user %s: setting oauth token %s', login, token_name)

        with self.transaction():
            self.add_or_update_user(login)

            token = orm.OAuthToken.find(self.db, user_name=login, name=token_name)

            if token:
                token.value = token_value
            else:
                token = orm.OAuthToken(
                    user_name=login,
                    name=token_name,
                    value=token_value,
                )
                self.db.add(token)

    def drop_oauth_open(self, login, token_name):
        self.log.debug('user %s: dropping oauth token %s', login, token_name)

        with self.transaction():
            token = orm.OAuthToken.find(self.db, user_name=login, name=token_name)
            if token:
                self.db.delete(token)

    def clean_dirty(self):
        if self.db.dirty:
            self.log.warning('Rolling back dirty objects %s', self.db.dirty)
            self.db.rollback()
