import React, { FC, useEffect, useState } from 'react';
import { IOrganization } from '../../model/organization';
import block from 'bem-cn';

import './OrganizationChoicePage.scss';
import { H1, Page } from '../Typo/Typo';
import Loading from '../Loading/Loading';
import Alert from '../Alert/Alert';
import Link from '../Link/Link';
import { IConnectOrganization } from '../../model/connectOrganization';
import { Icon } from 'lego-on-react';
import { useAppStateContext } from './App';
import { useConsoleApi } from '../../context/console-api';
import { useRoutes } from '../../context/routes';

const b = block('OrganizationChoicePage');

interface IOrganizationGroup {
    connect?: IConnectOrganization;
    organizations: IOrganization[];
}

const OrganizationChoicePage: FC = () => {
    const routes = useRoutes();
    const api = useConsoleApi();
    const appState = useAppStateContext();

    const [organizationList, setOrganizationList] = useState<IOrganizationGroup[]>([]);
    const [isLoading, setIsLoading] = useState(true);
    const [isError, setIsError] = useState(false);
    const [error, setError] = useState('Произошла ошибка');
    const fetch = async () => {
        try {
            const data = await api.getAllOrganizations();
            data.sort((o1, o2) => o1.name.toLowerCase().localeCompare(o2.name.toLowerCase()));

            const groups = new Map<number, IOrganizationGroup>();
            for (const organization of data) {
                const connectOrgId =
                    (organization.connectOrganization && organization.connectOrganization.id) || 0;

                if (!groups.has(connectOrgId)) {
                    groups.set(connectOrgId, {
                        connect: organization.connectOrganization,
                        organizations: [],
                    });
                }

                delete organization.connectOrganization;

                groups.get(connectOrgId)!.organizations.push(organization);
            }

            setOrganizationList(
                Array.from(groups.values()).sort((g1, g2) =>
                    ((g1.connect && g1.connect.name) || '')
                        .toLowerCase()
                        .localeCompare(((g2.connect && g2.connect.name) || '').toLowerCase()),
                ),
            );
        } catch (e) {
            if (e.code === 403) {
                setError('Нет доступа к ceрвису');
            }
            if (e.code === 404) {
                setError('Нет привязаных к вам организаций');
            }
            setIsError(true);
        } finally {
            setIsLoading(false);
        }
    };
    useEffect(() => void fetch(), []);

    const render = () => {
        if (isError) {
            return (
                <Alert size='l' type='error'>
                    {error}
                </Alert>
            );
        }
        if (isLoading) {
            return (
                <div className={'page-center'}>
                    <Loading size='s' />
                </div>
            );
        }
        if (!Boolean(organizationList && organizationList.length)) {
            return (
                <Alert size='l' type='info'>
                    Нет привязанных к вам организаций
                </Alert>
            );
        }

        const userConnectOrganizations = new Set(
            (appState.user.connect || '').split(',').map((x) => parseInt(x, 10)),
        );

        return organizationList.map((group, i) => (
            <div className={b('group')} key={(group.connect && group.connect.id) || 0}>
                {group.connect && (
                    <div className={b('group-header')}>
                        <span className={b('group-header-title')}>
                            <Icon url={routes.assets('images/connect.svg')} />
                            {group.connect!.name}
                        </span>
                        {userConnectOrganizations.has(group.connect.id) && (
                            <span className={b('group-header-links')}>
                                <Link
                                    external
                                    to={routes.connect.portal(group.connect!.id)}
                                    target='_blank'
                                >
                                    <Icon url={routes.assets('images/new-tab.svg')} />
                                    Коннект
                                </Link>
                                <Link
                                    external
                                    to={routes.connect.settings(group.connect!.id)}
                                    target='_blank'
                                >
                                    <Icon url={routes.assets('images/profile.svg')} />
                                    Управление правами
                                </Link>
                            </span>
                        )}
                    </div>
                )}
                {group.organizations.map((org) => (
                    <Link key={org.id} to={routes.app.organization(org.id)}>
                        <div className={b('organization')}>
                            <span className={b('organization-name')}>{org.name}</span>{' '}
                            <span className={b('organization-promo-count')}>
                                {org.promoCount}
                                <Icon url={routes.assets('images/promocode.svg')} />
                            </span>
                        </div>
                    </Link>
                ))}
            </div>
        ));
    };
    return (
        <Page>
            <Page.Head>
                <Page.HeadContent>
                    <H1>Организации</H1>
                </Page.HeadContent>
            </Page.Head>
            <Page.Content>
                <Page.Page>{render()}</Page.Page>
            </Page.Content>
        </Page>
    );
};

export default OrganizationChoicePage;
