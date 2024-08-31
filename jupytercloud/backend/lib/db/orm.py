import datetime

import alembic.command
import alembic.config
from alembic.arcadia.script import ArcadiaScriptDirectory
from jupyterhub.orm import (
    DatabaseSchemaMismatch, JSONDict, add_row_format, mysql_large_prefix_check,
    register_foreign_keys, register_ping_connection,
)
from sqlalchemy import (
    Column, DateTime, ForeignKey, Integer, Text, Unicode, UnicodeText, UniqueConstraint,
    create_engine,
)
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker
from sqlalchemy.pool import StaticPool
from tornado.log import app_log


class Serializable:
    def as_dict(self):
        return dict((col, getattr(self, col)) for col in self.__table__.columns.keys())

# This changes alembic's behaviour
# so it does not use filesystem anymore,
# but uses resources from within the binary.
# That is useful for Arcadia!
alembic.settings.use_arcadia_modules = True

Base = declarative_base(cls=Serializable)
Base.log = app_log


class User(Base):
    __tablename__ = 'users'

    id = Column(Integer, primary_key=True, autoincrement=True)
    name = Column(Unicode(255), unique=True)
    idm_state = Column(JSONDict)
    settings = Column(JSONDict)

    @classmethod
    def find(cls, db, name):
        return db.query(cls).filter(cls.name == name).first()


# NOTE: По-хорошему надо ввести промежуточную таблицу User << Server << Pillar.
# Но текущая мысль в том, что pillar-ы никогда не будут удаляться, потому что
# может выйти так, что hub будет думать, что сервер умер, но pillars в этом случае все
# равно хочется хранить для этого сервера.
class Pillar(Base):
    __tablename__ = 'pillars'

    __table_args__ = (
        UniqueConstraint('minion_id', 'name'),
    )

    id = Column(Integer, primary_key=True, autoincrement=True)
    minion_id = Column(Unicode(255), index=True)

    name = Column(Unicode(255))
    value = Column(Text())

    @classmethod
    def find(cls, db, minion_id, name):
        return db.query(cls).filter(cls.minion_id == minion_id, cls.name == name).first()


class OAuthToken(Base):
    __tablename__ = 'oauth_tokens'

    __table_args__ = (
        UniqueConstraint('user_name', 'name'),
    )

    id = Column(Integer, primary_key=True, autoincrement=True)
    user_name = Column(
        Unicode(255),
        ForeignKey('users.name', ondelete='CASCADE'),
        nullable=True,
    )
    name = Column(Unicode(255))
    value = Column(Unicode(255))

    @classmethod
    def find(cls, db, user_name, name):
        return db.query(cls).filter(cls.name == name, cls.user_name == user_name).first()


class JupyTicket(Base):
    __tablename__ = 'jupyticket'

    id = Column(Integer, primary_key=True, autoincrement=True)

    user_name = Column(
        Text(),
        ForeignKey(User.name, ondelete='RESTRICT'),
        nullable=False,
    )

    created = Column(
        DateTime,
        default=datetime.datetime.utcnow,
        nullable=False,
    )

    updated = Column(
        DateTime,
        default=datetime.datetime.utcnow,
        nullable=False,
    )

    title = Column(
        UnicodeText(),
        nullable=False,
    )

    description = Column(
        UnicodeText(),
        nullable=True,
    )


class JupyTicketArcadia(Base):
    __tablename__ = 'jupyticket_arcadia'

    jupyticket_id = Column(
        Integer(),
        ForeignKey(JupyTicket.id, ondelete='CASCADE'),
        primary_key=True,
    )

    user_name = Column(
        Text(),
        ForeignKey(User.name, ondelete='RESTRICT'),
        nullable=False,
    )

    path = Column(
        Text(),
        primary_key=True,
    )

    revision = Column(
        Text(),
        primary_key=True,
    )

    shared = Column(
        DateTime,
        default=datetime.datetime.utcnow,
        nullable=False,
    )

    message = Column(
        UnicodeText(),
        nullable=True,
    )


class JupyTicketStartrek(Base):
    __tablename__ = 'jupyticket_startrek'

    jupyticket_id = Column(
        Integer(),
        ForeignKey(JupyTicket.id, ondelete='CASCADE'),
        primary_key=True,
    )

    startrek_id = Column(
        Text(),
        primary_key=True,
    )

    created = Column(
        DateTime,
        default=datetime.datetime.utcnow,
        nullable=False,
    )


class JupyTicketNirvana(Base):
    __tablename__ = 'jupyticket_nirvana'

    jupyticket_id = Column(
        Integer(),
        ForeignKey(JupyTicket.id, ondelete='CASCADE'),
        primary_key=True,
    )

    workflow_id = Column(
        UUID(as_uuid=False),
        primary_key=True,
    )

    instance_id = Column(
        UUID(as_uuid=False),
        primary_key=True,
    )

    created = Column(
        DateTime,
        default=datetime.datetime.utcnow,
        nullable=False,
    )


def check_db_revision(engine):
    from jupytercloud.backend.lib.db.util import _temp_alembic_ini

    current_table_names = set(engine.table_names())
    my_table_names = set(Base.metadata.tables.keys())

    with _temp_alembic_ini(engine.url) as ini:
        cfg = alembic.config.Config(ini)
        scripts = ArcadiaScriptDirectory.from_config(cfg)

        head = scripts.get_heads()[0]
        base = scripts.get_base()

        if not my_table_names.intersection(current_table_names):
            # no tables have been created, stamp with current revision
            app_log.debug('Stamping empty database with alembic revision %s', head)
            alembic.command.stamp(cfg, head)
            return

        if 'alembic_version' not in current_table_names:
            app_log.debug('Stamping database schema version %s', base)
            alembic.command.stamp(cfg, base)

    # check database schema version
    # it should always be defined at this point
    alembic_revision = engine.execute(
        'SELECT version_num FROM alembic_version',
    ).first()[0]
    if alembic_revision == head:
        app_log.debug('database schema version found: %s', alembic_revision)
    else:
        raise DatabaseSchemaMismatch(
            'Found database schema version {found} != {head}. '
            'Backup your database and run `jupytercloud-idm-service upgrade-db`'
            ' to upgrade to the latest schema.'.format(
                found=alembic_revision, head=head,
            ),
        )


def new_session_factory(
    url='sqlite:///:memory:', reset=False, expire_on_commit=False, **kwargs,
):
    """Create a new session at url."""
    if url.startswith('sqlite'):
        kwargs.setdefault('connect_args', {'check_same_thread': False})

    elif url.startswith('mysql'):
        kwargs.setdefault('pool_recycle', 60)

    if url.endswith(':memory:'):
        # If we're using an in-memory database, ensure that only one connection
        # is ever created.
        kwargs.setdefault('poolclass', StaticPool)

    engine = create_engine(url, **kwargs)
    if url.startswith('sqlite'):
        register_foreign_keys(engine)

    # enable pessimistic disconnect handling
    register_ping_connection(engine)

    if reset:
        Base.metadata.drop_all(engine)

    if mysql_large_prefix_check(engine):  # if mysql is allows large indexes
        add_row_format(Base)  # set format on the tables
    # check the db revision (will raise, pointing to `upgrade-db` if version doesn't match)

    check_db_revision(engine)

    Base.metadata.create_all(engine)

    # We set expire_on_commit=False, since we don't actually need
    # SQLAlchemy to expire objects after committing - we don't expect
    # concurrent runs of the hub talking to the same db. Turning
    # this off gives us a major performance boost
    session_factory = sessionmaker(
        bind=engine,
        expire_on_commit=expire_on_commit,
        autocommit=True,
    )
    return session_factory
