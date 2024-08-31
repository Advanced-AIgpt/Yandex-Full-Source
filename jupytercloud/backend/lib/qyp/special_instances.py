from .instance import CPU_MULT, GB, Instance, MB


def _instance(cpu, mem, disk_size, disk_type='ssd', io_guarantee=None):
    return Instance(
        name=f'cpu{cpu}_ram{mem}_{disk_type}{disk_size}',
        cpu=cpu * CPU_MULT,
        mem=mem * GB,
        disk_size=disk_size * GB,
        disk_type=disk_type,
        # io_guarantee filled later in UserQuota._transform_set_available_presets
        # with depending on segment type
        io_guarantee=io_guarantee,
    )


def _dict(*instances):
    return {instance.name: instance for instance in instances}


SPECIAL_INSTANCES = {
    # jupytercloud (service quota)
    # needed for test reasons
    # 'abc:service:2142': _dict(
    #     _instance(1, 4, 72),
    #     _instance(2, 16, 144),
    #     _instance(4, 32, 288),
    #     _instance(6, 24, 120),
    # ),
    # taxi jupyter DWH
    'abc:service:33985': _dict(
        _instance(6, 24, 120, io_guarantee=54 * MB),
    ),
    # taxi quota yp jupyte rany
    'abc:service:36601': _dict(
        _instance(2, 8, 70, io_guarantee=18 * MB),
    ),
}
