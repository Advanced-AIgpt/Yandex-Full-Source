import { asyncJsonResponse, notFound } from '../utils';
import { ActivationCode, Device, Organization } from '../../db';
import RestApiError from '../../lib/errors/restApi';
import { Status } from '../../db/tables/device';
import config from '../../lib/config';
import { Op } from 'sequelize';
import { ActivationCodeSchema } from '../../db/tables/activationCode';
import { dialogovoHistograms, fillHist } from '../../services/solomon'

interface DeviceIdentifier {
    platform: string;
    deviceId: string;
}

export const getControlledDevices = asyncJsonResponse(
    async (req, res): Promise<DeviceIdentifier[]> => {
        const devices = await Device.findAll({
            attributes: ['platform', 'deviceId'],
            limit: 10000,
        });

        return devices.map((device) => ({
            platform: device.platform,
            deviceId: device.deviceId,
        }));
    },
    {
        wrapResult: true,
    },
);

interface DeviceState {
    locked: boolean;
    outputSpeech?: string;
    mordoviaUrl?: string;

    stationUrl?: string;
    userUrl?: string;
    code?: string;
}

export const getDeviceState = asyncJsonResponse(
    async (req, res): Promise<DeviceState> => {
        const start = Date.now()

        const { platform, device_id: deviceId } = req.query;

        if (!platform || !deviceId) {
            throw new RestApiError(
                'Query params "platform" and "device_id" are required',
                400,
            );
        }

        const device = (await Device.findOne({
            where: {
                deviceId,
                platform,
            },
            include: [Organization],
            rejectOnEmpty: true,
        }).catch(notFound('Device')))!;

        fillHist(dialogovoHistograms.afterDeviceSelect, (Date.now() - start));

        switch (device.status) {
            case Status.Active:
                fillHist(dialogovoHistograms.endGetState, Date.now() - start);
                return {
                    locked: false,
                };

            case Status.Inactive:
            case Status.Reset:
                const [activationCode] = await ActivationCode.findOrCreate({
                    where: {
                        deviceId: device.id,
                        createdAt: {
                            [Op.gte]: new Date(
                                Date.now() -
                                    config.app.customerActivationOperation
                                        .codeRefreshInterval,
                            ),
                        },
                    },
                    defaults: {
                        code: Math.random().toFixed(9).substr(2),
                        deviceId: device.id,
                        createdAt: new Date(),
                    } as ActivationCodeSchema,
                    logging: (...msg) => {
                        // @ts-ignore
                        const type = msg[1]?.type;
                        if(type){
                            switch (type) {
                                case 'SELECT':
                                    fillHist(dialogovoHistograms.startActivationCodeSelect, Date.now() - start)
                                    break;
                                case 'INSERT':
                                    fillHist(dialogovoHistograms.startActivationCodeCreate, Date.now() - start)
                                    break;
                            }
                        }

                    }
                });

                fillHist(dialogovoHistograms.endGetState, Date.now() - start);

                return {
                    locked: true,
                    outputSpeech: `Устройство заблокировано. Для активации откройте страницу ya.cc/activate (но это не точно) и введите код ${activationCode.code
                        .split('')
                        .map((s, i) => (i > 0 && i % 3 === 0 ? ' ' + s : s))
                        .join('')}`,
                    mordoviaUrl: `${config.dialogovo.mordoviaUrl}?code=${activationCode.code}`,
                    stationUrl: config.dialogovo.mordoviaUrl,
                    userUrl: 'http://ya.cc',
                    code: activationCode.code,
                };
        }
    },
    {
        wrapResult: true,
    },
);
