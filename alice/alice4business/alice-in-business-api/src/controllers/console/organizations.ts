import { asyncJsonResponse, notFound } from '../utils';
import { ConnectOrganization, Device, Operation, PromoCode } from '../../db';
import { Op } from 'sequelize';
import * as ACL from '../../lib/acl';
import { OrganizationInstance } from '../../db/tables/organization';
import config from '../../lib/config';
import {
    OperationInstance,
    Scope as OperationScope,
    Status as OperationStatus,
    UserScope,
} from '../../db/tables/operation';
import RestApiError from '../../lib/errors/restApi';

const organizationIncludes = [
    {
        model: ConnectOrganization,
        attributes: ['id', 'name'],
    },
    {
        model: PromoCode,
        attributes: ['id'],
        required: false,
        where: {
            status: { [Op.eq]: null },
        },
    },
];

const pickOrganization = (organization: OrganizationInstance) => ({
    id: organization.id,
    name: organization.name,
    connectOrganization: organization.connectOrganization,
    promoCount: organization.promoCodes?.length ?? 0,
    usesRooms: organization.usesRooms
});

export const getAllOrganizations = asyncJsonResponse(
    async (req, res) => {
        const orgs = await ACL.getOrganizations(req.user, [], {
            include: organizationIncludes,
        });

        return orgs.map(pickOrganization);
    },

    { wrapResult: true },
);

export const getOrganization = asyncJsonResponse(
    async (req, res) => {
        const { id } = req.params;
        const org = await ACL.getOrganization(
            req.user,
            [],
            { id },
            {
                include: organizationIncludes,
            },
        ).catch(notFound('Organization'));

        return pickOrganization(org);
    },
    { wrapResult: true },
);

export interface SerializedOperation {
    id: string;
    date: Date;
    kolonkish?: {
        login?: string;
        uid: string;
    };
    type: 'activate' | 'reset' | 'promo-activate' | 'reset-room' | 'activate-room';
    status: OperationStatus;
    context?: OperationScope['context'];
    deviceId: string;
    devicePk: string;
    userLogin?: string;
    shouldActivatePromo?: boolean;
}

const serializeOperation = (op: OperationInstance): SerializedOperation => {
    const { id, createdAt, scope, payload, type, device } = op;
    const kolonkish = payload
        ? { login: payload.kolonkishLogin, uid: payload.kolonkishUid }
        : undefined;
    return {
        id,
        kolonkish,
        type,
        date: createdAt,
        status: op.status,
        context: scope?.context,
        userLogin:
            scope?.context !== 'customer' ? (scope as UserScope)?.userLogin : undefined,
        deviceId: device!.deviceId,
        devicePk: device!.id,
    };
};

export const getOrganizationHistory = asyncJsonResponse(
    async (req, res) => {
        const { id: organizationId } = req.params;
        const { next, devicePk } = req.query;

        if (
            (next && typeof next !== 'string') ||
            (devicePk && typeof devicePk !== 'string')
        ) {
            throw new RestApiError('Incorrect query type', 400);
        }

        await ACL.getOrganization(req.user, [], { id: organizationId }).catch(
            notFound('Organization'),
        );
        if (devicePk) {
            await Device.findOne({
                where: {
                    id: devicePk,
                    organizationId,
                },
                paranoid: false,
                rejectOnEmpty: true,
            }).catch(notFound('Device'));
        }

        const operations = (
            await Operation.findAll({
                where: next ? { createdAt: { [Op.lte]: next } } : undefined,
                order: [['createdAt', 'DESC']],
                limit: config.app.historyBatchSize + 1,
                include: [
                    {
                        model: Device,
                        where: devicePk
                            ? { organizationId, id: devicePk }
                            : { organizationId },
                        required: true,
                        paranoid: false,
                    },
                ],
            })
        ).map(serializeOperation);

        return {
            operations: operations.slice(0, config.app.historyBatchSize),
            next:
                operations.length > config.app.historyBatchSize
                    ? operations[config.app.historyBatchSize].date
                    : undefined,
        };
    },
    { wrapResult: true },
);
