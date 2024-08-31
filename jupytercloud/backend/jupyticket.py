import datetime

from traitlets import Instance, Integer
from traitlets.config import Configurable

from jupytercloud.backend.lib.clients.startrek import StartrekClient
from jupytercloud.backend.lib.db.configurable import JupyterCloudDB
from jupytercloud.backend.lib.db.orm import JupyTicket as JupyTicketModel
from jupytercloud.backend.lib.db.orm import JupyTicketArcadia as ArcadiaModel
from jupytercloud.backend.lib.db.orm import JupyTicketNirvana as NirvanaModel
from jupytercloud.backend.lib.db.orm import JupyTicketStartrek as StartrekModel


class JupyTicket(Configurable):
    db = Instance(JupyterCloudDB)

    # startrek_client binds a handler with user cookies
    # to use UserTicket authorization
    startrek_client = Instance(StartrekClient)

    id = Integer()
    model = Instance(JupyTicketModel)

    _fields = set(JupyTicketModel.__table__.columns.keys())

    @classmethod
    def new(cls, db, startrek_client, **kwargs):
        model = JupyTicketModel(**kwargs)

        # if we will do `db.add(model)` inside outer db.transaction(),
        # flush will not happen and we won't get model id afterwards
        assert not db.db.transaction
        with db.transaction():
            db.add(model)

        return cls(
            db=db,
            startrek_client=startrek_client,
            id=model.id,
            model=model,
        )

    @classmethod
    def get(cls, db, startrek_client, id):
        model = db.query(JupyTicketModel) \
            .filter(JupyTicketModel.id == id) \
            .one_or_none()

        if not model:
            return None

        return cls(
            db=db,
            startrek_client=startrek_client,
            id=model.id,
            model=model,
        )

    @classmethod
    def last_by_path(cls, db, startrek_client, arcadia_path):
        model = db.query(JupyTicketModel) \
            .filter(JupyTicketModel.arcadia_path == arcadia_path) \
            .order_by(JupyTicketModel.arcadia_revision.desc()) \
            .first()

        if not model:
            return None

        return cls(
            db=db,
            startrek_client=startrek_client,
            id=str(model.id),
            model=model,
        )

    def update(self, **kwargs):
        with self.db.transaction():
            for key, value in kwargs.items():
                setattr(self.model, key, value)

            self.model.updated = datetime.datetime.utcnow()

    def get_all_startrek_tickets(self):
        return self.db.query(StartrekModel) \
            .filter(StartrekModel.jupyticket_id == self.id) \
            .order_by(StartrekModel.created.desc()) \
            .all()

    async def set_startrek_tickets(self, tickets):
        tickets = set(tickets)

        with self.db.transaction():
            startrek_tickets = self.get_all_startrek_tickets()

            existent = {t.startrek_id for t in startrek_tickets}

            tickets_to_add = tickets - existent
            tickets_to_delete = existent - tickets

            if tickets_to_delete:
                self.db.query(StartrekModel) \
                    .filter(
                        StartrekModel.jupyticket_id == self.id,
                        StartrekModel.startrek_id.in_(tickets_to_delete),
                    ) \
                    .delete()

            if tickets_to_add:
                self.db.bulk_save_objects([
                    StartrekModel(
                        jupyticket_id=self.id,
                        startrek_id=startrek_id,
                    ) for startrek_id in tickets_to_add
                ])

                for ticket in tickets_to_add:
                    await self.startrek_client.create_link(ticket, self.id, 'relates')

            self.update()

    def add_new_arcadia_share(self, user_name, path, revision, message):
        with self.db.transaction():
            model = ArcadiaModel(
                jupyticket_id=self.id,
                user_name=user_name,
                path=path,
                revision=revision,
                message=message,
            )

            self.db.add(model)
            self.update()

    def get_all_shares(self):
        return self.db.query(ArcadiaModel) \
            .filter(ArcadiaModel.jupyticket_id == self.id) \
            .order_by(ArcadiaModel.shared.desc()) \
            .all()

    def get_last_share(self):
        model = self.db.query(ArcadiaModel) \
            .filter(ArcadiaModel.jupyticket_id == self.id) \
            .order_by(ArcadiaModel.shared.desc()) \
            .first()

        return model

    def link_nirvana_instance(self, workflow_id, instance_id):
        with self.db.transaction():
            model = self.db.query(NirvanaModel) \
                .filter_by(
                    workflow_id=workflow_id,
                    instance_id=instance_id,
                ) \
                .first()

            if model:
                return

            model = NirvanaModel(
                jupyticket_id=self.id,
                workflow_id=workflow_id,
                instance_id=instance_id,
            )
            self.db.add(model)
            self.update()

    def get_all_nirvana_instances(self):
        return self.db.query(NirvanaModel) \
            .filter(NirvanaModel.jupyticket_id == self.id) \
            .order_by(NirvanaModel.created.desc()) \
            .all()

    def as_dict(self):
        return {
            key: getattr(self.model, key)
            for key in self._fields
        }
