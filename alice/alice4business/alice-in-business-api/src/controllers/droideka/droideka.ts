import { Op } from "sequelize";
import { Device, Organization } from "../../db";
import { Platform } from "../../db/tables/device";
import RestApiError from "../../lib/errors/restApi";
import { asyncJsonResponse, notFound } from "../utils";

interface TVInfo {
    imageUrl: string | null;
    infoUrl: string | null;
    title: string | null;
    subtitle: string | null;
};

export const getTVList = asyncJsonResponse(
    async (req): Promise<string[]> => {
        let { page, limit } = req.query;
        if (!page) {
            page = 0;
        } else {
            page = Number(page);
        }
        if (!limit) {
            limit = 1000;
        } else {
            limit = Number(limit);
        }
        if (isNaN(page) || page < 0) {
            throw new RestApiError("Invalid 'page' value", 400);
        }
        if (isNaN(limit) || limit <= 0) {
            throw new RestApiError("Invalid 'limit' value", 400);
        }
        const tvPlatforms = Object.values(Platform).filter(v=>v.startsWith('yandex_tv_') || v.startsWith('yandexmodule_'));
        const devices = await Device.findAll({
            attributes: ['deviceId'],
            offset: page * limit,
            limit,
            order: ['created_at'],
            where: {
                platform: {
                    [Op.in]: tvPlatforms,
                }
            }
        });
        return devices.map(device => device.deviceId);
    },
    {
        wrapResult: true
    }
);

export const getTVInfo = asyncJsonResponse(
    async (req): Promise<TVInfo> => {
        const tvPlatforms = Object.values(Platform).filter(v=>v.startsWith('yandex_tv_') || v.startsWith('yandexmodule_'));
        const {device_id} = req.query;
        if (!device_id) {
            throw new RestApiError("Query param 'device_id' is required", 400);
        }
        const device = await Device.findOne({
            where: {
                deviceId: device_id,
                platform: {
                    [Op.in]:  tvPlatforms,
                }
            },
            include: [{model: Organization, required: true}],
            rejectOnEmpty: true,        
        }).catch(notFound("Device"));
        return {
            imageUrl: device!.organization!.imageUrl,
            infoUrl: device!.organization!.infoUrl,
            title: device!.organization!.infoTitle,
            subtitle: device!.organization!.infoSubtitle,
        }        
    },
    {
        wrapResult: true
    }
);