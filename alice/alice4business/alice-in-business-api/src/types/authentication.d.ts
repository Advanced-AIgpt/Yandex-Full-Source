import * as ACL from '../lib/acl';

declare module 'express' {
    interface Request {
        user: ACL.User;
    }
}
