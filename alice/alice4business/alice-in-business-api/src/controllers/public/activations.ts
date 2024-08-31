import { Op } from "sequelize";
import { ActivationLink } from "../../db";
import { ActivationLinkSchema } from "../../db/tables/activationLink";
import { DeviceInstance } from "../../db/tables/device";
import { RoomInstance } from "../../db/tables/room";
import RestApiError from "../../lib/errors/restApi";
import log from '../../lib/log';

export const resetActivationLinks = async (entity: DeviceInstance | RoomInstance) => {
    return await ActivationLink.update(
        {
            enabled: false
        } as ActivationLinkSchema,
        {
            where: {
                [Op.or]: [
                    {
                        deviceId: entity.id
                    },
                    {
                        roomId:  entity.id
                    }
                ],
                enabled: true
            }
        }
    )
        .then(([updatedCount]) => {
            log.info("Activation links are reset", {
                activationLinkReset: {
                    id: entity.id,
                    updatedCount
                }
            })
        })
        .catch((error) => {
            log.warn("Unable to reset activation links", {
                activationLinkReset: {
                    id: entity.id,
                    error
                }
            })
        });
}


export const createActivation = async (entity: DeviceInstance | RoomInstance,
    firstActivationDate: Date,
    lastActivationDate: Date) => {
    const now = new Date();
    if (isNaN(lastActivationDate.getTime())) {
        throw new RestApiError('last_activation_date parameter is required', 400);
    }
    if (lastActivationDate <= now) {
        throw new RestApiError('last_activation_date should be in future', 400);
    }
    if (isNaN(firstActivationDate.getTime())) {
        throw new RestApiError('first_activation_date parameter is required', 400);
    }
    if (firstActivationDate > lastActivationDate) {
        throw new RestApiError('last_activation_date should after first_activation_date', 400);
    }

    return await ActivationLink.create({
        roomId: entity instanceof RoomInstance ? entity.id : null,
        deviceId: entity instanceof DeviceInstance ? entity.id : null,
        enabled: true,
        activeSince: firstActivationDate,
        activeTill: lastActivationDate,
    } as ActivationLinkSchema);
}