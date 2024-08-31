import { rolesConfig } from './config';
import { Message, PostRequest, RoleError } from './errors';
import { RoleManipulateParams } from '../../services/idm';

export const isValidRequest = (reqBody: PostRequest) => {
    return Boolean(reqBody && reqBody.login && reqBody.role && JSON.parse(reqBody.role)[rolesConfig.slug]);
};

export const parseRoleParams = (req: PostRequest): RoleManipulateParams => {
    if (!isValidRequest(req)) {
        throw new RoleError(1, Message.invalidProps, 'fatal');
    }
    const login = JSON.parse(req.fields)['passport-login'];
    const role = JSON.parse(req.role)[rolesConfig.slug];
    if (!Object.keys(rolesConfig.values).includes(role)) {
        throw new RoleError(1, Message.nonexistRole, 'fatal');
    }
    return { login, role };
};
