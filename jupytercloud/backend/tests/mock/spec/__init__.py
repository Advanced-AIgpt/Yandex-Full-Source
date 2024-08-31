import json
import pathlib
import requests

from dataclasses import dataclass, asdict

JUPYTERHUB_SPEC_FILE = pathlib.Path('jupyterhub_spec.json')


@dataclass(frozen=True, kw_only=True)
class JupyterHubSpec:
    hub_port: int
    hub_ip: str
    public_port: int
    traefik_port: int
    hub_prefix: str

    stdout_log: str
    stderr_log: str

    def asdict(self):
        return asdict(self)

    def to_json(self):
        return json.dumps(
            self.asdict(),
            sort_keys=True,
            indent=2
        )

    def dump(self):
        if JUPYTERHUB_SPEC_FILE.exists():
            raise RuntimeError(
                f'{JUPYTERHUB_SPEC_FILE} already exists at {pathlib.Path.cwd()}'
            )

        data = self.to_json()
        JUPYTERHUB_SPEC_FILE.write_text(data)

    @classmethod
    def load(cls):
        raw = JUPYTERHUB_SPEC_FILE.read_text()
        data = json.loads(raw)
        return cls(**data)

    def request(
        self,
        method,
        uri,
        *,
        data=None,
        **kwargs
    ):
        prefix = self.hub_prefix.strip('/')
        url = f'http://{self.hub_ip}:{self.public_port}/{prefix}/' + uri.lstrip('/')

        if data:
            assert not kwargs.get('body')
            kwargs['body'] = json.dumps(data)

        return requests.request(
            method=method,
            url=url,
            **kwargs
        )
