import { asyncJsonResponse, notFound } from '../utils';
import express from 'express';
import RestApiError from '../../lib/errors/restApi';
import { Organization, PromoCode } from '../../db';
import { Op } from 'sequelize';
import { OrganizationSchema } from '../../db/tables/organization';

const getConnectOrgId = (req: express.Request) => {
    const headerName = 'X-Org-Id';

    const orgIdString = req.header(headerName);
    if (!orgIdString) {
        throw new RestApiError(`Header ${headerName} is required`, 400);
    }

    const orgIdNumber = parseInt(orgIdString, 10);
    if (isNaN(orgIdNumber)) {
        throw new RestApiError(`Header ${headerName} must be int value`, 400);
    }

    return orgIdNumber;
};

export const getResourcesHandler = asyncJsonResponse(async (req) => {
    const organizations = await Organization.findAll({
        attributes: ['id', 'name'],
        where: {
            connectOrgId: getConnectOrgId(req),
        },
    });

    return {
        status: 'ok',
        metadata: organizations.reduce(
            (res, item) => ({ ...res, [item.id]: { name: item.name } }),
            {} as { [id: string]: { name: string } },
        ),
    };
});

export const getResourceHandler = asyncJsonResponse(async (req) => {
    const organization = (await Organization.findOne({
        where: {
            id: req.params.id,
            connectOrgId: getConnectOrgId(req),
        },
        rejectOnEmpty: true,
    }).catch(notFound('Resource')))!;

    return {
        status: 'ok',
        resource: {
            id: organization.id,
            metadata: {
                name: organization.name,
            },
        },
    };
});

export const createResourceHandler = asyncJsonResponse(
    async (req) => {
        const { name } = req.body;
        if (!name) {
            throw new RestApiError('Name is required', 400);
        }

        const organization = await Organization.create({
            name,
            connectOrgId: getConnectOrgId(req),
        } as OrganizationSchema);

        return {
            status: 'ok',
            resource: {
                id: organization.id,
                metadata: {
                    name: organization.name,
                },
            },
        };
    },
    { putPostStatus: 201 },
);

export const updateResourceHandler = asyncJsonResponse(async (req) => {
    const organization = (await Organization.findOne({
        where: {
            id: req.params.id,
            connectOrgId: getConnectOrgId(req),
        },
        rejectOnEmpty: true,
    }).catch(notFound('Resource')))!;

    const { name } = req.body;
    if (!name) {
        throw new RestApiError('Name is required', 400);
    }

    await organization.update({ name } as OrganizationSchema);

    return {
        status: 'ok',
        resource: {
            id: organization.id,
            metadata: {
                name: organization.name,
            },
        },
    };
});

export const deleteResourceHandler = asyncJsonResponse(async (req) => {
    const organization = await Organization.findOne({
        where: {
            id: req.params.id,
            connectOrgId: getConnectOrgId(req),
        },
        include: [
            {
                model: PromoCode,
                attributes: ['id'],
                required: false,
                as: 'promoCodes',
                where: {
                    status: { [Op.eq]: null },
                },
            },
        ],
    });

    if (organization) {
        if (organization.promoCodes!.length > 0) {
            throw new RestApiError(
                'Unable to delete Resource with unused promocodes',
                405,
            );
        }

        await organization.destroy();
    }

    return { status: 'ok' };
});
