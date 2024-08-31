from .role_manager import RoleManagerProxy, CachingRoleManager, safe_wrapper

from copy import deepcopy
import logging


class SimpleRoleManagerProxy(RoleManagerProxy):
    def __init__(self, logger=None):
        self.logger = logger or logging.getLogger('SimpleRoleManagerProxy')
        self.roles = list()

    @safe_wrapper
    async def try_add_role(self, login, user_type, role):
        self.roles.append((login, user_type, role))

    @safe_wrapper
    async def try_remove_role(self, login, user_type, role):
        self.roles.remove((login, user_type, role))

    @safe_wrapper
    async def try_get_roles(self, offset, limit):
        return deepcopy(self.roles[offset:offset+limit])

    @safe_wrapper
    async def try_get_all_roles(self):
        return deepcopy(self.roles)

    @safe_wrapper
    async def try_is_granted(self, login, user_type, role):
        '''Not used by CachingRoleManager'''
        return (login, user_type, role) in self.roles


def make_simple_role_manager(role_info, logger=None):
    simple_role_manager = SimpleRoleManagerProxy(logger)
    return CachingRoleManager(role_info, simple_role_manager, logger=simple_role_manager.logger)
