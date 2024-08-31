import sys
import os
from urllib.request import urlretrieve
import tarfile
import zipfile
import shutil
import argparse
import textwrap
import hashlib
import warnings

checksums_traefik = {
    "https://github.com/containous/traefik/releases/download/v1.7.5/traefik_linux-amd64": "4417a9d83753e1ad6bdd64bbbeaeb4b279bcc71542e779b7bcb3b027c6e3356e",
    "https://github.com/containous/traefik/releases/download/v1.7.5/traefik_darwin-amd64": "379d4af242743a3fe44b44a1ee6df68ea8332578d85de35f264e062c19fd20a0",
    "https://github.com/containous/traefik/releases/download/v1.7.0/traefik_linux-amd64": "b84cb03e8a175b8b7d1a30246d19705f607c6ae5ee89f2dca7a1adccab919135",
    "https://github.com/containous/traefik/releases/download/v1.7.0/traefik_darwin-amd64": "3000cb9f8ed567e9bc567cce33107f6877f2017c69fae8ac235b51a7a94229bf",
}

checksums_etcd = {
    "https://github.com/etcd-io/etcd/releases/download/v3.3.10/etcd-v3.3.10-linux-amd64.tar.gz": "1620a59150ec0a0124a65540e23891243feb2d9a628092fb1edcc23974724a45",
    "https://github.com/etcd-io/etcd/releases/download/v3.3.10/etcd-v3.3.10-darwin-amd64.tar.gz": "fac4091c7ba6f032830fad7809a115909d0f0cae5cbf5b34044540def743577b",
    "https://github.com/etcd-io/etcd/releases/download/v3.2.25/etcd-v3.3.10-linux-amd64.tar.gz": "8a509ffb1443088d501f19e339a0d9c0058ce20599752c3baef83c1c68790ef7",
    "https://github.com/etcd-io/etcd/releases/download/v3.2.25/etcd-v3.3.10-darwin-amd64.tar.gz": "9950684a01d7431bc12c3dba014f222d55a862c6f8af64c09c42d7a59ed6790d",
}

checksums_consul = {
    "https://releases.hashicorp.com/consul/1.5.0/consul_1.5.0_linux_amd64.zip": "1399064050019db05d3378f757e058ec4426a917dd2d240336b51532065880b6",
    "https://releases.hashicorp.com/consul/1.5.0/consul_1.5.0_darwin_amd64.zip": "b4033ea6871fe6136ee5d940c834be2248463c3ec248dc22370e6d5360931325",
}


def checksum_file(path):
    """Compute the sha256 checksum of a path"""
    hasher = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def install_traefik(prefix, plat, traefik_version):
    traefik_bin = os.path.join(prefix, "traefik")

    traefik_url = (
        "https://github.com/containous/traefik/releases"
        f"/download/v{traefik_version}/traefik_{plat}"
    )

    if os.path.exists(traefik_bin):
        print("Traefik already exists")
        if traefik_url not in checksums_traefik:
            warnings.warn(
                f"Couldn't verify checksum for traefik-v{traefik_version}-{plat}",
                stacklevel=2,
            )
            os.chmod(traefik_bin, 0o755)
            print("--- Done ---")
            return
        else:
            checksum = checksum_file(traefik_bin)
            if checksum == checksums_traefik[traefik_url]:
                os.chmod(traefik_bin, 0o755)
                print("--- Done ---")
                return
            else:
                print(f"checksum mismatch on {traefik_bin}")
                os.remove(traefik_bin)

    print(f"Downloading traefik {traefik_version}...")
    urlretrieve(traefik_url, traefik_bin)

    if traefik_url in checksums_traefik:
        checksum = checksum_file(traefik_bin)
        if checksum != checksums_traefik[traefik_url]:
            raise IOError("Checksum failed")
    else:
        warnings.warn(
            f"Couldn't verify checksum for traefik-v{traefik_version}-{plat}",
            stacklevel=2,
        )

    os.chmod(traefik_bin, 0o755)

    print("--- Done ---")


def install_etcd(prefix, plat, etcd_version):
    etcd_downloaded_dir_name = f"etcd-v{etcd_version}-{plat}"
    etcd_archive_extension = ".tar.gz"
    if "linux" in plat:
        etcd_archive_extension = "tar.gz"
    else:
        etcd_archive_extension = "zip"
    etcd_downloaded_archive = os.path.join(
        prefix, etcd_downloaded_dir_name + "." + etcd_archive_extension
    )
    etcd_binaries = os.path.join(prefix, "etcd_binaries")

    etcd_bin = os.path.join(prefix, "etcd")
    etcdctl_bin = os.path.join(prefix, "etcdctl")

    etcd_url = (
        "https://github.com/etcd-io/etcd/releases/"
        f"/download/v{etcd_version}/etcd-v{etcd_version}-{plat}.{etcd_archive_extension}"
    )

    if os.path.exists(etcd_bin) and os.path.exists(etcdctl_bin):
        print("Etcd and etcdctl already exist")
        if etcd_url not in checksums_etcd:
            warnings.warn(
                f"Couldn't verify checksum for etcd-v{etcd_version}-{plat}",
                stacklevel=2,
            )
            os.chmod(etcd_bin, 0o755)
            os.chmod(etcdctl_bin, 0o755)
            print("--- Done ---")
            return
        else:
            checksum_etcd_archive = checksum_file(etcd_downloaded_archive)
            if checksum_etcd_archive == checksums_etcd[etcd_url]:
                os.chmod(etcd_bin, 0o755)
                os.chmod(etcdctl_bin, 0o755)
                print("--- Done ---")
                return
            else:
                print("checksum mismatch on etcd")
                os.remove(etcd_bin)
                os.remove(etcdctl_bin)
                os.remove(etcd_downloaded_archive)

    if not os.path.exists(etcd_downloaded_archive):
        print(f"Downloading {etcd_downloaded_dir_name} archive...")
        urlretrieve(etcd_url, etcd_downloaded_archive)
    else:
        print(f"Archive {etcd_downloaded_dir_name} already exists")

    if etcd_archive_extension == "zip":
        with zipfile.ZipFile(etcd_downloaded_archive, "r") as zip_ref:
            zip_ref.extract(etcd_downloaded_dir_name + "/etcd", etcd_binaries)
            zip_ref.extract(etcd_downloaded_dir_name + "/etcdctl", etcd_binaries)
    else:
        with (tarfile.open(etcd_downloaded_archive, "r")) as tar_ref:
            print("Extracting the archive...")
            tar_ref.extract(etcd_downloaded_dir_name + "/etcd", etcd_binaries)
            tar_ref.extract(etcd_downloaded_dir_name + "/etcdctl", etcd_binaries)

    shutil.copy(os.path.join(etcd_binaries, etcd_downloaded_dir_name, "etcd"), etcd_bin)
    shutil.copy(
        os.path.join(etcd_binaries, etcd_downloaded_dir_name, "etcdctl"), etcdctl_bin
    )

    if etcd_url in checksums_etcd:
        checksum_etcd_archive = checksum_file(etcd_downloaded_archive)
        if checksum_etcd_archive != checksums_etcd[etcd_url]:
            raise IOError("Checksum failed")
    else:
        warnings.warn(
            f"Couldn't verify checksum for etcd-v{etcd_version}-{plat}", stacklevel=2
        )

    os.chmod(etcd_bin, 0o755)
    os.chmod(etcdctl_bin, 0o755)

    # Cleanup
    shutil.rmtree(etcd_binaries)

    print("--- Done ---")


def install_consul(prefix, plat, consul_version):
    plat = plat.replace("-", "_")
    consul_downloaded_dir_name = f"consul_v{consul_version}_{plat}"
    consul_archive_extension = ".tar.gz"
    consul_archive_extension = "zip"

    consul_downloaded_archive = os.path.join(
        prefix, consul_downloaded_dir_name + "." + consul_archive_extension
    )
    consul_binaries = os.path.join(prefix, "consul_binaries")

    consul_bin = os.path.join(prefix, "consul")

    consul_url = (
        "https://releases.hashicorp.com/consul/"
        f"{consul_version}/consul_{consul_version}_{plat}.{consul_archive_extension}"
    )

    if os.path.exists(consul_bin):
        print("Consul already exists")
        if consul_url not in checksums_consul:
            warnings.warn(
                f"Couldn't verify checksum for consul_v{consul_version}_{plat}",
                stacklevel=2,
            )
            os.chmod(consul_bin, 0o755)
            print("--- Done ---")
            return
        else:
            checksum_consul_archive = checksum_file(consul_downloaded_archive)
            if checksum_consul_archive == checksums_consul[consul_url]:
                os.chmod(consul_bin, 0o755)
                print("--- Done ---")
                return
            else:
                print("checksum mismatch on consul")
                os.remove(consul_bin)
                os.remove(consul_downloaded_archive)

    if not os.path.exists(consul_downloaded_archive):
        print(f"Downloading {consul_downloaded_dir_name} archive...")
        urlretrieve(consul_url, consul_downloaded_archive)
    else:
        print(f"Archive {consul_downloaded_dir_name} already exists")

    with zipfile.ZipFile(consul_downloaded_archive, "r") as zip_ref:
        zip_ref.extract("consul", consul_binaries)

    shutil.copy(os.path.join(consul_binaries, "consul"), consul_bin)

    if consul_url in checksums_consul:
        checksum_consul_archive = checksum_file(consul_downloaded_archive)
        if checksum_consul_archive != checksums_consul[consul_url]:
            raise IOError("Checksum failed")
    else:
        warnings.warn(
            f"Couldn't verify checksum for consul_v{consul_version}_{plat}",
            stacklevel=2,
        )

    os.chmod(consul_bin, 0o755)

    # Cleanup
    shutil.rmtree(consul_binaries)

    print("--- Done ---")


def main():

    parser = argparse.ArgumentParser(
        description="Dependencies intaller",
        epilog=textwrap.dedent(
            """\
            Checksums available for:
            - traefik:
                - v1.7.5-linux-amd64
                - v1.7.5-darwin-amd64
                - v1.7.0-linux-amd64
                - v1.7.0-darwin-amd64
            - etcd:
                - v3.3.10-linux-amd64
                - v3.3.10-darwin-amd64
                - v3.2.25-linux-amd64
                - v3.2.25-darwin-amd64
            - consul:
                - v1.5.0_linux_amd64
                - v1.5.0_darwin_amd64
            """
        ),
        formatter_class=argparse.RawTextHelpFormatter,
    )

    parser.add_argument(
        "--output",
        dest="installation_dir",
        default="./dependencies",
        help=textwrap.dedent(
            """\
            The installation directory (absolute or relative path).
            If it doesn't exist, it will be created.
            If no directory is provided, it defaults to:
            --- %(default)s ---
            """
        ),
    )

    default_platform = sys.platform + "-amd64"

    parser.add_argument(
        "--platform",
        dest="plat",
        default=default_platform,
        help=textwrap.dedent(
            """\
            The platform to download for.
            If no platform is provided, it defaults to:
            --- %(default)s ---
            """
        ),
    )

    parser.add_argument(
        "--traefik",
        action="store_true",
        help=textwrap.dedent(
            """\
            Whether or not to install traefik.
            By default traefik is NOT going to be installed.
            """
        ),
    )

    parser.add_argument(
        "--traefik-version",
        dest="traefik_version",
        default="1.7.5",
        help=textwrap.dedent(
            """\
            The version of traefik to download.
            If no version is provided, it defaults to:
            --- %(default)s ---
            """
        ),
    )

    parser.add_argument(
        "--etcd",
        action="store_true",
        help=textwrap.dedent(
            """\
            Whether or not to install etcd.
            By default etcd is NOT going to be installed.
            """
        ),
    )

    parser.add_argument(
        "--etcd-version",
        dest="etcd_version",
        default="3.3.10",
        help=textwrap.dedent(
            """\
            The version of etcd to download.
            If no version is provided, it defaults to:
            --- %(default)s ---
            """
        ),
    )

    parser.add_argument(
        "--consul",
        action="store_true",
        help=textwrap.dedent(
            """\
            Whether or not to install consul.
            By default consul is NOT going to be installed:
            """
        ),
    )

    parser.add_argument(
        "--consul-version",
        dest="consul_version",
        default="1.5.0",
        help=textwrap.dedent(
            """\
            The version of consul to download.
            If no version is provided, it defaults to:
            --- %(default)s ---
            """
        ),
    )

    args = parser.parse_args()
    deps_dir = args.installation_dir
    plat = args.plat
    traefik_version = args.traefik_version
    etcd_version = args.etcd_version
    consul_version = args.consul_version

    if os.path.exists(deps_dir):
        print("Dependencies directory already exists.")
    else:
        print(f"Creating output directory {deps_dir}...")
        os.makedirs(deps_dir)

    if args.traefik:
        install_traefik(deps_dir, plat, traefik_version)
    if args.etcd:
        install_etcd(deps_dir, plat, etcd_version)
    if args.consul:
        install_consul(deps_dir, plat, consul_version)


if __name__ == "__main__":
    main()
