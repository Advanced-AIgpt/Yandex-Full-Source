from jupytercloud.backend.lib.qyp.quota import UserQuota, Resources
from jupytercloud.backend.lib.qyp.instance import SEGMENT_DEFAULT

from .base import JCAPIHandler, JCPageHandler
from .utils import NAME_RE

JUPYTER_USER = 'robot-jupyter-cloud'
JUPYTER_ACCOUNT_ID = 'abc:service:2142'


class BaseQuotaHandler:
    async def get_accounts(self, name):
        idm_state = self.jupyter_cloud_db.get_patched_user_idm_state(name)
        oauth = self.get_user_oauth(user=name)

        return await UserQuota.request(
            qyp_client=self.qyp_client,
            abc_client=self.abc_client,
            user=name,
            jupyter_user=JUPYTER_USER,  # todo: move to config
            jupyter_account_id=JUPYTER_ACCOUNT_ID,
            idm_state=idm_state,
            have_oauth_token=oauth.present,
            oauth_url=oauth.get_oauth_url(),
        )


class QYPQuotaHandler(JCAPIHandler, BaseQuotaHandler):
    async def get(self, name):
        accounts = await self.get_accounts(name)
        self.write(accounts.as_dict())


class QYPQuotaServicesHandler(JCPageHandler, BaseQuotaHandler):
    async def get(self, name):
        user = self.user_from_username(name)
        user_accounts = await self.get_accounts(name)

        accounts = {}

        for cluster_name, cluster in user_accounts.service_clusters.items():
            for account_name, account in cluster.accounts.items():
                accounts[account_name] = account.readable_name

        html = await self.render_template(
            'quota_services.html',
            accounts=accounts,
            user=user,
        )

        self.write(html)


class QYPQuotaCapacityHandler(JCAPIHandler, BaseQuotaHandler):
    def pretty_resources(self, resources):
        r = resources
        return dict(
            cpu=f'{r.cpu_pretty} cores',
            mem=f'{r.mem_pretty} GB',
            ssd=f'{r.ssd_pretty} GB',
            ssd_io=f'{r.ssd_io_pretty} MB/s'
        )

    def calc_stats(self, segment, preset):
        a = segment.available_resources
        instances_fit = dict(
            by_cpu=a.cpu // preset.cpu,
            by_mem=a.mem // preset.mem,
            by_ssd=a.ssd // preset.disk_total_size,
            by_ssd_io=a.ssd_io // preset.io_guarantee
        )

        min_ = min(instances_fit.values())
        max_ = max(instances_fit.values())
        instances_fit['total'] = min_

        cpu = instances_fit['by_cpu'] * preset.cpu
        mem = instances_fit['by_mem'] * preset.mem
        ssd = instances_fit['by_ssd'] * preset.disk_total_size
        ssd_io = instances_fit['by_ssd_io'] * preset.io_guarantee

        resources_extra = Resources(
            cpu=cpu - preset.cpu * min_,
            mem=mem - preset.mem * min_,
            ssd=ssd - preset.disk_total_size * min_,
            ssd_io=ssd_io - preset.io_guarantee * min_,
            hdd=0,
            hdd_io=0,
            internet_access=0,
        )

        resources_lack = Resources(
            cpu=preset.cpu * max_ - cpu,
            mem=preset.mem * max_ - mem,
            ssd=preset.disk_total_size * max_ - ssd,
            ssd_io=preset.io_guarantee * max_ - ssd_io,
            hdd=0,
            hdd_io=0,
            internet_access=0,
        )

        return dict(
            instances_fit=instances_fit,
            resources_extra=self.pretty_resources(resources_extra),
            resources_lack=self.pretty_resources(resources_lack),
            instance_usage=preset.readable_name,
        )

    async def get(self, name, abc_id):
        user_accounts = await self.get_accounts(name)

        result = {}

        for cluster_name, cluster in user_accounts.service_clusters.items():
            result_cluster = result.setdefault(cluster_name, {})
            for account_name, account in cluster.accounts.items():
                if account_name != abc_id:
                    continue

                for segment_name, segment in account.segments.items():
                    if segment_name == SEGMENT_DEFAULT and account_name == JUPYTER_ACCOUNT_ID:
                        continue

                    result_segment = result_cluster.setdefault(segment_name, {})
                    preset_name, preset = min(segment.presets.all.items())

                    # only minimal instance
                    result_segment[preset_name] = self.calc_stats(segment, preset)

        self.write(result)


default_handlers = [
    (f'/api/quota/{NAME_RE}', QYPQuotaHandler),
    (f'/quota/{NAME_RE}/services', QYPQuotaServicesHandler),
    (f'/api/quota/{NAME_RE}/capacity/(?P<abc_id>[^/]+)', QYPQuotaCapacityHandler),
]
