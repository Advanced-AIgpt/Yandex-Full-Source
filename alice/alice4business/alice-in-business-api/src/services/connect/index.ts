import * as domains from './domains';
import * as organizations from './organizations';
import * as resources from './resources';
import * as services from './services';
import * as whois from './whois';
import * as departments from './departments';
import * as groups from './groups';
import * as users from './users';
import * as webhooks from './webhooks';

const Connect = {
    ...domains,
    ...organizations,
    ...resources,
    ...services,
    ...whois,
    ...departments,
    ...groups,
    ...users,
    ...webhooks,
};
export default Connect;
