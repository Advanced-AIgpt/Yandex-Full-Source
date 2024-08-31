import asyncio
import json

from traitlets import Bool, Dict, Float, HasTraits, Instance, Integer, Unicode, default

from jupytercloud.backend.lib.clients.yp import get_available_accounts
from jupytercloud.backend.lib.util.format import pretty_float
from jupytercloud.backend.lib.util.misc import HasTraitsReprMixin

from .instance import CPU_MULT, GB, MB, BasePresets, JupyterPresets, UserPresets, SEGMENT_DEFAULT


ACCOUNT_PERSONAL = 'PERSONAL'
ACCOUNT_SERVICE = 'SERVICE'


class _PathDict(dict):
    def get_path(self, *path):
        struct = self

        for element in path:
            struct = struct.get(element)

            if not struct:
                return None

        if struct:
            return struct

        return None


class Resources(HasTraits, HasTraitsReprMixin):
    resources_types = ('mem', 'cpu', 'ssd', 'hdd', 'internet_adress')

    mem = Float()
    cpu = Float()
    ssd = Float()
    hdd = Float()
    ssd_io = Float()
    hdd_io = Float()
    internet_adress = Float()

    @classmethod
    def from_json(cls, data):
        if not data:
            return cls()

        mem = int(data.get('mem', 0))
        cpu = int(data.get('cpu', 0))
        internet_adress = int(data.get('internetAddress', 0))

        disk = data.get('diskPerStorage', {})
        ssd = int(disk.get('ssd', 0))
        hdd = int(disk.get('hdd', 0))

        io = data.get('ioGuaranteesPerStorage', {})
        ssd_io = int(io.get('ssd', 0))
        hdd_io = int(io.get('hdd', 0))

        return cls(
            mem=mem,
            cpu=cpu,
            ssd=ssd,
            hdd=hdd,
            ssd_io=ssd_io,
            hdd_io=hdd_io,
            internet_access=internet_adress,
        )

    @property
    def cpu_pretty(self):
        return pretty_float(self.cpu / CPU_MULT, 1)

    @property
    def mem_pretty(self):
        return pretty_float(self.mem / GB, 1)

    @property
    def ssd_pretty(self):
        return pretty_float(self.ssd / GB, 1)

    @property
    def hdd_pretty(self):
        return pretty_float(self.hdd / GB, 1)

    @property
    def ssd_io_pretty(self):
        return pretty_float(self.ssd_io / MB, 1)

    @property
    def hdd_io_pretty(self):
        return pretty_float(self.hdd_io / MB, 1)

    def __add__(self, other):
        cls = type(self)
        assert cls is type(other)

        return cls(
            cpu=self.cpu + other.cpu,
            mem=self.mem + other.mem,
            ssd=self.ssd + other.ssd,
            hdd=self.hdd + other.hdd,
            ssd_io=self.ssd_io + other.ssd_io,
            hdd_io=self.hdd_io + other.hdd_io,
            internet_adres=self.internet_adress + other.internet_adress,
        )

    def __sub__(self, other):
        cls = type(self)
        assert cls is type(other)

        return cls(
            cpu=self.cpu - other.cpu,
            mem=self.mem - other.mem,
            ssd=self.ssd - other.ssd,
            hdd=self.hdd - other.hdd,
            ssd_io=self.ssd_io - other.ssd_io,
            hdd_io=self.hdd_io - other.hdd_io,
            internet_adres=self.internet_adress - other.internet_adress,
        )

    def can_contain_instance(self, instance):
        disk_available = (
            instance.disk_total_size <= self.ssd
            if instance.disk_type == 'ssd' else
            instance.disk_total_size <= self.hdd
        )
        io_available = (
            instance.io_guarantee <= self.ssd_io
            if instance.disk_type == 'ssd' else
            instance.io_guarantee <= self.hdd_io
        )
        internet_available = (
            self.internet_adress > 0
            if instance.internet_access == 'ipv6_ipv4_tunnel' else
            True
        )

        return (
            instance.cpu <= self.cpu and
            instance.mem <= self.mem and
            disk_available and
            io_available and
            internet_available
        )


class Segment(HasTraits, HasTraitsReprMixin):
    name = Unicode()
    usage = Instance(Resources)
    limits = Instance(Resources)
    presets = Instance(BasePresets, allow_none=True)

    available = Bool()

    @default('available')
    def _default_available(self):
        # XXX: По смыслу это должно быть property,
        # потому что оно не предполагает изменения и целиком вычисляется,
        # основываясь на поля объекта.
        # Но ради ленивости и кешируемости оформляем в виде trait.
        return any(preset.available for preset in self.presets.all.values())

    @property
    def available_resources(self):
        return self.limits - self.usage

    def can_contain_instance(self, instance):
        return self.available_resources.can_contain_instance(instance)

    def set_presets(self, presets):
        self.presets = presets.get_available(self, presets.all, self.available_resources)


class PersonalSegment(Segment):
    personal_usage = Instance(Resources)
    personal_limits = Instance(Resources)

    @property
    def personal_available_resources(self):
        return self.personal_limits - self.personal_usage

    def can_contain_instance(self, instance):
        return (
            super().can_contain_instance(instance) and
            self.personal_available_resources.can_contain_instance(instance)
        )

    def set_presets(self, presets):
        self.presets = presets.get_available(
            self, presets.personal_all, self.personal_available_resources,
        )


class Account(HasTraits, HasTraitsReprMixin):
    type = Unicode()
    name = Unicode()
    members_count = Integer()
    segments = Dict()

    abc_id = Unicode()
    name_ru = Unicode()
    name_en = Unicode()
    readable_name = Unicode()
    abc_link = Unicode()

    @default('readable_name')
    def _default_readable_name(self):
        # XXX: По смыслу это должно быть property (см. пояснение выше)
        return self.name_en or self.name_ru or self.name

    available = Bool()

    @default('available')
    def _default_available(self):
        # XXX: По смыслу это должно быть property (см. пояснение выше)
        return any(segment.available for segment in self.segments.values())

    @staticmethod
    def _create_segment(
        segment_name,
        limits_data,
        usage_data,
        is_personal,
        personal_limits_data,
        personal_usage_data,
    ):
        limits = Resources.from_json(limits_data)
        usage = Resources.from_json(usage_data)

        if not limits.cpu or limits.cpu <= usage.cpu:
            return None

        if not is_personal:
            return Segment(
                name=segment_name,
                limits=limits,
                usage=usage,
            )

        personal_limits = Resources.from_json(personal_limits_data)
        personal_usage = Resources.from_json(personal_usage_data)

        if not personal_limits.cpu:
            return None

        return PersonalSegment(
            name=segment_name,
            limits=limits,
            usage=usage,
            personal_limits=personal_limits,
            personal_usage=personal_usage,
        )

    @property
    def is_personal(self):
        return self.type == 'PERSONAL'

    @classmethod
    def from_json(cls, data):
        if data['id'] == 'tmp':
            return None

        data = _PathDict(data)

        is_personal = data['type'] == 'PERSONAL'

        limits_per_segment = data.get_path('limits', 'perSegment')
        if not limits_per_segment:
            return None

        usage_per_segment = data.get_path('usage', 'perSegment') or {}

        segments = {}

        for segment_name, limits_data in limits_per_segment.items():
            usage_data = usage_per_segment.get(segment_name)

            personal_limits_data = None
            personal_usage_data = None

            if is_personal:
                personal_limits_data = data.get_path(
                    'personal', 'limits', 'perSegment', segment_name,
                )
                personal_usage_data = data.get_path(
                    'personal', 'usage', 'perSegment', segment_name,
                )

            segment = cls._create_segment(
                segment_name=segment_name,
                limits_data=limits_data,
                usage_data=usage_data,
                is_personal=is_personal,
                personal_limits_data=personal_limits_data,
                personal_usage_data=personal_usage_data,
            )
            if segment:
                segments[segment.name] = segment

        if segments:
            return cls(
                type=data['type'],
                name=data['id'],
                segments=segments,
                members_count=data['membersCount'],
            )

        return None


class Cluster(HasTraits, HasTraitsReprMixin):
    name = Unicode()
    accounts = Dict()

    available = Bool()

    @default('available')
    def _default_available(self):
        # XXX: По смыслу это должно быть property (см. пояснение выше)
        return any(account.available for account in self.accounts.values())

    @classmethod
    def from_json(cls, name, data):
        accounts = {}
        for account_data in data['accounts']:
            account = Account.from_json(
                data=account_data,
            )
            if account:
                accounts[account.name] = account

        if accounts:
            return cls(
                name=name,
                accounts=accounts,
            )

        return None

    def get_personal_accounts(self):
        return {
            name: account for name, account in self.accounts.items()
            if account.type == ACCOUNT_PERSONAL
        }

    def get_service_accounts(self):
        return {
            name: account for name, account in self.accounts.items()
            if account.type == ACCOUNT_SERVICE
        }


class UserQuota(HasTraits, HasTraitsReprMixin):
    """Пользовательская квота QYP.

    Этот класс отвечает за запрос информации об аккаунтах пользователя
    с разных кластеров QYP и дальнейшей ее агрегацией в удобный для отображения вид.

    Этот класс содержит в себе иерархию
    {Кластер} -> {Аккаунт на кластере} -> {Сегмент} -> {Доступные пресеты для сегмента}.

    Также в классе идет разделение на clusters (для работы с персональной квотой и
    сервисными квотами пользователя) и на jupyter_clusters (для работы с квотой JupyterCloud).

    Их принципиальная разница заключается в том, что на jupyter_clusters влияют idm-роли
    в виде доступных пресетов, а в clusters есть персональная квота, в которой доступен
    другой набор пресетов. Также в персональной квоте надо особым образом считать
    personal_usage, агрегируя информацию между кластерами.

    """

    user = Unicode()
    clusters = Dict()
    jupyter_clusters = Dict()
    personal_clusters = Dict()
    service_clusters = Dict()

    have_oauth_token = Bool()
    have_any_idm_role = Bool()
    have_personal = Bool()
    have_service = Bool()

    @classmethod
    async def request(
        cls,
        qyp_client, abc_client,
        user, jupyter_user, jupyter_account_id,
        idm_state, have_oauth_token,
        oauth_url,
    ):
        raw_accounts, robot_accounts = await asyncio.gather(
            qyp_client.request_accounts([user, jupyter_user]),
            get_available_accounts(qyp_client, jupyter_user),
        )

        clusters = {}
        for cluster, cluster_data in raw_accounts[user].items():
            cluster = Cluster.from_json(
                name=cluster,
                data=cluster_data,
            )
            if cluster:
                clusters[cluster.name] = cluster

        jupyter_clusters = {}
        for cluster, cluster_data in raw_accounts[jupyter_user].items():
            cluster = Cluster.from_json(
                name=cluster,
                data=cluster_data,
            )
            if cluster:
                jupyter_clusters[cluster.name] = cluster

                # remove non-jupyter quota, which robot can accidentally have access
                for account_name in list(cluster.accounts):  # dict
                    if account_name != jupyter_account_id:
                        del cluster.accounts[account_name]

        result = cls(
            user=user,
            clusters=clusters,
            jupyter_clusters=jupyter_clusters,
            have_oauth_token=have_oauth_token,
            have_any_idm_role=any(
                role.get('role', {}).get('role') == 'quota'
                for role in idm_state.values()
            ),
        )

        result._transform_personal_usage()
        result._transform_pop_default(jupyter_clusters)
        result._transform_set_available_presets(idm_state, have_oauth_token, robot_accounts)
        result._transorm_grant_access_links(oauth_url)
        await result._transform_set_account_names(abc_client)
        result._transform_separate_clusters()

        return result

    def _transform_pop_default(self, clusters):
        for cluster_name, cluster in list(clusters.items()):
            for account_name, account in list(cluster.accounts.items()):
                if SEGMENT_DEFAULT in account.segments:
                    del account.segments[SEGMENT_DEFAULT]

                if not account.segments:
                    del cluster.accounts[account_name]

            if not cluster.accounts:
                del clusters[cluster_name]

    def _transform_personal_usage(self):
        service_accounts = []
        personal_accounts = []

        for cluster in self.clusters.values():
            for account in cluster.accounts.values():
                if account.type == ACCOUNT_SERVICE:
                    service_accounts.append(account)
                elif account.type == ACCOUNT_PERSONAL:
                    personal_accounts.append(account)

        self.have_service = bool(service_accounts)
        self.have_personal = bool(personal_accounts)

        if not personal_accounts:
            return

        real_usage = Resources()
        for account in service_accounts:
            for segment in account.segments.values():
                if segment.name != 'dev':
                    continue

                m = account.members_count

                if not m:
                    continue

                li = segment.limits

                real_usage += Resources(
                    cpu=li.cpu / m,
                    mem=li.mem / m,
                    ssd=li.ssd / m,
                    hdd=li.hdd / m,
                    ssd_io=li.ssd_io / m,
                    hdd_io=li.hdd_io / m,
                    internet_adress=li.internet_adress / m,
                )

        for account in personal_accounts:
            for segment in account.segments.values():
                real_usage += segment.personal_usage

        for account in personal_accounts:
            for segment in account.segments.values():
                assert isinstance(segment, PersonalSegment)
                segment.personal_usage = real_usage

    def _transform_set_available_presets(self, idm_state, have_oauth_token, robot_accounts):
        jupyter_presets = JupyterPresets(idm_state=idm_state)

        for cluster in self.jupyter_clusters.values():
            for account in cluster.accounts.values():
                for segment in account.segments.values():
                    segment.set_presets(jupyter_presets)

        for cluster in self.clusters.values():
            for account in cluster.accounts.values():
                have_access = (account.name in robot_accounts) or have_oauth_token
                user_presets = UserPresets.from_account(
                    have_access=have_access,
                    account_name=account.name,
                )

                for segment in account.segments.values():
                    segment.set_presets(user_presets)

    async def _transform_set_account_names(self, abc_client):
        accounts = []
        qyp_ids = {}

        for quota in (self.clusters, self.jupyter_clusters):
            for cluster in quota.values():
                for account in cluster.accounts.values():
                    accounts.append(account)

                    # common name is "abc:service:2142" -> Jupyter in the Cloud
                    abc_id = abc_client.parse_account_id(account.name)
                    if abc_id:
                        qyp_ids[account.name] = abc_id

        coroutines = []
        abc_ids = []
        for abc_id in qyp_ids.values():
            abc_ids.append(abc_id)
            coroutines.append(abc_client.get_service_name_by_id(abc_id))

        abc_services = dict(
            zip(
                abc_ids,
                await asyncio.gather(*coroutines),
            ),
        )

        for account in accounts:
            abc_id = qyp_ids.get(account.name)
            if not abc_id:
                continue

            account.abc_id = abc_id
            account.abc_link = abc_client.get_service_link(abc_id)

            name_info = abc_services.get(abc_id, {})
            if not name_info:
                continue

            if 'ru' in name_info:
                account.name_ru = name_info.get('ru')

            if 'en' in name_info:
                account.name_en = name_info.get('en')

    def _transform_separate_clusters(self):
        self.personal_clusters = {}
        for name, cluster in self.clusters.items():
            accounts = cluster.get_personal_accounts()
            if accounts:
                self.personal_clusters[name] = Cluster(
                    name=name,
                    accounts=accounts,
                )

        self.service_clusters = {}
        for name, cluster in self.clusters.items():
            accounts = cluster.get_service_accounts()
            if accounts:
                self.service_clusters[name] = Cluster(
                    name=name,
                    accounts=accounts,
                )

        self.clusters = {}

    def _transorm_grant_access_links(self, oauth_url):
        oauth_message = (
            'Missing oauth token, you can grant access '
            f'<a href={oauth_url}>via link</a>'
        )

        for cluster in self.clusters.values():
            for account in cluster.accounts.values():
                for segment in account.segments.values():
                    if segment.presets.have_access:
                        continue

                    for preset in segment.presets.all.values():
                        preset.add_note('warning', oauth_message)

    def as_dict(self):
        dct = super().as_dict()
        dct.pop('clusters')
        return dct

    def as_json(self, indent=None):
        return json.dumps(
            self.as_dict(),
            indent=indent,
        )
