import temp from 'temp';
import util from 'util';
import path from 'path';
import fs from 'fs';
import config from '../config';
import RestApiError from '../errors/restApi';
import { DeviceInstance, DeviceSchema, Status } from '../../db/tables/device';
import { OrganizationInstance } from '../../db/tables/organization';
import createGot from '../../services/got';
import pdfFillForm from 'pdf-fill-form';
import log from '../log';
import { Device, Organization } from '../../db';
import { sendPush } from '../../services/push/send';

const templateDir = temp.mkdirSync('template-dir');
const got = createGot({ sourceName: 'agreementTemplate' });

const getTemplatePath = async (organization: OrganizationInstance) => {
    const fileName = `template-agreement-${organization.id}.pdf`;
    const templatePath = path.join(templateDir, fileName);

    if (fs.existsSync(templatePath)) {
        const modifiedTime = fs.statSync(templatePath).mtimeMs;
        const ageMs = Date.now() - modifiedTime;

        if (ageMs < config.app.agreementCacheTime) {
            return templatePath;
        }
    }

    if (!organization.templateUrl) {
        throw new RestApiError('Template not set', 404);
    }

    const inputBuffer = await got(organization.templateUrl, { encoding: null }).then(
        (res) => {
            if (
                res.statusCode !== 200 ||
                res.headers['content-type'] !== 'application/pdf'
            ) {
                throw new RestApiError('Failed to download template', 500, {
                    payload: {
                        status: res.statusCode,
                    },
                });
            }
            return res.body as Buffer;
        },
        (error) => {
            throw new RestApiError('Failed to download template', 500, {
                origError: error,
            });
        },
    );
    await util.promisify(fs.writeFile)(templatePath, Buffer.from(inputBuffer));
    return templatePath;
};

export const getAgreement = async (device: DeviceInstance) => {
    if (device.status === Status.Reset || !device.kolonkishLogin) {
        throw new RestApiError('Device was not reset proper', 409);
    }

    if (!device.organization) {
        await device.reload({ include: [Organization] });
    }
    const templatePath = await getTemplatePath(device.organization!);

    const data = {
        login: `${device.kolonkishLogin}@yandex.ru`,
    };

    const agreement = (await pdfFillForm
        .write(templatePath, data, { save: 'pdf' })
        .catch((error: any) => {
            log.warn('Failed to fill PDF form ', { error });
            throw new RestApiError('Error when filling out PDF', 500, {
                origError: error,
            });
        })) as Buffer;

    await Device.update(
        {
            agreementAccepted: true,
        } as DeviceSchema,
        {
            where: {
                id: device.id,
                kolonkishId: device.kolonkishId,
                kolonkishLogin: device.kolonkishLogin,
            },
        },
    ).then(([updatedDeviceCount]) => {
        if (updatedDeviceCount !== 1) {
            throw new RestApiError(
                'Device state has changed during agreement generating',
                409,
            );
        }
    });
    sendPush({
        topic: device.organizationId,
        event: 'device-state',
        payload: device.id,
    });

    return agreement;
};
