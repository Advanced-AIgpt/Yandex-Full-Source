import { asyncJsonResponse } from '../utils';
import { Device, Organization } from '../../db';
import RestApiError from '../../lib/errors/restApi';
import { Status } from '../../db/tables/device';
import config from '../../lib/config';
import { isEmptyResultError } from '../../lib/errors/utils';
import log from "../../lib/log";

export const getSubscriptionState = asyncJsonResponse(
    async (req, res) => {
        const { id: deviceId, platform } = req.query;

        const device =
            typeof deviceId === 'string' &&
            deviceId &&
            typeof platform === 'string' &&
            platform
                ? await Device.findOne({
                      attributes: ['status'],
                      where: { deviceId, platform },
                  }).catch((origError) => {
                      throw new RestApiError('Internal error', 500, {
                          origError,
                          payload: {
                              error: {
                                  name: 'INTERNAL_ERROR',
                                  message: origError.message,
                              },
                          },
                      });
                  })
                : null;

        const {
            deactivationShiftMs,
            activationShiftMs,
        } = config.quasar.subscriptionStatus;
        const active = device?.status === Status.Active;
        const expirationTs =
            Date.now() + (active ? activationShiftMs : deactivationShiftMs);

        // https://st.yandex-team.ru/QUASAR-4735#5d4d8ec44b92d7001d3c1e81
        return {
            finished: false,
            expires: new Date(expirationTs).toISOString(),
        };
    },
    { wrapResult: true },
);

export const getAuxiliaryConfig = asyncJsonResponse(
    async (req, res) => {
        const { device_id: deviceId, platform } = req.query;

        if (
            typeof deviceId !== 'string' ||
            !deviceId ||
            typeof platform !== 'string' ||
            !platform
        ) {
            throw new RestApiError(
                'Query params "platform" and "device_id" are required',
                400,
            );
        }

        const device = (await Device.findOne({
            attributes: ['smartHomeUid', 'preactivatedSkillIds', 'status', 'platform'],
            where: { deviceId, platform },
            include: [
                {
                    model: Organization,
                    attributes: ['preactivatedSkillIds', 'maxStationVolume'],
                },
            ],
            rejectOnEmpty: true,
        }).catch((origError) => {
            if (isEmptyResultError(origError)) {
                throw new RestApiError('Device not found', 404);
            }

            throw new RestApiError('Internal error', 500, { origError });
        }))!;

        const { lifetimeMs } = config.quasar.auxiliaryConfig;

        const preactivatedSkillIds = (
            device.organization?.preactivatedSkillIds || []
        ).concat(device.preactivatedSkillIds || []);

        const maxVolume = (device.isStation() && device.organization?.maxStationVolume) || undefined;

        return {
            data: {
                /**
                 * NAlice.NQuasarAuxiliaryConfig.TAlice4BusinessConfig
                 * https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/quasar/auxiliary_config.proto
                 */
                smart_home_uid: device.smartHomeUid || undefined,
                preactivated_skill_ids: [...new Set(preactivatedSkillIds)],
                unlocked: device.status === Status.Active,
                max_volume: maxVolume
            },
            expires: new Date(Date.now() + lifetimeMs).toISOString(),
        };
    },
    { wrapResult: true },
);
