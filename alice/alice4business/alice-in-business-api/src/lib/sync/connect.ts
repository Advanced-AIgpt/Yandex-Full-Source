import { ConnectOrganization, sequelize, Organization, Permission, User } from '../../db';
import {
    ConnectOrganizationInstance,
    ConnectOrganizationSchema,
} from '../../db/tables/connectOrganization';
import { OrganizationInstance } from '../../db/tables/organization';
import RestApiError from '../errors/restApi';
import Connect from '../../services/connect';
import log from '../log';
import got from 'got';
import ip from 'ip';
import {
    PermissionSchema,
    Source,
    Types as PermissionTypes,
} from '../../db/tables/permission';
import { literal, WhereValue } from 'sequelize';
import { UserSchema } from '../../db/tables/user';

export const syncOrganization = async (orgId: number, logContext?: object) => {
    logContext = {
        ...logContext,
        connectOrgId: orgId,
    };

    log.info('sync connectOrganization: start', logContext);

    // 1. Load bound organizations (don't wait result)
    const boundOrganizations = Organization.findAll({
        attributes: ['id'],
        where: {
            connectOrgId: orgId,
        },
    });

    // 2. Get connectOrganization from DB or create it. Update organization info (name, active)
    const orgInfo = await _getConnectOrganizationAndUpdateInfo(
        orgId,
        boundOrganizations,
        logContext,
    );
    if (!orgInfo) {
        return;
    }

    // 3. Update resources in organization
    const [connectOrganization, adminUid] = orgInfo;
    await _updateConnectOrganizationResources(
        connectOrganization,
        boundOrganizations,
        adminUid,
        logContext,
    );

    // 4. Update grants
    await _updateConnectOrganizationPermissions(
        connectOrganization,
        boundOrganizations,
        logContext,
    );

    // Finish
    const now = new Date();
    await ConnectOrganization.update(
        {
            lastSync: now,
            updatedAt: now,
        } as ConnectOrganizationSchema,
        {
            where: {
                id: connectOrganization.id,
                updatedAt: connectOrganization.updatedAt as WhereValue,
            },
            silent: true, // использовать значение updatedAt из values
            returning: true,
        },
    ).then(_organizationUpdateHelper(connectOrganization));

    log.info('sync connectOrganization: done', logContext);
};

export const _getConnectOrganizationAndUpdateInfo = async (
    orgId: number,
    boundOrganizationsPromise: Promise<OrganizationInstance[]>,
    logContext?: object,
): Promise<[ConnectOrganizationInstance, number] | null> => {
    const [connectOrganization, isNew] = await ConnectOrganization.findOrBuild({
        where: { id: orgId },
        defaults: { id: orgId },
    });

    if (!isNew) {
        await _checkLock(connectOrganization);
    }

    log.info('sync connectOrganization: load organization info', logContext);

    const organizationInfo = await Connect.getOrganization(connectOrganization.id, [
        'id',
        'name',
        'admin_id',
    ]).catch(async (error) => {
        if (error instanceof got.HTTPError && error.body) {
            if (
                error.statusCode === 403 &&
                (error.body as any).code === 'service_is_not_enabled'
            ) {
                return null;
            }

            if (
                error.statusCode === 404 &&
                (error.body as any).code === 'unknown_organization'
            ) {
                return null;
            }
        }

        throw new RestApiError('failed to load organization info', 500, {
            origError: error,
        });
    });

    if (organizationInfo) {
        log.debug('sync connectOrganization: got organization info', {
            ...logContext,
            organizationInfo,
        });

        if (isNew) {
            connectOrganization.set({
                active: true,
                name: organizationInfo.name?.trim(),
            });

            await connectOrganization.save();
        } else {
            await ConnectOrganization.update(
                {
                    active: true,
                    name: organizationInfo.name?.trim(),
                } as ConnectOrganizationSchema,
                {
                    where: {
                        id: connectOrganization.id,
                        updatedAt: connectOrganization.updatedAt as WhereValue,
                    },
                    returning: true,
                },
            )
                .then(_organizationUpdateHelper(connectOrganization))
                .catch((operationUpdateError) => {
                    throw new RestApiError(
                        'Failed to update connectOrganization info',
                        500,
                        {
                            origError: operationUpdateError,
                        },
                    );
                });
        }

        return [connectOrganization, organizationInfo.admin_id!];
    }

    if (isNew) {
        log.info(
            'sync connectOrganization: organization info is unavailable; organization will not be saved',
            logContext,
        );
    } else {
        log.info(
            'sync connectOrganization: organization info is unavailable; organization will be disabled',
            logContext,
        );

        const boundOrganizationIds = (await boundOrganizationsPromise)
            .map((organization) => organization.id)
            .sort();

        await sequelize.transaction(async (transaction) => {
            const now = new Date();

            if (boundOrganizationIds.length > 0) {
                log.info(
                    'sync connectOrganization: removing permissions for organizations (as Connect resources)',
                    {
                        ...logContext,
                        organizationId: boundOrganizationIds,
                    },
                );

                await Permission.destroy({
                    where: {
                        source: Source.Connect,
                        organizationId: boundOrganizationIds,
                    },
                    transaction,
                });
            }

            await ConnectOrganization.update(
                {
                    active: false,
                    lastSync: now,
                    updatedAt: now,
                } as ConnectOrganizationSchema,
                {
                    where: {
                        id: connectOrganization.id,
                        updatedAt: connectOrganization.updatedAt as WhereValue,
                    },
                    silent: true, // использовать значение updatedAt из values
                    returning: true,

                    transaction,
                },
            )
                .then(_organizationUpdateHelper(connectOrganization))
                .catch((operationUpdateError) => {
                    throw new RestApiError(
                        'Failed to update connectOrganization info',
                        500,
                        {
                            origError: operationUpdateError,
                        },
                    );
                });
        });

        log.debug(
            'sync connectOrganization: organization successfully disabled',
            logContext,
        );
    }

    return null;
};

export const _updateConnectOrganizationResources = async (
    connectOrganization: ConnectOrganizationInstance,
    organizationsPromise: Promise<OrganizationInstance[]>,
    adminUid: number,
    logContext?: object,
) => {
    await _checkLock(connectOrganization);

    log.info('sync connectOrganization: loading resources', logContext);
    const [resources, organizations] = await Promise.all([
        Connect.getResources(connectOrganization.id),
        organizationsPromise,
    ]);

    const resourceIdSet = new Set(resources.map((resource) => resource.id));
    const organizationIdSet = new Set(
        organizations.map((organization) => organization.id),
    );

    const counters = {
        toCreate: 0,
        toDelete: 0,
        notAffected: 0,
    };
    const actions = [] as Promise<any>[];
    // 1. Delete connect resources with unknown id
    for (const resourceId of resourceIdSet.values()) {
        if (organizationIdSet.delete(resourceId)) {
            ++counters.notAffected;
        } else {
            actions.push(
                Connect.deleteResource(connectOrganization.id, resourceId, {
                    uid: adminUid,
                    ip: ip.address(),
                }),
            );
            ++counters.toDelete;
        }
    }
    // 2. Create connect resources
    for (const organizationId of organizationIdSet.values()) {
        actions.push(
            Connect.createResource(connectOrganization.id, { id: organizationId }),
        );
        ++counters.toCreate;
    }

    if (actions.length === 0) {
        log.info('sync connectOrganization: resources are up to date', {
            ...logContext,
            counters,
        });
        return;
    }

    log.info('sync connectOrganization: resources have to be updated', {
        ...logContext,
        counters,
    });

    await Promise.all(actions);

    log.debug('sync connectOrganization: resources updated successfully', {
        ...logContext,
        counters,
    });
};

export const _updateConnectOrganizationPermissions = async (
    connectOrganization: ConnectOrganizationInstance,
    boundOrganizationsPromise: Promise<OrganizationInstance[]>,
    logContext?: object,
) => {
    const boundOrganizationIDs = new Set<string>(
        (await boundOrganizationsPromise).map((organization) => organization.id),
    );

    if (boundOrganizationIDs.size === 0) {
        log.info(
            'sync connectOrganization: permission sync skipped (no resources)',
            logContext,
        );
        return;
    }

    log.info('sync connectOrganization: loading users from Connect', logContext);
    await _checkLock(connectOrganization);

    const users = await Connect.getUsers(connectOrganization.id, ['id', 'email'], {
        query: { is_robot: 0 },
    });
    await _checkLock(connectOrganization);

    await User.bulkCreate(
        users.map((user) => ({
            id: user.id.toString(10),
            login: user.email,
        })) as UserSchema[],
        { ignoreDuplicates: true },
    );
    log.debug('sync connectOrganization: user list saved', logContext);

    const permissionTypes = new Set(Object.keys(PermissionTypes));
    const permissions = [] as PermissionSchema[];

    // TODO: fix getUserResources
    if (false && users.length < boundOrganizationIDs.size * permissionTypes.size) {
        log.info('sync connectOrganization: loading permissions (by user)', logContext);

        const requestParamsQueue = users.map((user) => user.id);
        do {
            await _checkLock(connectOrganization);

            const requestParamsBatch = requestParamsQueue.splice(0, 10);
            await Promise.all(
                requestParamsBatch.map((uid) =>
                    Connect.getUserResources(
                        connectOrganization.id,
                        {
                            uid,
                        },
                        {
                            relation_name: Array.from(permissionTypes),
                        },
                    ).then((userResources) => {
                        for (const resource of userResources) {
                            for (const relation of resource.relations || []) {
                                if (
                                    boundOrganizationIDs.has(resource.id) &&
                                    permissionTypes.has(relation.name)
                                ) {
                                    permissions.push({
                                        organizationId: resource.id,
                                        uid,
                                        type: relation.name as any,
                                        source: Source.Connect,
                                    });
                                }
                            }
                        }
                    }),
                ),
            );
        } while (requestParamsQueue.length > 0);
    } else {
        log.info(
            'sync connectOrganization: loading permissions (by permissions)',
            logContext,
        );

        const requestParamsQueue = [] as {
            organizationId: string;
            type: keyof typeof PermissionTypes;
        }[];
        for (const organizationId of boundOrganizationIDs) {
            for (const type of permissionTypes as Set<keyof typeof PermissionTypes>) {
                requestParamsQueue.push({ organizationId, type });
            }
        }
        do {
            await _checkLock(connectOrganization);

            const requestParamsBatch = requestParamsQueue.splice(0, 10);
            await Promise.all(
                requestParamsBatch.map((params) =>
                    Connect.getUsers(connectOrganization.id, ['id'], {
                        query: {
                            is_robot: 0,
                            resource: params.organizationId,
                            resource_relation_name: params.type,
                        },
                    }).then((userWithPermission) => {
                        for (const user of userWithPermission) {
                            permissions.push({
                                organizationId: params.organizationId,
                                uid: user.id,
                                type: params.type,
                                source: Source.Connect,
                            });
                        }
                    }),
                ),
            );
        } while (requestParamsQueue.length > 0);
    }

    await _checkLock(connectOrganization);

    log.info('sync connectOrganization: saving permissions', logContext);
    await sequelize.transaction(async (transaction) => {
        await Permission.destroy({
            where: {
                source: Source.Connect,
                organizationId: Array.from(boundOrganizationIDs),
            },
            transaction,
        });

        await Permission.bulkCreate(permissions as PermissionSchema[], {
            ignoreDuplicates: true,
            transaction,
        });
    });

    log.info('sync connectOrganization: permissions updated successfully', logContext);
};

export const _organizationUpdateHelper = (
    connectOrganization: ConnectOrganizationInstance,
) => ([count, connectOrganizations]: [number, ConnectOrganizationInstance[]]) => {
    if (count !== 1 || connectOrganizations.length !== 1) {
        throw new Error('ConnectOrganization was updated by another worker');
    }

    connectOrganization.set(connectOrganizations[0].get(), {
        raw: true,
        reset: true,
    });

    return connectOrganization;
};

export const _checkLock = async (connectOrganization: ConnectOrganizationInstance) => {
    return ConnectOrganization.update(
        {
            name: literal('name'), // sequelize ignores updates with timestamp field only
        } as ConnectOrganizationSchema & { name: any },
        {
            where: {
                id: connectOrganization.id,
                updatedAt: connectOrganization.updatedAt as WhereValue,
            },
            returning: true,
        },
    ).then(_organizationUpdateHelper(connectOrganization));
};
