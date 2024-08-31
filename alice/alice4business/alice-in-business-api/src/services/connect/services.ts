import config from '../../lib/config';
import { _organizationHeaders, _userHeaders, connectGotWithTvm, User } from './utils';

const selfSlug = config.connect.selfSlug;

export const enableService = async (orgId: number, user: User, options: object = {}) =>
    connectGotWithTvm<void>(`/v11/services/${selfSlug}/enable/`, {
        ...options,
        method: 'post',
        headers: {
            ..._organizationHeaders(orgId),
            ..._userHeaders(user),
        },
    });

export const disableService = async (orgId: number, user: User, options: object = {}) =>
    connectGotWithTvm<void>(`/v11/services/${selfSlug}/disable/`, {
        ...options,
        method: 'post',
        headers: {
            ..._organizationHeaders(orgId),
            ..._userHeaders(user),
        },
    });
