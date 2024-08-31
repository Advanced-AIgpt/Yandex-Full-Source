import socket
import jupytercloud.backend.lib.util.config as uc
from jupytercloud.backend.handlers import default_handlers, jc_static_url
from jupytercloud.backend.static import get_templates_path
from jupyterhub_traefik_proxy import TraefikTomlProxy

c = get_config()

hub_ip = '{{ hub_ip }}'
hub_port = {{ hub_port }}
hub_prefix = '{{ hub_prefix }}'
public_port = {{ public_port }}
traefik_port = {{ traefik_port }}

traefik_api_url = f'http://{hub_ip}:{traefik_port}'

c.JupyterHub.port = public_port
c.JupyterHub.hub_ip = hub_ip
c.JupyterHub.hub_port = hub_port
c.JupyterHub.hub_prefix = hub_prefix
c.JupyterHub.template_paths = [get_templates_path()]
c.JupyterHub.extra_handlers = default_handlers
c.JupyterHub.logo_file = 'resfs/file/jupytercloud/backend/static/resources/logo.svg'

c.JupyterHub.proxy_class = TraefikTomlProxy
c.TraefikProxy.traefik_api_url = traefik_api_url
