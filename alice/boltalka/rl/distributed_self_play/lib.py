import os
import vh
import tarfile
import subprocess


@vh.lazy.world_descriptor(
    vh.MasterSlaveWorldDescriptor(
        ttl=100000,
        num_slaves=3,
        master_ports=dict(queue=vh.PortDescriptor(check=False), model=vh.PortDescriptor(check=False)),
        both_ports=dict(),
        slave_ports=dict(),
        master_hardware=vh.HardwareParams(
            max_ram=15000,
            cpu_guarantee=5000,
            gpu_type=vh.GPUType.CUDA_5_0,
            gpu_count=1,
        ),
        slave_hardware=vh.HardwareParams(
            gpu_type=vh.GPUType.NO_GPU,
            max_ram=100000,
            cpu_guarantee=5000,
        )
    )
)
@vh.lazy.from_annotations
def run_training(name: str, data: vh.File, script: vh.File, state: vh.mkoutput(vh.File, snapshot=True)):
    tmpfs_dir = os.environ.get('TMPFS_PATH', None)
    if tmpfs_dir is not None:
        os.chdir(tmpfs_dir)
    with tarfile.open(script, 'r', dereference=True) as tar:
        tar.extractall()
    with tarfile.open(data, 'r', dereference=True) as tar:
        tar.extractall()
    descr = vh.get_world_descriptor()
    if descr.self.role == vh.HostRole.MASTER:
        subprocess.check_call(['python3.6', 'train.py', '--comment', name, '--path', state, '--master-ip', f'[{descr.self.network.ipv6}]',
                               '--master-port', str(descr.self.network.ports['queue']), '--master-model-port', str(descr.self.network.ports['model'])])
    else:
        master = None
        for host in descr.other_hosts:
            if host.role == vh.HostRole.MASTER:
                master = host
                break
        subprocess.check_call(['python3.6', 'session_sampler.py', '--master-ip', f'[{master.network.ipv6}]', '--master-port', str(master.network.ports['queue']),
                               '--master-model-port', str(master.network.ports['model'])])


@vh.lazy.hardware_params(
    vh.HardwareParams(
        ttl=36000,
        gpu_type=vh.GPUType.NO_GPU,
        max_ram=120000,
        cpu_guarantee=5000,
    )
)
@vh.lazy.from_annotations
def run_generation(idx: vh.mkoption(int), data: vh.File, script: vh.File, res: vh.mkoutput(vh.File)):
    tmpfs_dir = os.environ.get('TMPFS_PATH', None)
    if tmpfs_dir is not None:
        os.chdir(tmpfs_dir)
    with tarfile.open(script, 'r', dereference=True) as tar:
        tar.extractall()
    with tarfile.open(data, 'r', dereference=True) as tar:
        tar.extractall()
    subprocess.check_call(['python3.6', 'session_sampler.py', '--master-ip', 'nikola10.search.yandex.net', '--master-port', '5758',
                           '--master-model-port', '5759'])
    return 1
