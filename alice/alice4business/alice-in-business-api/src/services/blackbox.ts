import yabox from 'yabox';
import config from '../lib/config';
import { getServiceTickets } from './tvm';

// https://doc.yandex-team.ru/Passport/AuthDevGuide/concepts/DB_About.html#DB_About__db-attributes
const ACC_HAS_PLUS_ATTR = '1015';

const blackbox = yabox(config.blackbox);

export const getUserInfo = async (props: any) => {
    const serviceTickets = await getServiceTickets(['blackbox']);
    const response = await blackbox.userinfo({
        headers: {
            'x-ya-service-ticket': serviceTickets.blackbox.ticket,
        },
        ...props,
    });

    return response;
};

export const getOauth = async (props: any) => {
    const serviceTickets = await getServiceTickets(['blackbox']);
    const response = await blackbox.oauth({
        headers: {
            'x-ya-service-ticket': serviceTickets.blackbox.ticket,
        },
        ...props,
    });

    return response;
};

export const getUserHasPlus = async (uid: string, ip: string) => {
    const userInfo = (await getUserInfo({uid, userip: ip, attributes: ACC_HAS_PLUS_ATTR})).body.users[0];
    return userInfo != null ? Boolean(userInfo.attributes && userInfo.attributes[ACC_HAS_PLUS_ATTR]) : null;
};
