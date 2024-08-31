# coding: utf-8

from vins_core.nlu.flow_nlu_factory.transition_model import register_transition_model
from vins_core.nlg.template_nlg import register_global
from crm_bot import nlg_globals

from crm_bot.transition_model import create_crmbot_transition_model

register_transition_model('crm_bot', create_crmbot_transition_model)

register_global('is_first_message', nlg_globals.is_first_message)
register_global('default_redirect', nlg_globals.default_redirect)
register_global('operator_redirect', nlg_globals.operator_redirect)
register_global('department_redirect', nlg_globals.department_redirect)
register_global('is_webim_v1', nlg_globals.is_webim_v1)
register_global('is_webim_v2', nlg_globals.is_webim_v2)
register_global('is_webim', nlg_globals.is_webim)
