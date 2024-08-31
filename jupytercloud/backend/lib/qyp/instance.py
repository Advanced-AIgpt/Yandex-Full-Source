import enum

from functools import cached_property
from traitlets import (
    Bool, CaselessStrEnum, Dict, HasTraits, Integer, List, Unicode, UseEnum, default,
)

from jupytercloud.backend.lib.util.format import pretty_float
from jupytercloud.backend.lib.util.misc import HasTraitsReprMixin


MB = 1024 ** 2
GB = 1024 ** 3
CPU_MULT = 1000
QYP_DISK_RESERVE = 6 * GB

SSD = 'ssd'
HDD = 'hdd'
DISK_TYPES = (SSD, HDD)

SEGMENT_DEFAULT = 'default'
SEGMENT_DEV = 'dev'
SEGMENTS_PERMITTED = (SEGMENT_DEFAULT, SEGMENT_DEV)


class InternetAcces(enum.Enum):
    no_access = 1
    nat64 = 2
    ipv4 = 3


def pretty_int(value, denominator):
    if value % denominator:
        return pretty_float(value / denominator)
    return value // denominator


class Instance(HasTraits, HasTraitsReprMixin):
    cpu = Integer()

    # 32 is pretty number
    # potentially we limited with physical cores
    # and have ~200 cores machines in production
    cpu_limit = Integer(32 * CPU_MULT)

    mem = Integer()
    disk_type = CaselessStrEnum(DISK_TYPES, SSD)
    disk_size = Integer()
    disk_reserved_size = Integer(QYP_DISK_RESERVE)
    io_guarantee = Integer(allow_none=True)
    internet_access = UseEnum(InternetAcces, InternetAcces.nat64)
    internet_access = CaselessStrEnum(('no_access', 'nat64', 'ipv6_ipv4_tunnel'), 'nat64')

    name = Unicode()

    @default('name')
    def _name_default(self):
        return f'cpu{self.cpu_pretty}_ram{self.mem_pretty}_{self.disk_type}{self.disk_size_pretty}'

    @property
    def readable_name(self):
        return (
            f'{self.cpu_pretty} cores, '
            f'{self.mem_pretty} GB mem, '
            f'{self.disk_total_size_pretty} GB {self.disk_type}, '
            f'{self.io_guarantee_pretty} MB/s {self.disk_type} IO'
        )

    @property
    def disk_total_size(self):
        return self.disk_size + self.disk_reserved_size

    @property
    def cpu_pretty(self):
        return pretty_int(self.cpu, CPU_MULT)

    @property
    def mem_pretty(self):
        return pretty_int(self.mem, GB)

    @property
    def disk_size_pretty(self):
        return pretty_int(self.disk_size, GB)

    @property
    def io_guarantee_pretty(self):
        return pretty_int(self.io_guarantee, MB)

    @cached_property
    def disk_reserved_size_pretty(self):
        return pretty_int(self.disk_reserved_size, GB)

    @property
    def disk_total_size_pretty(self):
        return pretty_int(self.disk_total_size, GB)

    def update(self, **kwargs):
        new = self.__class__(**self.get_trait_values())
        for arg, value in kwargs.items():
            setattr(new, arg, value)

        return new

    def copy(self):
        return self.update()

    def with_io(self, segment_name):
        if segment_name == SEGMENT_DEFAULT:
            guarantee = 30 if self.disk_type == SSD else 15
        else:
            if segment_name not in (SEGMENT_DEV, ):
                pass  # TODO: add logging here

            guarantee = (
                (self.cpu / CPU_MULT) *
                (18 if self.disk_type == SSD else 3)
            )

        return self.update(
            io_guarantee=int(guarantee * MB)
        )


def _default_instances(default_sizes, *, personal, disk_types):
    result = {}

    for cpu, mem, disk_size in default_sizes:
        for disk_type in disk_types:
            # XXX: hack for increasing disk_size without renaming
            # instances and idm roles >_<
            name = f'cpu{cpu}_ram{mem}_{disk_type}{disk_size}'
            instance = Instance(
                name=name,
                cpu=cpu * CPU_MULT,
                mem=mem * GB,
                disk_size=disk_size * GB * 3,
                disk_type=disk_type,
                # io_guarantee filled later in UserQuota._transform_set_available_presets
                # with depending on segment type
                io_guarantee=None,
                disk_reserved_size=(0 if personal else QYP_DISK_RESERVE)
            )
            result[instance.name] = instance

    return result


JUPYTER_INSTANCES = _default_instances(
    [
        (1, 4, 24),
        (2, 16, 48),
        (4, 32, 96),
    ],
    personal=False,
    disk_types=[SSD],
)


SERVICE_INSTANCES = _default_instances(
    [
        (1, 4, 24),
        (2, 16, 48),
        (4, 32, 96),
        (8, 50, 100),
    ],
    personal=False,
    disk_types=[SSD],
)

PERSONAL_INSTANCES = _default_instances(
    [
        (1, 4, 24),
        (2, 16, 48),
        (4, 32, 96),
        (8, 50, 100),
    ],
    personal=True,
    disk_types=[SSD],
)


class InstancePreset(Instance):
    available = Bool(allow_none=False)
    notes = List(Dict)

    def add_note(self, level, text):
        self.notes.append({
            'level': level,
            'text': text,
        })

    def add_notes(self, level, notes):
        for note in notes:
            self.add_note(level, note)

    @classmethod
    def from_instance(cls, instance, available, notes=None):
        return cls(
            available=available,
            notes=notes or [],
            **instance.get_trait_values(),
        )


class BasePresets(HasTraits, HasTraitsReprMixin):
    all = Dict()

    @classmethod
    def get_missing_resources(cls, instance, available):
        messages = []

        disk_available = (
            available.ssd if instance.disk_type == SSD else available.hdd
        ) - instance.disk_total_size

        if disk_available < 0:
            msg = 'Missing {} GB {} for disk quota'.format(
                pretty_int(-1 * disk_available, GB),
                instance.disk_type,
            )
            if instance.disk_reserved_size:
                msg = '{} (NB: QYP reserves additional {} GB for itself)'.format(
                    msg,
                    instance.disk_reserved_size_pretty,
                )

            messages.append(msg)

        io_available = (
            available.ssd_io if instance.disk_type == SSD else available.hdd_io
        ) - instance.io_guarantee

        if io_available < 0:
            messages.append(
                'Missing {} MB/s {} for io guarantee quota'.format(
                    pretty_int(-1 * io_available, MB), instance.disk_type,
                ),
            )

        if available.cpu < instance.cpu:
            messages.append(
                'Missing {} cores for CPU quota'.format(
                    pretty_int(instance.cpu - available.cpu, CPU_MULT),
                ),
            )

        if available.mem < instance.mem:
            messages.append(
                'Missing {} GB for memory quota'.format(
                    pretty_int(instance.mem - available.mem, GB),
                ),
            )

        return messages

    @classmethod
    def get_available(cls, segment, instances, available_resources):
        result = {}
        for name, raw_instance in instances.items():
            instance = (
                raw_instance.copy()
                if raw_instance.io_guarantee
                else raw_instance.with_io(segment.name)
            )

            if segment.can_contain_instance(instance):
                preset = InstancePreset.from_instance(
                    instance=instance,
                    available=True,
                )
            else:
                preset = InstancePreset.from_instance(
                    instance=instance,
                    available=False,
                )
                preset.add_notes(
                    'danger',
                    cls.get_missing_resources(instance, available_resources),
                )

            if segment.name not in SEGMENTS_PERMITTED:
                preset.available = False
                preset.add_note(
                    'danger',
                    f'QYP segments except "{SEGMENT_DEV}" and "{SEGMENT_DEFAULT}" '
                    f'are not supported (this one is "{segment.name}")'
                )

            if preset.disk_type == HDD:
                preset.add_note('warning', 'VM with HDD would have perfomance issues')

            result[name] = preset

        return cls(all=result)

    def get(self, name):
        instance = self.all.get(name)
        if not instance:
            raise ValueError(f'trying to get unknown instance {name} by name')

        return instance


class JupyterPresets(BasePresets):
    idm_state = Dict(allow_none=False)

    @default('all')
    def _all_default(self):
        return JUPYTER_INSTANCES

    def get_available(self, segment, instances, available_resources):
        presets = super().get_available(segment, instances, available_resources)

        presets.idm_state = self.idm_state

        instance_roles = set()

        for idm_info in self.idm_state.values():
            role_info = idm_info['role']
            if role_info['role'] == 'quota':
                instance_name = role_info['vm']
                instance_roles.add(instance_name)

        for name, preset in presets.all.items():
            if name not in instance_roles:
                preset.available = False
                preset.add_note('danger', 'Missing role for this type of instance')

        return presets


class UserPresets(BasePresets):
    have_access = Bool()
    personal_all = Dict()

    @default('personal_all')
    def _personal_all_default(self):
        return PERSONAL_INSTANCES

    @default('all')
    def _all_default(self):
        return SERVICE_INSTANCES

    @classmethod
    def from_account(cls, *, have_access, account_name):
        from .special_instances import SPECIAL_INSTANCES

        return cls(
            have_access=have_access,
            all=SPECIAL_INSTANCES.get(account_name, SERVICE_INSTANCES),
        )

    def get(self, name):
        return (
            self.personal_all.get(name) or
            super().get(name)
        )

    def get_available(self, segment, instances, available_resources):
        presets = super().get_available(segment, instances, available_resources)
        presets.have_access = self.have_access

        if not self.have_access:
            for preset in presets.all.values():
                preset.available = False

        return presets
