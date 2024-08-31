import {GotError} from 'got';
import ip from 'ip';

import { asyncJsonResponse } from '../utils';
import { ConnectOrganization, Device, Organization, Permission, sequelize } from '../../db';
import { OrganizationInstance} from '../../db/tables/organization';
import { Source } from '../../db/tables/permission';
import Connect from '../../services/connect';
import config from '../../lib/config';

import { logSupportOperation } from './utils';

const OK_STATUS = 'ok'
const NOT_OK_STATUS = 'not ok'
const YES = 'Да'
const NO = 'Нет'

export const getAllOrganizations = asyncJsonResponse(
    async () => {
        const organizations = await Organization.findAll();

        const connectOrgs = {} as Record<string, string>;
        const byConnectOrgId = {} as Record<string, OrganizationInstance[]>;

        for (const organization of organizations) {
            const orgId = organization.connectOrgId || '';

            if (byConnectOrgId[orgId]) {
                byConnectOrgId[orgId].push(organization);
            } else {
                byConnectOrgId[orgId] = [organization];

                if (orgId) {
                    const connectOrganization = await Connect.getOrganization(
                        orgId,
                        ['name'],
                        { tvmSrc: 'connect-controller' },
                    );

                    connectOrgs[orgId] = `${connectOrganization.name} (${orgId})`;
                }
            }
        }

        const result = [];
        for (const orgId of Object.keys(byConnectOrgId).sort(
            (a, b) => parseInt(a, 10) - parseInt(b, 10),
        )) {
            result.push(
                ...byConnectOrgId[orgId]
                    .sort((a, b) => a.name.localeCompare(b.name))
                    .map((org) => ({
                        value: org.id,
                        name: org.name
                    })),
            );
        }

        return result;
    },
    { wrapResult: true },
);

export const getConnectOrganizationsChoiceList = asyncJsonResponse(
    async () => {
        const organizations = await ConnectOrganization.findAll();

        return organizations
            .sort((a, b) => a.id - b.id)
            .map((org) => ({
                value: org.id,
                name: `${org.id}: ${org.name}${org.active ? ' (active)' : ''}`,
            }));
    },
    { wrapResult: true },
);

export const createOrganization = asyncJsonResponse(
    async (req) => {
        const operationType = 'organizations:create';
        const logPuid = req.user.uid.toString();

        try {
            const organizationData = req.body.params;
            organizationData.usesRooms = organizationData.usesRooms === YES;
            organizationData.maxStationVolume = Number(organizationData.maxStationVolume || '7');

            const organization = await Organization.create(organizationData);

            if (organization.connectOrgId) {
                await Connect.createResource(organization.connectOrgId, {
                    id: organization.id,
                }).catch(async (err: GotError) => {
                    if (err.name === 'HTTPError') {
                        if (err.statusCode === 409) {
                            const id = await logSupportOperation(operationType, false, logPuid, err.message)
                            return {
                                status: NOT_OK_STATUS,
                                message: `Operation failed with id ${id}`
                            }
                        }
                        if (err.statusCode === 403) {
                            const msg = `${config.connect.selfSlug} service is disabled for Connect organization ${organization.connectOrgId}. Please sync resources later`;
                            await logSupportOperation(operationType, false, logPuid, msg)
                            return {
                                status: NOT_OK_STATUS,
                                message: msg
                            }
                        }
                    }
                    throw err;
                });
            }

            await logSupportOperation(operationType, true, logPuid)
            return {
                status: OK_STATUS,
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

export const setMaxVolume = asyncJsonResponse(
    async (req) => {
        const operationType = 'organizations:set-max-volume';
        const logPuid = req.user.uid.toString();

        try {
            const {organizationId, newMaxStationVolume} = req.body.params
            const organization = await Organization.findByPk(organizationId);
            if (!organization) {
                const msg = `No such organization with id ${organizationId}`;
                await logSupportOperation(operationType, false, logPuid, msg)
                return {
                    status: NOT_OK_STATUS,
                    message: msg
                }
            }
            await organization.update({maxStationVolume: Number(newMaxStationVolume)});
            await logSupportOperation(operationType, true, logPuid)
            return {
                status: OK_STATUS,
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

export const changeOrganization = asyncJsonResponse(
    async (req) => {
        const operationType = 'organizations:change';
        const logPuid = req.user.uid.toString();

        try {
            const newData = req.body.params
            newData.usesRooms = newData.usesRooms === YES ? true : (newData.usesRooms === null ? null : false);
            newData.maxStationVolume = newData.maxStationVolume === null ? null : Number(newData.maxStationVolume)

            const organization = await Organization.findByPk(newData.organizationId);
            if (!organization) {
                const msg = `No such organization with id ${newData.organizationId}.`;
                await logSupportOperation(operationType, false, logPuid, msg)
                return {
                    status: NOT_OK_STATUS,
                    message:  msg
                }
            }

            const oldConnectId = organization.connectOrgId;
            await sequelize.transaction(async (transaction) => {
                await organization.update(newData, { transaction });

                if (organization.connectOrgId !== oldConnectId) {
                    await Permission.destroy({
                        where: {
                            organizationId: organization.id,
                            source: Source.Connect,
                        },
                        transaction,
                    });
                }
            });

            if (organization.connectOrgId !== oldConnectId) {
                if (oldConnectId) {
                    try {
                        const {
                            admin_id,
                        } = await Connect.getOrganization(
                            oldConnectId,
                            ['admin_id'],
                            { tvmSrc: 'connect-controller' },
                        );

                        await Connect.deleteResource(oldConnectId, organization.id, {
                            uid: admin_id!,
                            ip: ip.address(),
                        });
                    } catch (e) {
                        const msg = `Failed to delete resource in Connect organization ${newData.connectOrgId}. Sync it later.`
                        await logSupportOperation(operationType, false, logPuid, msg)
                        return {
                            status: NOT_OK_STATUS,
                            message:  msg
                        }
                    }
                }

                if (organization.connectOrgId) {
                    try {
                        await Connect.createResource(organization.connectOrgId, {
                            id: organization.id,
                        });
                    } catch (e) {
                        const msg = `Failed to create resource in Connect organization ${newData.connectOrgId}. Sync it later.`
                        await logSupportOperation(operationType, false, logPuid, msg)
                        return {
                            status: NOT_OK_STATUS,
                            message:  msg
                        }
                    }
                }
            }
            await logSupportOperation(operationType, true, logPuid)
            return {
                status: OK_STATUS,
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

export const getDefaults = asyncJsonResponse(
    async (req) => {
        const {id} = req.params;
        const {
            name,
            connectOrgId,
            templateUrl,
            imageUrl,
            infoUrl,
            infoTitle,
            infoSubtitle,
            usesRooms,
            maxStationVolume,
        } = await Organization.findByPk(id, {
            rejectOnEmpty: true,
        });

        return {
            name,
            connectOrgId,
            templateUrl,
            imageUrl,
            infoUrl,
            infoTitle,
            infoSubtitle,
            usesRooms: usesRooms ? YES : (usesRooms === null ? null : NO),
            maxStationVolume: maxStationVolume ? maxStationVolume.toString() : null
        }
    },
    { wrapResult: true }
);

export const getDevices = asyncJsonResponse(
    async (req) => {
        const {organizationId} = req.params;

        const organization = (await Organization.findOne({
            where: { id: organizationId },
            include: [
                {
                    model: Device,
                    where: {
                        roomId: null
                    }
                },
            ],
            rejectOnEmpty: false,
        }))!;

        if (!organization) {
            return []
        }

        const result = [];
        for (const device of organization.devices!) {
            result.push({
                value: device.id,
                name: `${device.platform} (${device.deviceId})`
            });
        }
        return result;
    },
    { wrapResult: true },
)


