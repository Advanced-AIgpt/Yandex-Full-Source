from traitlets import Instance, Unicode, default
from traitlets.config import Configurable

from jupytercloud.backend.lib.clients.abc import ABCClient
from jupytercloud.backend.lib.clients.staff import StaffClient
from jupytercloud.backend.lib.clients.yp import get_available_accounts


class VMPermissions(Configurable):
    login = Unicode()
    default_account_id = Unicode()

    jupyter_cloud_abc_group = Unicode(config=True)
    jupyter_cloud_robot_user = Unicode(config=True)

    abc_access_scope = Unicode('administration', config=True, allow_none=True)

    abc_client = Instance(ABCClient)

    @default('abc_client')
    def _abc_client_default(self):
        return ABCClient.instance(parent=self)

    staff_client = Instance(StaffClient)

    @default('staff_client')
    def _staff_client_default(self):
        return StaffClient.instance(parent=self)

    async def get_group_ids(self, account_id):
        group_ids = [self.jupyter_cloud_abc_group]

        if (
            account_id == self.default_account_id or
            not self.abc_access_scope
        ):
            return group_ids

        # get_service_role response structure example:
        #
        # GET https://staff-api.yandex-team.ru/v3/groups
        # ?parent.service.id=2142
        # &role_scope=administration
        # &_fields=id,name,affiliation_counters
        # &type=servicerole
        # &_one=1
        #
        # {
        #     "affiliation_counters": {
        #         "yandex": 2,
        #         "yamoney": 0,
        #         "external": 0
        #     },
        #     "id": 184158,
        #     "name": "Администрирование (Jupyter в облаках)"
        # }

        if quota_abc_id := self.abc_client.parse_account_id(account_id):
            service_role = await self.staff_client.get_service_role(
                quota_abc_id, self.abc_access_scope,
            ) or {}

            service_role_id = service_role.get('id')

            affiliation_counter = max(
                service_role.get('affiliation_counters', {}).items(),
                default=0
            )

            if service_role_id and affiliation_counter:
                group_ids.append(str(service_role_id))

        return group_ids

    def get_owners(self):
        return [self.login, self.jupyter_cloud_robot_user]

    async def check_account_access(self, account_id):
        available_accounts = await get_available_accounts(self, self.jupyter_cloud_robot_user)
        return account_id in available_accounts
