# coding: utf-8
from __future__ import unicode_literals

from personal_assistant.callback import callback_method
from vins_core.dm.response import ClientActionDirective


class StrokaCallbacksMixin(object):
    @callback_method
    def navigate_browser(self, req_info, form, response, **kwargs):
        cmd_name = form.name.split('.')[-1]
        payload = {
            'command_name': cmd_name
        }

        if cmd_name == 'select_tab':
            payload['tab_number'] = str(form.tab_number.value)

        response.directives.append(
            ClientActionDirective(name='navigate_browser', sub_name='pc_navigate_browser' + cmd_name, payload=payload)
        )

    @callback_method
    def power_off(self, req_info, response, **kwargs):
        response.directives.append(ClientActionDirective(name='power_off', sub_name='pc_power_off'))

    @callback_method
    def restart_pc(self, req_info, response, **kwargs):
        response.directives.append(ClientActionDirective(name='restart_pc', sub_name='pc_restart'))

    @callback_method
    def hibernate(self, req_info, response, **kwargs):
        response.directives.append(ClientActionDirective(name='hibernate', sub_name='pc_hibernate'))

    @callback_method
    def search_local(self, req_info, form, response, **kwargs):
        response.directives.append(ClientActionDirective(name='search_local', sub_name='pc_search_local', payload={
            'text': form.query.value
        }))

    @callback_method
    def open_folder(self, req_info, form, response, **kwargs):
        response.directives.append(ClientActionDirective(name='open_folder', sub_name='pc_open_folder', payload={
            'folder': form.folder.value
        }))

    @callback_method
    def open_file(self, req_info, form, response, **kwargs):
        response.directives.append(ClientActionDirective(name='open_file', sub_name='pc_open_file', payload={
            'file': form.file.value
        }))

    @callback_method
    def mute(self, req_info, form, response, **kwargs):
        response.directives.append(ClientActionDirective(name='mute', sub_name='pc_mute'))

    @callback_method
    def unmute(self, req_info, form, response, **kwargs):
        response.directives.append(ClientActionDirective(name='unmute', sub_name='pc_unmute'))

    @callback_method
    def open_default_browser(self, req_info, form, response, **kwargs):
        response.directives.append(
            ClientActionDirective(name='open_default_browser', sub_name='pc_open_default_browser')
        )

    @callback_method
    def open_ya_browser(self, req_info, form, response, **kwargs):
        response.directives.append(ClientActionDirective(name='open_ya_browser', sub_name='pc_open_ya_browser'))

    @callback_method
    def open_flash_card(self, req_info, form, response, **kwargs):
        response.directives.append(ClientActionDirective(name='open_flash_card', sub_name='pc_open_flash_card'))

    @callback_method
    def open_start(self, req_info, form, response, **kwargs):
        response.directives.append(ClientActionDirective(name='open_start', sub_name='pc_open_start'))

    @callback_method
    def open_settings(self, req_info, form, response, **kwargs):
        response.directives.append(ClientActionDirective(name='open_settings', sub_name='pc_open_settings', payload={
            'target': form.target.value or 'tablo'
        }))

    @callback_method
    def open_disk(self, req_info, form, response, **kwargs):
        response.directives.append(ClientActionDirective(name='open_disk', sub_name='pc_open_disk', payload={
            'disk': form.disk.value
        }))
