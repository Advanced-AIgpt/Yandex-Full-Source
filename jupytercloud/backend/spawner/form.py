import itertools

from jinja2 import Environment
from jupyterhub.handlers.static import ResourceLoader
from traitlets import Dict, Instance, Unicode, default, observe
from traitlets.config import LoggingConfigurable

from jupytercloud.backend.settings import parse_settings
from jupytercloud.backend.static import get_templates_path


def _parse_user_options(user_options, options_list, defaults=None):
    if not user_options:
        raise ValueError('empty user options')

    defaults = defaults or {}

    result = {}
    for user_option in options_list:
        raw_values = user_options.get(user_option)
        if not raw_values:
            if user_option in defaults:
                result[user_option] = defaults[user_option]
                continue

            raise ValueError(f'no value for {user_option} option')

        if not isinstance(raw_values, (list, tuple)):
            raw_values = [raw_values]

        if len(raw_values) > 1:
            raise ValueError(f'too many values for {user_option} option')

        value = raw_values[0]

        result[user_option] = value

    return result


def parse_old_user_options(*, user_options, default_network):
    """
    NB: Нам нужно сохранять совместимость со старой схемой опций.

    Из доки Spawner.options_from_form:

    user_options are persisted in the JupyterHub database to be reused
    on subsequent spawns if no options are given.
    user_options is serialized to JSON as part of this persistence
    (with additional support for bytes in case of uploaded file data),
    and any non-bytes non-jsonable values will be replaced with None
    if the user_options are re-used.
    """
    USER_OPTIONS = ['instance', 'force_new']
    DEFAULTS = {
        'force_new': 'False',
    }
    INSTANCE_OPTIONS = [
        'cluster', 'account_id', 'node_segment', 'instance_type', 'use_user_token',
    ]

    result = _parse_user_options(
        user_options,
        USER_OPTIONS,
        DEFAULTS,
    )

    result['instance'] = {
        k: v
        for k, v in
        zip(INSTANCE_OPTIONS, result['instance'].split(';'))
    }

    result['force_new'] = result['force_new'] == 'True'
    result['network_id'] = default_network

    return result


def parse_user_options(*, user_options, default_network):
    if 'instance' in user_options:
        return parse_old_user_options(
            user_options=user_options,
            default_network=default_network,
        )

    if 'start_existing' in user_options:
        result = _parse_user_options(user_options, ['start_existing'])
        result['start_existing'] = result['start_existing'] == '1'
        result['settings'] = parse_settings(user_options, prefix='setting-')
        return result

    OPTIONS = [
        'cluster', 'account', 'segment', 'preset', 'quota_type', 'network_id',
    ]
    result = _parse_user_options(user_options, OPTIONS)
    result['settings'] = parse_settings(user_options, prefix='setting-')

    instance = result['instance'] = {}
    instance['cluster'] = result.pop('cluster')
    instance['account_id'] = result.pop('account')
    instance['node_segment'] = result.pop('segment')
    instance['instance_type'] = result.pop('preset')

    return result


def group_resources_by(resources, key_func):
    sorted_resources = sorted(resources, key=key_func)
    return itertools.groupby(sorted_resources, key=key_func)


class SpawnerForm(LoggingConfigurable):
    idm_url = Unicode(config=True)
    networks_doc_url = Unicode(config=True)
    template_path = Unicode(config=True)
    template_vars = Dict(allow_none=True)

    oauth_url = Unicode()

    environment = Instance(Environment)

    @default('template_path')
    def _template_path_default(self):
        return get_templates_path()

    @default('environment')
    def _environment_default(self):
        loader = ResourceLoader([
            self.template_path,
        ])
        return Environment(
            loader=loader,
        )

    @observe('template_path')
    def _template_path_changed(self, _):
        self.environment = self._environment_default()

    def get_template(self, name):
        return self.environment.get_template(name)

    def render_template(self, name, **kwargs):
        template_ns = {}
        if self.template_vars:
            template_ns.update(self.template_vars)

        template_ns.update(**kwargs)

        template = self.get_template(name)
        return template.render(**template_ns)

    async def render(
        self,
        *,
        url,
        user_quota,
        available_networks,
        backup_configurable,
        default_network,
        settings_registry,
    ):
        user = user_quota.user

        idm_url = self.idm_url.format(username=user)
        backup_url = backup_configurable.get_base_url(user)

        if backup_configurable.is_next_url_restore(user):
            restoring_from_backup = True
            last_backup = None
        else:
            backups = await backup_configurable.get_backups(user, limit=1)
            last_backup = backups[0] if backups else None
            restoring_from_backup = False

        return self.render_template(
            'spawn/full.html',
            url=url,
            user_quota=user_quota,
            available_networks=available_networks,
            default_network=default_network,
            oauth_url=self.oauth_url,
            idm_url=idm_url,
            networks_doc_url=self.networks_doc_url,
            last_backup=last_backup,
            backup_url=backup_url,
            restoring_from_backup=restoring_from_backup,
            settings_registry=settings_registry,
        )


class ExistingVMSpawnerForm(SpawnerForm):
    async def render(self, *, url, existing_vm, settings_registry):
        return self.render_template(
            'spawn/existing_vm.html',
            existing_vm=existing_vm,
            url=url,
            settings_registry=settings_registry,
        )


class QYPUnavailableSpawnerForm(SpawnerForm):
    qyp_infra_link = Unicode(
        'https://infra.yandex-team.ru/timeline?preset=YHoLqEASGGg',
        config=True,
    )

    async def render(self, url, exception, traceback):
        return self.render_template(
            'spawn/qyp_unavailable.html',
            url=url,
            exception=exception,
            qyp_infra_link=self.qyp_infra_link,
            traceback=traceback,
        )
