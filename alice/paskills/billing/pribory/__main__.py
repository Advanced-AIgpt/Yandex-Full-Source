# -*- coding: utf8 -*-
"""
A alert / check generator for YASM + Juggler

Fairly simple.

Main points:
    - have everything under `quasar` prefix in yasm and namespace in juggler
    - have `quasar` juggler tag so our notification rule catches that up
"""
from __future__ import print_function

import requests
from collections import namedtuple
from enum import Enum

UI = 'https://yasm.yandex-team.ru/'
API = 'https://yasm.yandex-team.ru/srvambry/'
# SOMEHOW each panel must have a user but it is not checked anywhere! Just use our robot here.
PANEL_OWNER_USER = 'robot-paskills-yt'


def make_alert(
        component,
        alert_name,
        signal,
        warn,
        crit,
        host,
        flap_stable=15,
        flap_critical=75,
        notify_marty=False,
        avg=0,
        **tags
):
    """
    :param int warn: warning lower bound, everything above causes warning!
    :param int crit: crit lower bound, everything above causes crit!
    :param tags: a {str: str} dict of tags, as present in UI, i.e.::

        make_alert(..., prj='x-products.ololo.pewpew', tier='foo-1,foo-2')
        make_alert(..., ctype='6160eb14-19b3-4137-9282-3ad3945c1b1c', tier='primary,replica',
        itype='mdbdom0', hosts='CON')

    :returns: body for alert to be sent to YASM API
    """
    if isinstance(warn, list) or isinstance(crit, list):
        warn_range = warn
        crit_range = crit
    else:
        warn_range = [warn, crit]
        crit_range = [crit, None]

    alert_dict = {
        "name": "paskills.%s_%s" % (component, alert_name),
        "signal": signal,
        "tags": {
            name: map(str.strip, value.split(','))
            for name, value in tags.items()
        },
        "abc": "yandexdialogs2",
        "warn": warn_range,
        "crit": crit_range,
        "juggler_check": {
            "aggregator": "logic_or",
            "aggregator_kwargs": {
                "nodata_mode": "force_ok",
                "unreach_mode": "force_ok",
                "unreach_service": [
                    {
                        "check": "yasm_alert:virtual-meta",
                    },
                ],
            },
            "flaps": {
                "stable": flap_stable,
                "critical": flap_critical,
            },
            "host": "paskills_%s" % component,
            "service": alert_name,
            "refresh_time": 5,
            "tags": [
                "paskills-billing",
            ],
            "namespace": "paskills",
            "ttl": 900,
        },
        "mgroups": [
            host,
        ],
    }
    if notify_marty:
        alert_dict["juggler_check"]["tags"].append("marty")

    if avg > 0:
        alert_dict['value_modify'] = {}
        alert_dict['value_modify']['type'] = "aver"
        alert_dict['value_modify']['window'] = avg

    return alert_dict
# billing hosts, historical and new names


billing_hosts = ['billing-1',
                 'billing-2',
                 'billing-3',
                 'billing-4',
                 'billing-5',
                 'billing-6',
                 'billing-7',
                 'billing-8',
                 'billing-9',
                 'billing-10',
                 'billing-11',
                 'billing-12',
                 'x-products-quasar-backend-main-prod-billing-1',
                 'x-products-quasar-backend-main-prod-billing-2',
                 'x-products-quasar-backend-main-prod-billing-3',
                 'x-products-quasar-backend-main-prod-billing-4',
                 'x-products-quasar-backend-main-prod-billing-5',
                 'x-products-quasar-backend-main-prod-billing-6',
                 'x-products-quasar-backend-main-prod-billing-7',
                 'x-products-quasar-backend-main-prod-billing-8',
                 'x-products-quasar-backend-main-prod-billing-9',
                 'x-products-quasar-backend-main-prod-billing-10',
                 'x-products-quasar-backend-main-prod-billing-11',
                 'x-products-quasar-backend-main-prod-billing-12']


def create_alert(alert_data):
    """
    :returns: name of the created alert for future reference
    """
    result = requests.post(API + 'alerts/update', params={'name': alert_data['name']}, json=alert_data)

    if result.status_code == 404:
        # not found. Probably alert does not exist yet
        result = requests.post(API + 'alerts/create', json=alert_data)

        if result.status_code != 200:
            print('Problem:', result.content)
            result.raise_for_status()

    elif result.status_code != 200:
        print('Problem:', result.content)
        result.raise_for_status()

    print('Upserted %salert/%s' % (UI, alert_data['name']))

    return alert_data['name']


def create_panel(panel_key, panel_data):
    result = requests.post(
        API + 'upsert',
        json={
            'keys': {'key': panel_key, 'user': PANEL_OWNER_USER},
            'values': panel_data,
        },
    )

    result.raise_for_status()

    print('Upserted %spanel/%s' % (UI, panel_key))


def panel_chart(geometry, type, title=None, minValue=0, **data):
    """
    :param geometry: a string like 1_2_3_4
        1 -- row
        2 -- column
        3 -- width
        4 -- height

        i.e. 2_2_1_1 -- second row, second column, 1x1-panel_chart
    """

    row, column, width, height = map(int, geometry.split('_'))

    return dict(
        row=row,
        height=height,
        col=column,
        width=width,
        title=title,
        type=type,
        minValue=minValue,
        **data
    )


def qloud_unanswers(project, tier, yaxis=2):
    """
    Returns chart signals for qloudy http unanswers of the given project / tier

    :param tier: one or multiple tiers in a string, like 'backend-1' or 'billing-1,billing-2,billing-3'
    :param project: qloud project, like 'x-products.quasar-backend.main-prod'
    """
    return [
        {u'color': u'#ff5555',
         u'host': u'QLOUD',
         u'name': u'or(perc(push-response_5xx_summ,push-requests_summ),0)',
         u'yAxis': yaxis,
         u'tag': u'itype=qloudrouter;ctype=common,common-public,quasar-ext,quasar-int;prj=%s;tier=%s' % (project, tier),
         u'title': u'5xx, %'},
        {u'color': u'#aaaa00',
         u'host': u'QLOUD',
         u'name': u'or(perc(push-response_4xx_summ,push-requests_summ),0)',
         u'yAxis': yaxis,
         u'tag': u'itype=qloudrouter;ctype=common,common-public,quasar-ext,quasar-int;prj=%s;tier=%s' % (project, tier),
         u'title': u'4xx, %'},
    ]


def qloud_timings(project, tier):
    """
    Returns chart signals for qloudy http timings of the given project / tier

    :param tier: one or multiple tiers in a string, like 'backend-1' or 'billing-1,billing-2,billing-3'
    :param project: qloud project, like 'x-products.quasar-backend.main-prod'
    """
    return [
        {u'color': color,
         u'host': u'QLOUD',
         u'name': u'quant(push-time_hgram,%s)' % perc,
         u'title': u'%s / %s %sp' % (project, tier.split(',')[0][:-2], perc),  # FIXME: that -2 trims `-N` part of tier, but it's ugly!
         u'tag': u'itype=qloudrouter;ctype=common,common-public,quasar-ext,quasar-int;prj=%s;tier=%s' % (project, tier),
         }
        for (perc, color) in [
            (50, '#37bff2'),
            (75, '#169833'),
            (95, '#c95edd'),
            (99, '#f6ab31'),
            (999, '#e85b4e'),
        ]
    ]


def nanny_unanswers(balancer, signal_5xx, signal_4xx, yaxis=2):
    return [
        {u'color': u'#ff5555',
         u'host': u'ASEARCH',
         # u'or(perc(balancer_report-report-billing-outgoing_5xx_summ,balancer_report-report-requests_to_%s-requests_summ),0)' % (upstream, upstream),
         u'name': signal_5xx,
         u'yAxis': yaxis,
         u'tag': u'itype=balancer;ctype=prod;prj=%s' % balancer,
         u'title': u'5xx, %'},
        {u'color': u'#aaaa00',
         u'host': u'ASEARCH',
         # u'or(perc(balancer_report-report-requests_to_%s-sc_4xx_summ,balancer_report-report-requests_to_%s-requests_summ),0)' % (upstream, upstream),
         u'name': signal_4xx,
         u'yAxis': yaxis,
         u'tag': u'itype=balancer;ctype=prod;prj=%s' % balancer,
         u'title': u'4xx, %'},
    ]


def nanny_timings(balancer, upstream):
    return [
        {u'color': color,
         u'host': u'ASEARCH',
         u'name': u'quant(balancer_report-report-%s-processing_time_hgram,%s)' % (upstream, perc),
         u'title': u'%sp' % perc,
         u'tag': u'itype=balancer;ctype=prod;prj=%s' % balancer,
         }
        for (perc, color) in [
            (50, '#37bff2'),
            (75, '#169833'),
            (95, '#c95edd'),
            (99, '#f6ab31'),
            (999, '#e85b4e'),
        ]
    ]


def merge(a_dict, **kw):
    """
    returns dict with overrides from **kw

    >>> sorted(merge(dict(a=1), a=2).items())
    [('a', 2)]

    >>> sorted(merge(dict(a=1), b=2).items())
    [('a', 1), ('b', 2)]
    """

    return dict(a_dict.items() + kw.items())


class HtttpCheckHelper(object):
    """
    Convenience helper to make a qloudy http alert

    Warn and Crit are in percents for all but `_timing`
    """

    def __call__(self, component, **kw):
        params = dict(
            component=component.name,
            prj=component.project,
            tier=component.tier,
            ctype=component.ctype,
            itype=component.itype,
            hosts=component.hosts,
            host=component.hosts,
        )

        params.update(**kw)

        return create_alert(make_alert(**params))

    def _4xx(self, component, warn, crit, **extra):
        return self(
            component,
            **merge(
                dict(
                    warn=warn,
                    crit=crit,
                    alert_name='4xx_response_perc',
                    signal='or(perc(diff(diff(diff(push-response_4xx_summ, push-http_403_summ), push-http_401_summ), '
                           'push-http_499_summ),push-requests_summ),0)',
                ),
                **extra
            )
        )

    def _auth(self, component, warn, crit, **extra):
        return self(
            component,
            **merge(
                dict(
                    warn=warn,
                    crit=crit,
                    alert_name='bad_auth_response_perc',
                    signal='or(perc(sum(push-response_401_summ, push-http_403_summ),push-requests_summ),0)',
                ),
                **extra
            )
        )

    def _ctimeout(self, component, warn, crit, **extra):
        return self(
            component,
            **merge(
                dict(
                    warn=warn,
                    crit=crit,
                    alert_name='client_timeout_response_perc',
                    signal='or(perc(push-http_499_summ,push-requests_summ),0)',
                ),
                **extra
            )
        )

    def _5xx(self, component, warn, crit, **extra):
        return self(
            component,
            **merge(
                dict(
                    warn=warn,
                    crit=crit,
                    alert_name='5xx_response_perc',
                    signal='or(perc(push-response_5xx_summ,push-requests_summ),0)',
                ),
                **extra
            )
        )

    def _timing(self, component, warn, crit, **extra):
        return self(
            component,
            **merge(
                dict(
                    warn=warn,
                    crit=crit,
                    alert_name='timings_98p',
                    signal='or(quant(push-time_hgram,98),0)',
                    avg=5,
                ),
                **extra
            )
        )

    def _custom_alert(self, component, alert_name, warn, crit, signal, flap_stable=0, flap_critical=0, itype='qloud',
                      **extra):
        return self(
            component,
            **merge(
                dict(
                    warn=warn,
                    crit=crit,
                    alert_name=alert_name,
                    signal=signal,
                    flap_stable=flap_stable,
                    flap_critical=flap_critical,
                    itype=itype,
                ),
                **extra
            )
        )


http_check = HtttpCheckHelper()


Component = namedtuple('Component', ['name', 'project', 'tier', 'ctype', 'itype', 'hosts', 'balancer', 'upstream',
                                     'signal_5xx', 'signal_4xx', 'balancer_ui'])

# TODO: make those tiers automatic via https://st.yandex-team.ru/GOLOVAN-2714
billing_old_prod = Component('billing', 'x-products.quasar-backend.main-prod', ', '.join(billing_hosts),
                             'common,common-public,quasar-ext,quasar-int', 'qloudrouter', 'QLOUD', '', '', '', '', '')

billing_priemka = Component('billing-priemka', 'paskills', '', 'priemka', 'billing', 'ASEARCH',
                            'paskills-common-testing.alice.yandex.net', 'requests_to_billing_rc',
                            u'or(perc(balancer_report-report-requests_to_billing_rc-sc_5xx_summ,'
                            u'balancer_report-report-requests_to_billing_rc-requests_summ),0)',
                            u'or(perc(balancer_report-report-requests_to_billing_rc-sc_4xx_summ,'
                            u'balancer_report-report-requests_to_billing_rc-requests_summ),0)',
                            'paskills-common-testing.alice.yandex.net'
                            )
billing_production = Component('billing', 'paskills', '', 'production', 'billing', 'ASEARCH',
                               'quasar.yandex.net', 'billing',
                               u'or(perc(balancer_report-report-billing-outgoing_5xx_summ,'
                               u'balancer_report-report-billing-requests_summ),0)',
                               u'or(perc(balancer_report-report-billing-outgoing_4xx_summ,'
                               u'balancer_report-report-billing-requests_summ),0)',
                               'paskills-common-production.alice.yandex.net'
                               )


def deploy_all():
    # deploy_billing_old()
    deploy_billing(billing_priemka)
    deploy_billing(billing_production)


def deploy_billing(component):

    tag = 'itype=billing;prj=paskills;ctype=%s' % component.ctype

    class APIAction(Enum):
        CALL = 'unistat-quasar_billing_method_{}_calls_dmmm'
        FAILURE = 'unistat-quasar_billing_method_{}_failures_dmmm'
        QUANT = 'quant(unistat-quasar_billing_method_{}_duration_dhhh,999)'

    def api_methods(action):
        return [
            {"title": "/{}".format(m),
             "active": False if m == 'unistat' else True,
             "tag": tag,
             "host": "ASEARCH",
             "name": action.format(m.replace('/', '-')),
             } for m in ['billing/content_available',
                         'billing/provider/yandexplus/activatePromoOnDevice',
                         'billing/provider/yandexplus',
                         'billing/provider/all',
                         'billing/getContentMetaInfo',
                         'billing/getCardsList',
                         'billing/initProviderPurchaseProcess',
                         'billing/createSkillPurchaseOffer',
                         'billing/purchase_offer',
                         'billing/getPurchaseStatus',
                         'billing/requestPlus',
                         'billing/getSubscriptionsInfo',
                         'billing/getPurchasedContent',
                         'billing/getTransactionsHistory',
                         'billing/v2/getTransactionsHistory',
                         'billing/v2/transaction/@',
                         'billing/user/skill_product/@',
                         'billing/user/skill_product/@/@',
                         'billing/promo/tv/bind_card',
                         'billing/promo/tv/binding_info',
                         'billing/startBinding',
                         'billing/promo/tv/promo_period_available',
                         'billing/promo/tv/activate_promo_period',
                         'billing/device_promo_available',
                         'billing/bindPurchaseOfferToUser',
                         'billing/takeout',
                         'billing/provider/kinopoisk',
                         'billing/provider/kinopoisk/activatePromoOnDevice',
                         'unistat']]

    def promo_activation_failure_results():
        return [
            {"title": title,
             "tag": tag,
             "host": "ASEARCH",
             "name": name,
             } for (title, name) in [('Плюс code_expired', 'unistat-quasar_billing_yandexplus_promo_activation_code_expired_dmmm'),
                                     ('Плюс not_allowed_in_current_region', 'unistat-quasar_billing_yandexplus_promo_activation_not_allowed_in_current_region_dmmm'),
                                     ('Плюс not_exists', 'unistat-quasar_billing_yandexplus_promo_activation_not_exists_dmmm'),
                                     ('Плюс cant_be_consumed', 'unistat-quasar_billing_yandexplus_promo_activation_cant_be_consumed_dmmm'),
                                     ('Плюс only_for_new_users', 'unistat-quasar_billing_yandexplus_promo_activation_only_for_new_users_dmmm'),
                                     ('Плюс only_for_web', 'unistat-quasar_billing_yandexplus_promo_activation_only_for_web_dmmm'),
                                     ('Плюс only_for_mobile', 'unistat-quasar_billing_yandexplus_promo_activation_only_for_mobile_dmmm'),
                                     ('Плюс failed_to_create_payment', 'unistat-quasar_billing_yandexplus_promo_activation_failed_to_create_payment_dmmm'),

                                     ('КП code_expired', 'unistat-quasar_billing_kinopoisk_promo_activation_code_expired_dmmm'),
                                     ('КП not_allowed_in_current_region', 'unistat-quasar_billing_kinopoisk_promo_activation_not_allowed_in_current_region_dmmm'),
                                     ('КП not_exists', 'unistat-quasar_billing_kinopoisk_promo_activation_not_exists_dmmm'),
                                     ('КП cant_be_consumed', 'unistat-quasar_billing_kinopoisk_promo_activation_cant_be_consumed_dmmm'),
                                     ('КП only_for_new_users', 'unistat-quasar_billing_kinopoisk_promo_activation_only_for_new_users_dmmm'),
                                     ('КП only_for_web', 'unistat-quasar_billing_kinopoisk_promo_activation_only_for_web_dmmm'),
                                     ('КП only_for_mobile', 'unistat-quasar_billing_kinopoisk_promo_activation_only_for_mobile_dmmm'),
                                     ('КП failed_to_create_payment', 'unistat-quasar_billing_kinopoisk_promo_activation_failed_to_create_payment_dmmm'),
                                     ]]

    class RemoteAction(Enum):
        CALL = "unistat-quasar_billing_remote_method_{}_calls_dmmm"
        FAILURE = "unistat-quasar_billing_remote_method_{}_failures_dmmm"
        QUANT = "quant(unistat-quasar_billing_remote_method_{}_duration_dhhh,999)"
        TIMEOUT = "unistat-quasar_billing_remote_method_{}_timeout_dmmm"

    def remote_methods(action, methods):
        return [
            {"title": "{}".format(m),
             "tag": tag,
             "host": "ASEARCH",
             "active": a,
             "name": action.format((func(m) if func else m).replace('/', '_', 1).replace('/', '-').replace('.', '-')),
             } for m, a, func in methods]

    def remote_methods_base(action):
        return remote_methods(action, [('api.mediabilling.yandex.net/billing/promo-code/clone', True, None),
                                       ('api.mediabilling.yandex.net/promo-codes/activate', True, None),
                                       ('api.mediabilling.yandex.net/payment/bind-card', True, None),
                                       ('music-web.music.yandex.net/internal-api/account/submit-native-order', True,
                                        None),
                                       ('droideka-smarttv-yandex-net/api/v7/suw/gift', True, None),
                                       ('blackbox.yandex.net/blackbox', True, None),
                                       ('quasar.yandex.net/billing/device_list', True, None),
                                       ('quasar.yandex.net/get_tags_for_user', True, None)])

    def remote_methods_kinopoisk(action):
        return remote_methods(action, [('api.ott.yandex.net/content/@/available', True, None),
                                       ('api.ott.yandex.net/content/@', True, None),
                                       ('api.ott.yandex.net/content/@/options', True, None),
                                       ('api.ott.yandex.net/products/@', True, None),
                                       ('api.ott.yandex.net/purchased', True, None)])

    def remote_methods_trust(action):
        return remote_methods(action, [('trust-payments.paysys.yandex.net/trust-payments/v2/payments/@', True, None),
                                       ('trust-payments.paysys.yandex.net/trust-payments/v2/payment-methods', True,
                                        None),
                                       ('trust-payments.paysys.yandex.net/trust-payments/v2/bindings', True, None),
                                       ('trust-payments.paysys.yandex.net/trust-payments/v2/bindings/@/start', True, None),
                                       ('payments.mail.yandex.net/v1/merchant_by_key/@', True, None),
                                       ('payments.mail.yandex.net/v1/internal/order/@', True, None),
                                       ('payments.mail.yandex.net/v1/internal/order/@/@', True, None),
                                       ('payments.mail.yandex.net/v1/internal/order/@/@/start', True, None),
                                       ('payments.mail.yandex.net/v1/internal/order/@/@/clear', True, None),
                                       ('payments.mail.yandex.net/v1/internal/order/@/@/unhold', True, None),
                                       ('payments.mail.yandex.net/v1/internal/service/@', True, None)])

    # http_check._4xx(billing_rc, warn=0.5, crit=2)
    # http_check._auth(billing_rc, warn=0.5, crit=2, notify_marty=True)
    # http_check._ctimeout(
    #     billing_rc, warn=20, crit=30, flap_stable=25, flap_critical=70, notify_marty=True
    # )  # we have retries on 499 so catch only avalanche growth
    # http_check._5xx(billing_rc, warn=0.05, crit=0.05, flap_stable=10, flap_critical=5, notify_marty=True)
    # http_check._timing(billing_rc, warn=2.200, crit=3.0)
    #
    # http_check._custom_alert(billing_rc, alert_name='plus_activation_failure', signal='or(quasar_billing_plus_activation_failure_dmmm,0)', warn=0.1, crit=1.0)
    # # promocode_trial_creation_failure =
    # http_check._custom_alert(billing_rc, alert_name='promocode_trial_creation_failure', signal='or(quasar_billing_promocode-trial-subscription-on-trust-creation-failure_dmmm,0)', warn=0.1, crit=1.0)
    # # subs_cancel_failure =
    # http_check._custom_alert(billing_rc, alert_name='subs_cancel_failure', signal='or(quasar_billing_subscription-cancelled-from-provider-failure_dmmm,0)', warn=0.1, crit=1.0)

    create_panel(
        'paskills_%s' % component.name,
        {
            u'editors': [],
            u'title': u'Dialogs Billing (%s)' % component.ctype,
            u'type': u'panel',
            u'charts': [

                panel_chart('1_1_1_1', 'graphic', title='Unanswers UI', normalize=True,
                            signals=nanny_unanswers(component.balancer, component.signal_5xx, component.signal_4xx)),
                panel_chart('1_2_1_1', 'graphic', title='Quantiles UI', normalize=True,
                            signals=nanny_timings(component.balancer, component.upstream)),
                panel_chart('1_3_1_1', 'graphic', title='Unanswers Alice', normalize=True,
                            signals=nanny_unanswers(component.balancer_ui, component.signal_5xx, component.signal_4xx)),
                panel_chart('1_4_1_1', 'graphic', title='Quantiles Alice', normalize=True,
                            signals=nanny_timings(component.balancer_ui, component.upstream)),
                panel_chart('2_1_2_1', 'graphic', title=u"Методы - calls", normalize=True, signals=api_methods(APIAction.CALL.value)),
                panel_chart('2_3_1_1', 'graphic', title=u"Методы - failures", normalize=True, signals=api_methods(APIAction.FAILURE.value)),
                panel_chart('2_4_1_1', 'graphic', title=u"Методы - время, мс (квантиль 99.9%)", normalize=True, signals=api_methods(APIAction.QUANT.value)),

                panel_chart('3_1_2_1', 'graphic', title=u"Исходящие вызовы КП - calls", normalize=True,
                            signals=remote_methods_kinopoisk(RemoteAction.CALL.value)),
                panel_chart('3_3_1_1', 'graphic', title=u"Исходящие вызовы КП - failures", normalize=True,
                            signals=remote_methods_kinopoisk(RemoteAction.FAILURE.value)),
                panel_chart('3_4_1_1', 'graphic', title=u"Исходящие вызовы КП - время, мс (квантиль 99.9%)",
                            normalize=True, signals=remote_methods_kinopoisk(RemoteAction.QUANT.value)),
                panel_chart('3_5_1_1', 'graphic', title=u"Исходящие вызовы КП - таймауты", normalize=True,
                            signals=remote_methods_kinopoisk(RemoteAction.TIMEOUT.value)),

                panel_chart('4_1_2_1', 'graphic', title=u"Исходящие вызовы - calls", normalize=True,
                            signals=remote_methods_base(RemoteAction.CALL.value)),
                panel_chart('4_3_1_1', 'graphic', title=u"Исходящие вызовы - failures", normalize=True,
                            signals=remote_methods_base(RemoteAction.FAILURE.value)),
                panel_chart('4_4_1_1', 'graphic', title=u"Исходящие вызовы - время, мс (квантиль 99.9%)",
                            normalize=True, signals=remote_methods_base(RemoteAction.QUANT.value)),
                panel_chart('4_5_1_1', 'graphic', title=u"Исходящие вызовы - таймауты", normalize=True,
                            signals=remote_methods_base(RemoteAction.TIMEOUT.value)),

                panel_chart('5_1_2_1', 'graphic', title=u"Исходящие TRUST/Оплаты - calls", normalize=True,
                            signals=remote_methods_trust(RemoteAction.CALL.value)),
                panel_chart('5_3_1_1', 'graphic', title=u"Исходящие TRUST/Оплаты - failures", normalize=True,
                            signals=remote_methods_trust(RemoteAction.FAILURE.value)),
                panel_chart('5_4_1_1', 'graphic', title=u"Исходящие TRUST/Оплаты - время, мс (квантиль 99.9%)",
                            normalize=True, signals=remote_methods_trust(RemoteAction.QUANT.value)),
                panel_chart('5_5_1_1', 'graphic', title=u"Исходящие TRUST/Оплаты - таймауты", normalize=True,
                            signals=remote_methods_trust(RemoteAction.TIMEOUT.value)),

                panel_chart('6_1_1_1', 'graphic', title=u"Некорректный CSRF токен", normalize=True, signals=[{
                    "title": u"Ошибки токена",
                    "color": "#f23737",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing_incorrect_x_csrf_token_dmmm"}]),
                panel_chart('6_2_1_1', 'graphic', title=u"Активации плюса и КП", normalize=False, signals=[{
                    "title": u"Активации Плюса",
                    "color": "#37bff2",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing_yandexplus_promo_activation_dmmm",
                }, {
                    "title": u"Ошибки активаций",
                    "color": "#ff0000",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing_yandexplus_promo_activation_failure_dmmm"
                }, {
                    "title": u"Активация КП",
                    "color": "#169833",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing_kinopoisk_promo_activation_dmmm"
                }, {
                    "title": u"Ошибки активаций КП",
                    "color": "#f6ab31",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing_kinopoisk_promo_activation_failure_dmmm"}]),
                panel_chart('6_3_1_1', 'graphic', title=u"Ошибки активации плюса и КП (Кол-во запросов)", normalize=True,
                            signals=promo_activation_failure_results()),
                panel_chart('6_4_1_1', 'graphic', title=u"Платёжный callback (первый) - calls", normalize=True, signals=[{
                    "color": "#37bff2",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing_payment-callback-first_calls_dmmm"}]),
                panel_chart('6_5_1_1', 'graphic', title=u"Платёжный callback (первый) - время, мс (квантиль 99.9%)", normalize=True, signals=[{
                    "color": "#37bff2",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "quant(unistat-quasar_billing_payment-callback-first_duration_dhhh,999)"}]),

                panel_chart('7_1_1_1', 'graphic', title=u"Postgres CPU", normalize=True, signals=[{
                    "title": u"USED",
                    "color": "#37bff2",
                    "tag": "itype=mdbdom0; ctype=2933d808-6793-4d08-985d-75a35e1229f8; tier=primary,replica",
                    "host": "CON",
                    "name": "portoinst-cpu_usage_cores_tmmv",
                }, {
                    "title": u"Guaranteed",
                    "color": "#169833",
                    "tag": "itype=mdbdom0; ctype=2933d808-6793-4d08-985d-75a35e1229f8; tier=primary,replica",
                    "host": "CON",
                    "name": "portoinst-cpu_guarantee_cores_tmmv",
                }, {
                    "title": u"Limit",
                    "color": "#f63131",
                    "tag": "itype=mdbdom0; ctype=2933d808-6793-4d08-985d-75a35e1229f8; tier=primary,replica",
                    "host": "CON",
                    "name": "portoinst-cpu_limit_cores_tmmv"
                }, {
                    "title": u"Wait",
                    "color": "#f63131",
                    "tag": "itype=mdbdom0; ctype=2933d808-6793-4d08-985d-75a35e1229f8; tier=primary,replica",
                    "host": "CON",
                    "name": "portoinst-cpu_wait_cores_tmmv"}]),
                panel_chart('7_2_1_1', 'graphic', title=u"Кол-во живых нод", normalize=True, signals=[{
                    "color": "#37bff2",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing_node_alive_ammv"}]),
                panel_chart('7_3_1_1', 'graphic', title=u"Кол-во неудавшихся платежей", normalize=True, signals=[{
                    "color": "#37bff2",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing_purchase_errors_dmmm"}]),
                panel_chart('7_4_1_1', 'graphic', title=u"Кол-во зависших в обработке подписок/платежей", normalize=True, signals=[{
                    "title": u"Платежи",
                    "color": "#169833",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing_stuck-purchases-count_axxv",
                }]),
                panel_chart('7_5_1_1', 'graphic', title=u"Активации продуктов навыков", normalize=False, signals=[{
                    "title": u"Успешная активация",
                    "color": "#37bff2",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing-user-skill-product_success_calls_dmmm",
                }, {
                    "title": u"Активация уже активированного продукта",
                    "color": "#169833",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing-user-skill-product_already-activated_calls_dmmm"
                }, {
                    "title": u"Неизвестный скилл",
                    "color": "#ff0000",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing-user-skill-product_unknown-skill_calls_dmmm"
                }, {
                    "title": u"Неизвестный токен",
                    "color": "#f6ab31",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing-user-skill-product_unknown-token_calls_dmmm"}]),

                panel_chart('8_1_1_1', 'graphic', title=u"Покупки в навыках", normalize=False, signals=[{
                    "title": u"Покупка",
                    "color": "#37bff2",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing_payment-callback_calls_dmmm",
                }, {
                    "title": u"Ошибка платежа",
                    "color": "#169833",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing_payment-callback-payment_error_dmmm"
                }, {
                    "title": u"Ошибка похода в навык",
                    "color": "#ff0000",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing_payment-skill-callback_failures_dmmm"
                }, {
                    "title": u"Ошибка обработки платежа",
                    "color": "#f6ab31",
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "unistat-quasar_billing_payment-callback_failures_dmmm"}]),

                panel_chart('8_2_1_1', 'graphic', title=u"Ошибка исчерпания промокодов в кампании", normalize=False, signals=[{
                    "title": u"Ошибки по платформе и промокоду",
                    "color": u'#ff5555',
                    "tag": tag,
                    "host": "ASEARCH",
                    "name": "or(unistat-quasar_billing_no_promocode_left-total_dmmm, 0)",
                }])
            ],
        },
    )


if __name__ == '__main__':
    deploy_all()
