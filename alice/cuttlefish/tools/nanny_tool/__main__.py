#! /usr/bin/env python3
import urllib.request
import json
import logging
import difflib
import os
import subprocess
import tempfile
import copy
from utils import write_json, confirm_diff, print_diff, apply_patch, make_patch, print_blue, vault_secrets, run_via_ssh, run_command


URL_BASE = "https://nanny.yandex-team.ru/"
TOKEN_PATH = os.path.expanduser("~/.nanny_oauth_token")
TOKEN_ENV = "NANNY_OAUTH_TOKEN"
NANNY_TVM_ID = 2002924
VAULT_URL_BASE = "https://vault-api.passport.yandex.net/1/"

# to get delegation tokens (Vault) this token must provide access to vault:use
# you may get via: https://oauth.yandex-team.ru/authorize?response_type=token&client_id=4d5d0bdcddc845aaa665d634a6c51b47
TOKEN = os.environ.get(TOKEN_ENV)
if TOKEN is None and os.path.exists(TOKEN_PATH):
    with open(TOKEN_PATH, "r") as f:
        TOKEN = f.read().strip()


def _get_delegation_token(sec_id, service_id):
    url = f"{VAULT_URL_BASE}secrets/{sec_id}/tokens/"
    resp = urllib.request.urlopen(
        urllib.request.Request(
            url=url,
            data=json.dumps({
                "tvm_client_id": NANNY_TVM_ID,
                "signature": service_id,
                "comment": f"get delegation token for {service_id} Nanny's service"
            }).encode("utf-8"),
            headers={
                "Authorization": f"OAuth {TOKEN}",
                "Content-Type": "application/json"
            },
            method="POST"
        )
    )
    jresp = json.loads(resp.read())
    if jresp["status"] != "ok":
        raise RuntimeError(f"Vault failed: {jresp}")
    return jresp["token"]


def update_vault_tokens(service_id, runtime_attrs):
    logging.debug(f"Update Vault's delegation tokens for {service_id} service...")
    for s in vault_secrets(runtime_attrs):
        s["delegationToken"] = _get_delegation_token(s["secretId"], service_id)
        logging.info(f"Updated Vault's delegation token of '{s['secretName']}' for service '{service_id}'")


def copy_vault_tokens(src_attrs, dst_attrs):
    src = {s["secretId"]: s for s in vault_secrets(src_attrs)}
    for dst_s in vault_secrets(dst_attrs):
        src_s = src.get(dst_s["secretId"])
        if src_s is not None:
            dst_s["delegationToken"] = src_s["delegationToken"]


def _nanny_request(url, method="GET", data=None):
    url = f"{URL_BASE}{url}"
    logging.debug(f"{method} {url}")
    resp = urllib.request.urlopen(
        urllib.request.Request(
            url=url,
            data=data,
            headers={
                "Authorization": f"OAuth {TOKEN}",
                "Content-Type": "application/json"
            },
            method=method
        )
    )
    return resp.read()


def get_runtime_attrs(service_id=None, snapshot_id=None):
    if snapshot_id is not None:
        url = f"v2/history/services/runtime_attrs/{snapshot_id}/"
    elif service_id is not None:
        url = f"v2/services/{service_id}/runtime_attrs/"
    else:
        raise RuntimeError("Invalid call")

    resp = json.loads(_nanny_request(url))
    return resp


def get_info_attrs(service_id=None, snapshot_id=None):
    if snapshot_id is not None:
        url = f"v2/history/services/info_attrs/{snapshot_id}/"
    elif service_id is not None:
        url = f"v2/services/{service_id}/info_attrs/"
    else:
        raise RuntimeError("Invalid call")

    resp = json.loads(_nanny_request(url))
    return resp


def set_runtime_attrs(service_id, content):
    _nanny_request(
        url=f"v2/services/{service_id}/runtime_attrs/",
        method="PUT",
        data=json.dumps(content).encode("utf-8")
    )


def set_info_attrs(service_id, content):
    _nanny_request(
        url=f"v2/services/{service_id}/info_attrs/",
        method="PUT",
        data=json.dumps(content).encode("utf-8")
    )


def set_instance_spec(service_id, content):
    _nanny_request(
        url=f"v2/services/{service_id}/runtime_attrs/instance_spec/",
        method="PUT",
        data=json.dumps(content).encode("utf-8")
    )


def get_activated_instances(service_id):
    resp = json.loads(_nanny_request(url=f"v2/services/{service_id}/current_state/instances/"))
    return resp


# -------------------------------------------------------------------------------------------------
class RuntimeAttrs:
    @staticmethod
    def set_attrs(service_id, content):
        set_runtime_attrs(service_id, content)

    def __init__(self, service_id=None, snapshot_id=None):
        self._attrs = get_runtime_attrs(service_id=service_id, snapshot_id=snapshot_id)
        self.service_id = self._attrs["service_id"] if (snapshot_id is not None) else service_id
        logging.debug(f"Got runtime attributes of {self.service_id}: id={self.id} author={self.author} comment='{self.comment}'")

    @property
    def id(self):
        return self._attrs["_id"]

    @property
    def author(self):
        return self._attrs["change_info"]["author"]

    @property
    def comment(self):
        return self._attrs["change_info"]["comment"]

    @property
    def content(self):
        return self._attrs["content"]

    @property
    def instance_spec(self):
        return self.content["instance_spec"]

    @property
    def resources(self):
        return self.content["resources"]
    
    @property
    def static_files(self):
        return self.resources["static_files"]

    def materialize_static_file(self, path):
        os.makedirs(path, exist_ok=True)
        for fdesc in self.static_files:
            fpath = os.path.join(path, fdesc["local_path"])
            os.makedirs(os.path.dirname(fpath), exist_ok=True)
            with open(fpath, "w") as f:
                f.write(fdesc["content"])
    
    def get_partial_content(self, *args):
        return {
            arg: self.content[arg] for arg in args
        }

    def get_updated_content(self, update):
        content = copy.deepcopy(self.content)
        content.update(update)
        return content


# -------------------------------------------------------------------------------------------------
class InfoAttrs:
    @staticmethod
    def set_attrs(service_id, content):
        set_info_attrs(service_id, content)

    def __init__(self, service_id=None, snapshot_id=None):
        self._attrs = get_info_attrs(service_id=service_id, snapshot_id=snapshot_id)
        self.service_id = self._attrs["service_id"] if (snapshot_id is not None) else service_id
        logging.debug(f"Got info attributes of {self.service_id}: id={self.id} author={self.author} comment='{self.comment}'")

    @property
    def id(self):
        return self._attrs["_id"]

    @property
    def author(self):
        return self._attrs["change_info"]["author"]

    @property
    def comment(self):
        return self._attrs["change_info"]["comment"]

    @property
    def content(self):
        return self._attrs["content"]

    def get_partial_content(self, *args):
        return {
            arg: self.content[arg] for arg in args
        }

    def get_updated_content(self, update):
        content = copy.deepcopy(self.content)
        content.update(update)
        return content

# -------------------------------------------------------------------------------------------------
class MaterializeStaticFiles:
    """ Materialize a service's static files in a local directory
    """

    @staticmethod
    def add_args(parser):
        parser.add_argument("-s", "--service-id", help="Nanny service ID", metavar="ID", required=True)
        parser.add_argument("-d", "--dst-path", help="Directory for materialized files", metavar="PATH", required=True)


    @staticmethod
    def run(args):
        ra = RuntimeAttrs(service_id=args.service_id)
        ra.materialize_static_file(args.dst_path)


# -------------------------------------------------------------------------------------------------
class DumpInstanceSpec:
    """ Write service's instance spec to a file
    """

    @staticmethod
    def add_args(parser):
        parser.add_argument("-s", "--service-id", help="Nanny service ID", metavar="ID", required=True)
        parser.add_argument("-f", "--dst-file", help="Destination JSON file", metavar="PATH")
        parser.add_argument("--snapshot-id", help="Use snapshot with given ID (not current)", metavar="ID")

    @staticmethod
    def run(args):
        ra = RuntimeAttrs(service_id=args.service_id, snapshot_id=args.snapshot_id)
        if args.dst_file is None:
            args.dst_file = f"{args.service_id}-instance_spec@{ra.id}.json"
        with open(args.dst_file, "w") as f:
            json.dump(ra.instance_spec, f, indent=4, sort_keys=True)


class SetInstanceSpec:
    """ Change instance spec of a service
    """

    @staticmethod
    def add_args(parser):
        parser.add_argument("-s", "--service-ids", help="Nanny service IDs", metavar="ID", required=True, nargs="+")
        parser.add_argument("-f", "--src-file", help="Source JSON file contains instance spec", metavar="PATH", required=True)
        parser.add_argument("-m", "--comment", help="Comment message", metavar="MSG", default="no message")


    @staticmethod
    def run(args):
        with open(args.src_file, "r") as f:
            instance_spec = json.load(f)
        content = {
            "content": instance_spec,
            "comment": args.comment,
        }

        for sid in args.service_ids:
            current = RuntimeAttrs(sid).instance_spec
            if confirm_diff(current, instance_spec):
                set_instance_spec(sid, content)


# -------------------------------------------------------------------------------------------------
class DumpInfoAttrs:
    """ Write service's information attributes to a file
    """

    @staticmethod
    def add_args(parser):
        parser.add_argument("-s", "--service-id", help="Nanny service ID", metavar="ID", required=True)
        parser.add_argument("-f", "--dst-file", help="Destination JSON file", metavar="PATH")
        parser.add_argument("--snapshot-id", help="Use snapshot with given ID (not current)", metavar="ID")

    @staticmethod
    def run(args):
        attrs = InfoAttrs(service_id=args.service_id, snapshot_id=args.snapshot_id)
        if args.dst_file is None:
            args.dst_file = f"{args.service_id}@{attrs.id}_info.json"
        with open(args.dst_file, "w") as f:
            json.dump(attrs.content, f, indent=4, sort_keys=True)


# -------------------------------------------------------------------------------------------------
class DumpRuntimeAttrs:
    """ Write service's runtime attributes to a file
    """

    @staticmethod
    def add_args(parser):
        parser.add_argument("-s", "--service-id", help="Nanny service ID", metavar="ID", required=True)
        parser.add_argument("-f", "--dst-file", help="Destination JSON file", metavar="PATH")
        parser.add_argument("--snapshot-id", help="Use snapshot with given ID (not current)", metavar="ID")

    @staticmethod
    def run(args):
        attrs = RuntimeAttrs(service_id=args.service_id, snapshot_id=args.snapshot_id)
        if args.dst_file is None:
            args.dst_file = f"{args.service_id}@{attrs.id}.json"
        with open(args.dst_file, "w") as f:
            json.dump(attrs.get_partial_content("instance_spec", "resources"), f, indent=4, sort_keys=True)


# -------------------------------------------------------------------------------------------------
class SetRuntimeAttrs:
    """ Change runtime attributes of a service
    """

    @staticmethod
    def add_args(parser):
        parser.add_argument("-s", "--service-ids", help="Nanny service ID", metavar="ID", nargs="+", required=True)
        parser.add_argument("-f", "--src-file", help="Source JSON file contains whole runtime attributes", metavar="PATH", required=True)
        parser.add_argument("-m", "--comment", help="Comment message", metavar="MSG", default="no message")
        parser.add_argument("-u", "--update-vault-tokens", action="store_true")

    @staticmethod
    def run(args):
        with open(args.src_file, "r") as f:
            content = json.load(f)

        for sid in args.service_ids:
            if args.update_vault_tokens:
                update_vault_tokens(service_id=sid, runtime_attrs=content)

            attrs = RuntimeAttrs(sid)
            current_content = attrs.get_partial_content("instance_spec", "resources")

            if not confirm_diff(current_content, content):
                continue

            RuntimeAttrs.set_attrs(sid, {
                "content": current_attrs.get_updated_content(content),
                "comment": args.comment,
                "snapshot_id": current_attrs.id
            })


# -------------------------------------------------------------------------------------------------
class CopyRuntimeAttrs:
    """ Copy runtime attributes of a service
    """

    @staticmethod
    def add_args(parser):
        parser.add_argument("-s", "--src-service-id", help="Source Nanny service ID", metavar="ID", required=True)
        parser.add_argument("-d", "--dst-service-ids", help="Target Nanny services' IDs", metavar="ID", nargs="+", required=True)
        parser.add_argument("-m", "--comment", help="Comment message", metavar="MSG", default="no message")
        parser.add_argument("-u", "--update-vault-tokens", action="store_true", help="Request new tokens for Yandex Vault's secrets")
        parser.add_argument("--no-resources", action="store_true", help="Don not copy resources")
        parser.add_argument("--no-instance-spec", action="store_true", help="Don not copy instance spec")

    @staticmethod
    def run(args):
        if args.comment is None:
            args.comment = f"Copy runtime attributes from {args.src_service_id}"

        attrs = RuntimeAttrs(service_id=args.src_service_id)

        components = []
        if not args.no_resources:
            components.append("resources")
        if not args.no_instance_spec:
            components.append("instance_spec")

        content = attrs.get_partial_content(*components)

        for sid in args.dst_service_ids:
            current_attrs = RuntimeAttrs(sid)
            current_content = current_attrs.get_partial_content(*components)

            if args.update_vault_tokens:
                logging.debug(f"Request new token for Vault's secrects")
                update_vault_tokens(service_id=sid, runtime_attrs=content)
            else:
                logging.debug(f"Try to preserve existing Vault's tokens...")
                copy_vault_tokens(src_attrs=current_content, dst_attrs=content)

            if not confirm_diff(current_content, content, title=f"Patch for {sid}"):
                continue

            RuntimeAttrs.set_attrs(sid, {
                "content": current_attrs.get_updated_content(content),
                "comment": args.comment,
                "snapshot_id": current_attrs.id
            })


# -------------------------------------------------------------------------------------------------
class CopyInfoAttrs:
    """ Copy information attributes of a service
    """

    @staticmethod
    def add_args(parser):
        parser.add_argument("-s", "--src-service-id", help="Source Nanny service ID", metavar="ID", required=True)
        parser.add_argument("-d", "--dst-service-ids", help="Target Nanny services' IDs", metavar="ID", nargs="+", required=True)
        parser.add_argument("-m", "--comment", help="Comment message", metavar="MSG")
        parser.add_argument("components", choices=["tickets_integration", "labels"], nargs="+")

    @staticmethod
    def run(args):
        if args.comment is None:
            args.comment = f"Copy info attributes from {args.src_service_id}"

        attrs = InfoAttrs(service_id=args.src_service_id)
        content = attrs.get_partial_content(*args.components)

        for sid in args.dst_service_ids:
            current_attrs = InfoAttrs(sid)
            current_content = current_attrs.get_partial_content(*args.components)

            if not confirm_diff(current_content, content, title=f"Patch for {sid}"):
                continue

            InfoAttrs.set_attrs(sid, {
                "content": current_attrs.get_updated_content(content),
                "comment": args.comment,
                "snapshot_id": current_attrs.id
            })


# -------------------------------------------------------------------------------------------------



# -------------------------------------------------------------------------------------------------
class DiffRuntimeAttrs:
    """ Show runtime attributes' diff
    """

    @staticmethod
    def add_args(parser):
        parser.add_argument("-s", "--service-ids", help="Source Nanny service ID", metavar="ID", nargs=2, required=True)

    @staticmethod
    def run(args):
        attrs1 = RuntimeAttrs(service_id=args.service_ids[0])
        attrs2 = RuntimeAttrs(service_id=args.service_ids[1])

        content1 = attrs1.get_partial_content("instance_spec", "resources")
        content2 = attrs2.get_partial_content("instance_spec", "resources")

        print_diff(content1, content2)


# -------------------------------------------------------------------------------------------------
class PatchRuntimeAttrs:
    """ Apply given patch o runtime attributes of given services
    """

    @staticmethod
    def add_args(parser):
        parser.add_argument("-s", "--service-ids", help="Nanny service ID", metavar="ID", nargs="+", required=True)
        parser.add_argument("-p", "--patch", help="Patch file", metavar="PATH", required=True)
        parser.add_argument("-m", "--comment", help="Comment message", metavar="MSG", default="no message")

    @staticmethod
    def run(args):
        for sid in args.service_ids:
            current = RuntimeAttrs(sid)
            patched = apply_patch(current.content, args.patch)
            if not confirm_diff(current.content, patched):
                continue
            set_runtime_attrs(sid, {
                "content": patched,
                "comment": args.comment,
                "snapshot_id": current.id
            })


# -------------------------------------------------------------------------------------------------
class ApplyDiff:
    """ Find difference between runtime attributes of two snapshots and apply it as a patch
    """

    @staticmethod
    def add_args(parser):
        parser.add_argument("-s", "--service-ids", help="Nanny service ID", metavar="ID", nargs="+", required=True)
        parser.add_argument("-d", "--snapshot-diff", help="Take diff from snapshots", metavar="PATH", nargs="+")
        parser.add_argument("-m", "--comment", help="Comment message", metavar="MSG", default="no message")
        parser.add_argument("--dry", help="Dry run", action="store_true")
        parser.add_argument("--info-attrs", action="store_true")

    @staticmethod
    def run(args):
        if len(args.snapshot_diff) > 2:
            raise RuntimeError("Too many snapshots to get diff")
        attr_class = InfoAttrs if args.info_attrs else RuntimeAttrs

        a0 = attr_class(snapshot_id=args.snapshot_diff[0])
        if len(args.snapshot_diff) == 2:
            a1 = attr_class(snapshot_id=args.snapshot_diff[1])
        else:
            a1 = attr_class(service_id=a0.service_id)

        with tempfile.TemporaryDirectory() as dname:
            patch_fname = os.path.join(dname, "my_patch")
            with open(patch_fname, "w") as f:
                make_patch(a0.content, a1.content, f)
            for sid in args.service_ids:
                current = attr_class(sid)
                patched = apply_patch(current.content, patch_fname)
                if not confirm_diff(current.content, patched, title=f"-- Patch for {sid}"):
                    logging.warning(f"Changes aren't applied to {sid}")
                    continue
                if args.dry:
                    logging.debug("Don't apply changes due to dry run")
                    continue
                attr_class.set_attrs(sid, {
                    "content": patched,
                    "comment": args.comment,
                    "snapshot_id": current.id
                })


# -------------------------------------------------------------------------------------------------
class ShowVaultTokens:
    """ Find difference between runtime attributes of two snapshots and apply it as a patch
    """

    @staticmethod
    def add_args(parser):
        parser.add_argument("-s", "--secret-id", metavar="ID", required=True)

    @staticmethod
    def run(args):
        url = f"{VAULT_URL_BASE}secrets/{args.secret_id}/tokens/"
        resp = urllib.request.urlopen(
            urllib.request.Request(
                url=url,
                headers={
                    "Authorization": f"OAuth {TOKEN}",
                    "Content-Type": "application/json"
                }
            )
        )
        jresp = json.loads(resp.read())
        print(json.dumps(jresp, indent=4))


# -------------------------------------------------------------------------------------------------
class RunViaSsh:
    """ Run command on hosts via SSH
    """

    @staticmethod
    def add_args(parser):
        parser.add_argument("--hosts", nargs="+")
        parser.add_argument("-s", "--service-ids", nargs="+")
        parser.add_argument("-c", "--command", help="Command to be executed on remote hosts", required=True)

    @staticmethod
    def run(args):
        hosts = args.hosts if args.hosts is not None else []
        if args.service_ids is not None:
            for sid in args.service_ids:
                hosts += [i["container_hostname"] for i in get_activated_instances(sid)["result"]]

        logging.info(f"Run command '{args.command}' on {len(hosts)} hosts...")
        run_via_ssh(cmd=args.command, hosts=hosts)


# -------------------------------------------------------------------------------------------------
class ForEach:
    """ Run bash script for each host of a service
    """

    @staticmethod
    def add_args(parser):
        parser.add_argument("--hosts", nargs="+")
        parser.add_argument("-s", "--service-ids", nargs="+")
        parser.add_argument("-P", "--parallel", help="Run this number of commands simultaneously", type=int, default=10)
        parser.add_argument("-c", "--command", help="Command to be executed for each hosts", required=True)

    @staticmethod
    def run(args):
        hosts = args.hosts if args.hosts is not None else []
        if args.service_ids is not None:
            for sid in args.service_ids:
                hosts += [i["container_hostname"] for i in get_activated_instances(sid)["result"]]

        logging.info(f"Run script '{args.command}' for {len(hosts)} hosts...")
        run_command(cmd=args.command, hosts=hosts, parallel_count=args.parallel)


# -------------------------------------------------------------------------------------------------
def format_action_name(name):
    res = "" + name[0].lower()
    for l in name[1:]:
        if l.isupper():
            res += "-"
        res += l.lower()
    return res


def main():
    import argparse
    global TOKEN

    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbose", action="store_true", help="Enable debug logs")
    parser.add_argument("-t", "--token", help="OAuth token for Nanny")
    subparsers = parser.add_subparsers(title="Action")
    for action in (
        MaterializeStaticFiles,
        DumpRuntimeAttrs,
        DumpInfoAttrs,
        SetRuntimeAttrs,
        CopyRuntimeAttrs,
        CopyInfoAttrs,
        DumpInstanceSpec,
        SetInstanceSpec,
        PatchRuntimeAttrs,
        ApplyDiff,
        ShowVaultTokens,
        DiffRuntimeAttrs,
        RunViaSsh,
        ForEach
    ):
        action_parser = subparsers.add_parser(
            name=format_action_name(action.__name__),
            help=action.__doc__.strip() if action.__doc__ else None
        )
        action.add_args(action_parser)
        action_parser.set_defaults(action=action)

    args = parser.parse_args()

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)
    if args.token is not None:
        TOKEN = args.token

    if TOKEN is None:
        print("OAuth token is not set")
        exit(1)

    args.action.run(args)


if __name__ == "__main__":
    main()



