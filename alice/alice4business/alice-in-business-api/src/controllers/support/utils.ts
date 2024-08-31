import { getUserInfo } from '../../services/blackbox';
import { SupportOperation } from '../../db';

import ip from 'ip';

export async function getUid(login: string) {
    const response = await getUserInfo({ login, userip: ip.address() });

    try {
        const user = response.body.users[0];
        return user.uid.value;
    } catch (err) {
        console.error(err);

        return undefined;
    }
}

export const logSupportOperation = async (operationType: string, succeed: boolean, puid: string, message?: string) => {
    const params = {operationType, succeed, puid, message: message || null}
    const { id } = await SupportOperation.create(params);
    return id;
}
