import { PromoCode } from '../../db';
import config from '../config';
import log from '../log';
import * as solomon from '../../services/solomon';
import { createAsyncWorker } from './utils';
import { fn } from 'sequelize';

const routine = async () => {
    try {
        const counts = await PromoCode.findAll({
            where: {
                status: null,
            },
            group: ['organizationId'],
            attributes: [
                'organizationId',
                [fn('COUNT', 'organizationId'), 'promoCodesCount'],
            ],
        });

        for (const item of counts) {
            const id = item.organizationId.split('-').pop();
            const count = +item.getDataValue('promoCodesCount' as any);

            solomon.setCounter(`promocode__rest_count_${id}_org_max`, count);
        }
    } catch (e) {
        log.error('counters worker process error', e);
    }
};

export const promoCounterWorker = createAsyncWorker({
    interval: config.app.countersWorker.interval,
    routine,
    name: 'promo-counter',
});
