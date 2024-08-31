import { Op } from 'sequelize';

import { asyncJsonResponse } from '../utils';
import { PromoCode } from '../../db';
import { PromoCodeSchema } from '../../db/tables/promoCode';
import { sendPush } from '../../services/push/send';

import { logSupportOperation } from './utils';

const pullOrganizationId = 'c8cba10c-97fb-40f1-8c36-a950ceed14bd';
const OK_STATUS = 'ok'
const NOT_OK_STATUS = 'not ok'

export const addPromocode = asyncJsonResponse(
    async (req) => {
        const operationType = 'promocodes:add';
        const logPuid = req.user.uid.toString();

        try {
            const { codes, organizationId, ticketKey }: {codes: string, organizationId: string, ticketKey: string} = req.body.params

            const codesArr = codes
                .split(/[\s\n;,]+/)
                .filter(Boolean)
                .map((code) => ({ code, organizationId, ticketKey }));

            await PromoCode.bulkCreate(codesArr as PromoCodeSchema[], {
                ignoreDuplicates: true,
            });

            sendPush({
                topic: organizationId,
                event: 'organization-info',
                payload: null,
            });

            await logSupportOperation(operationType, true, logPuid)
            return {
                status: OK_STATUS,
            }
        } catch (e){
            const id = await logSupportOperation(operationType, false, logPuid, e.message)
            return {
                status: NOT_OK_STATUS,
                message: `Operation failed with id ${id}`
            }
        }



    },
    { wrapResult: true },
)

export const addPromocodeToOrganization = asyncJsonResponse(
    async (req) => {
        const operationType = 'promocodes:add-to-organization';
        const logPuid = req.user.uid.toString();

        try {
            const {organizationId, codesNum} = req.body.params

            const pullPromocodes = (await PromoCode.findAll(
                {
                    where: { organizationId: pullOrganizationId },
                    attributes: ['id'],
                    limit: codesNum,
                })).map((pullPromocode) => pullPromocode.id);

            await PromoCode.update({ organizationId }, { where: {
                    id: {[Op.in]: pullPromocodes}
                }})

            if(codesNum > pullPromocodes.length) {
                const msg = `Было добавлено только ${pullPromocodes.length} досутпных промокодов.`
                await logSupportOperation(operationType, true, logPuid, msg)
                return {
                    status: OK_STATUS,
                    message: msg
                }
            } else {
                await logSupportOperation(operationType, true, logPuid)
                return {
                    status: OK_STATUS,
                }
            }
        } catch (e) {
            const id = await logSupportOperation(operationType, false, logPuid, e.message)
            return {
                status: NOT_OK_STATUS,
                message: `Operation failed with id ${id}`
            }
        }
    },
    { wrapResult: true },
)
