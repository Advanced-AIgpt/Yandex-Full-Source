from functools import wraps
import time


HIERARCHICAL_SEPARATOR = '/'
ROLE_SEPARATOR = ':'


def parse_role(role_str):
    result = dict()
    for part in role_str.split(HIERARCHICAL_SEPARATOR):
        key, value = part.split(ROLE_SEPARATOR)
        result[key] = value
    return result


def serialize_role(role_dict):
    return HIERARCHICAL_SEPARATOR.join(f'{key}{ROLE_SEPARATOR}{value}' for key, value in sorted(role_dict.items()))


def build_role_path(role_dict, slugs):
    root_slug = slugs[0]
    return '/'.join([root_slug] + [role_dict[s] for s in slugs])


class SlugAwarenessMixin:
    def __init__(self):
        self.slugs = list()  # list of pairs (level, slug_name)

    def add_slug(self, level, slug_name):
        self.slugs.append((level, slug_name))
        self.slugs = sorted(self.slugs)

    def make_slug_path(self, role_dict):
        '''For format description go to
           https://wiki.yandex-team.ru/intranet/idm/API/#role-spec-slug-path
        '''
        return '/'.join(['{}/{}'.format(slug, role_dict[slug]) for _, slug in self.slugs])


class RoleManager(SlugAwarenessMixin):
    async def add_role(self, login, user_type, role):
        '''Add role to the base'''
        pass

    async def remove_role(self, login, user_type, role):
        '''Remove role from the base'''
        pass

    async def get_roles(self, offset=0, limit=None):
        '''Get all pairs (login, user_type, role) from base with offset and limit'''
        pass

    async def get_all_roles(self):
        '''Get all pairs (login, user_type, role) from base'''
        pass

    async def is_granted(self, login, user_type, role):
        '''Check whether user has permissions'''
        pass

    async def get_role_info(self):
        '''Returns role tree'''
        pass


class RoleManagerProxy:
    async def try_add_role(self, login, user_type, role):
        '''Add role to the base'''
        pass

    async def try_remove_role(self, login, user_type, role):
        '''Remove role from the base'''
        pass

    async def try_get_roles(self, offset, limit):
        '''Get list of some pairs (login, user_type, role) from base'''
        pass

    async def try_get_all_roles(self):
        '''Get list of all pairs (login, user_type, role) from base'''
        pass


class FreshnessManager(object):
    def __init__(self, period):
        self.period = period
        self.expiration_time = self.now()

    @staticmethod
    def now():
        # This simple implementation can be replaced to improve accuracy.
        return time.time()

    def update_expiration_time(self):
        self.expiration_time = max(self.expiration_time, self.now()) + self.period

    def expired(self):
        if self.now() < self.expiration_time:
            return False
        else:
            self.update_expiration_time()
            return True


class CachingRoleManager(RoleManager):
    def __init__(self, role_info, role_manager_proxy, logger, expiration_period=77):
        super().__init__()

        self.role_info = role_info
        self.role_manager_proxy = role_manager_proxy
        self.logger = logger
        self.freshness = FreshnessManager(expiration_period)
        self.roles = set()

    async def add_role(self, login, user_type, role):
        ok, result = await self.role_manager_proxy.try_add_role(login, user_type, role)
        if ok:
            role_entry = (login, user_type, role)
            self.roles.add(role_entry)
            self.logger.info('Role %s for %s %s was added', role, user_type, login)
            return True
        else:
            self.logger.warning('Role %s for %s %s was not added. Exception: %s(%s)',
                                role, user_type, login, type(result), result)
            return False

    async def remove_role(self, login, user_type, role):
        ok, result = await self.role_manager_proxy.try_remove_role(login, user_type, role)
        if ok:
            self.roles.discard((login, user_type, role))
            self.logger.info('Role %s for %s %s was removed', role, user_type, login)
            return True
        else:
            self.logger.warning('Role %s for %s %s was not removed. Exception: %s(%s)',
                                role, user_type, login, type(result), result)
            return False

    async def get_roles(self, offset, limit):
        ok, result = await self.role_manager_proxy.try_get_roles(offset, limit)
        if ok:
            self.logger.debug('Got roles with offset=%s and limit=%s', offset, limit)
            return result
        else:
            self.logger.warning('Unable to get roles with offset=%s and limit=%s. Exception: %s', offset, limit, result)
            return list()

    async def get_all_roles(self):
        await self._try_update_cache_if_expired()
        return self.roles

    async def is_granted(self, login, user_type, role):
        if isinstance(role, dict):
            role = serialize_role(role)
        await self._try_update_cache_if_expired()
        return (login, user_type, role) in self.roles

    async def _try_update_cache_if_expired(self):
        if self.freshness.expired():
            self.logger.debug('Cache expired. Old roles: %s', self.roles)
            ok, result = await self.role_manager_proxy.try_get_all_roles()
            if ok:
                self.roles = set(result)  # try_get_all_roles return list((login, user_type, role))
                self.logger.debug('Cache was updated. New roles: %s', self.roles)
            else:
                self.logger.warning('Cache was not updated. Exception: %s', result)

    async def get_role_info(self):
        return self.role_info


def safe_wrapper(func):
    @wraps(func)
    async def wrapper(*args, **kwargs):
        try:
            return True, await func(*args, **kwargs)
        except Exception as ex:
            return False, ex
    return wrapper
