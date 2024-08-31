import block from 'bem-cn';
import { observer, useLocalStore, useObserver } from 'mobx-react';
import React, { FC, useEffect, Suspense } from 'react';
import { Redirect, Route, Switch, useLocation, useParams } from 'react-router-dom';
import { useRoutes } from '../../context/routes';
import { DeviceListStore } from '../../store/device-list';
import { useModals } from '../../utils/modals/hooks';
import Alert from '../Alert/Alert';
import DeviceCreation from '../DeviceCreation/DeviceCreation';
import DeviceList from '../DeviceList/DeviceList';
import Link from '../Link/Link';
import Loading from '../Loading/Loading';
import Menu from '../Menu/Menu';
import { H1, Page } from '../Typo/Typo';
import { Button, Icon } from 'lego-on-react';
import './OrganizationPage.scss';
import SettingsPage from '../Settings/SettingsPage';
import { OrganizationStore } from '../../store/organization';
import '@yandex-int/xiva-websocket';
import { serializeQueryParams } from '../../lib/utils';
import { useAppStateContext } from './App';
import { useConsoleApi } from '../../context/console-api';
import { RoomListStore } from '../../store/room-list';
import RoomCreation from '../RoomCreation/RoomCreation';

const RoomList = React.lazy(() => import( '../RoomList/RoomList'));
const HistoryPageWrap = React.lazy(() => import('../History'));
const HistoryPageComponent: FC = () => {
    return (
        <Suspense fallback={
            <div className={'page-center'}>
                <Loading size='s' />
            </div>
        }>
            <HistoryPageWrap/>
        </Suspense>
    )
}

const OrganizationPage: FC = () => {
    const routes = useRoutes();
    const api = useConsoleApi();
    const appState = useAppStateContext();

    const { organizationId } = useParams();

    if (!organizationId) {
        return null;
    }

    const store = useLocalStore(
        (source) => ({
            organization: new OrganizationStore(source.organizationId, api),
            deviceList: new DeviceListStore(source.organizationId, api),
            roomList: new RoomListStore(source.organizationId, api),
        }),
        { organizationId },
    );

    useEffect(() => {
        const fetch = async () => {
            await store.organization.update();
            if(store.organization.usesRooms){
                await store.roomList.update();
            }
            await store.deviceList.update();
        }
        fetch().catch(console.error);
    }, []);

    const { search } = useLocation();
    useEffect(() => {
        const query = search ? serializeQueryParams(search) : {};
        store.deviceList.categoryFilter = query.category || null;
        store.deviceList.searchFilter = query.q || '';
    }, [search]);

    useEffect(() => {
        try {
            const xiva = new window.Ya.XIVA({
                api: `${appState.config.xiva.host}/v2/subscribe/websocket`,
                urlParams: {
                    service: appState.config.xiva.service,
                    topic: `${appState.config.xiva.topicPrefix}${organizationId}`,
                    client: 'frontend',
                    user: appState.user.uid,
                    session: Math.random().toString(36).substr(2),
                },
            });

            xiva.addEventListener('message', (event: Event) => {
                if (event instanceof CustomEvent) {
                    const detail = event.detail as xiva.EventDetail;
                    switch (detail.operation) {
                        case 'device-state':
                            if (detail.message) {
                                store.deviceList
                                    .updateDevice(detail.message)
                                    .catch((e) =>
                                        console.warn(
                                            `Failed to update device ${detail.message} on push`,
                                            e,
                                        ),
                                    );
                            }
                            break;
                        case 'room-state':
                            if (detail.message) {
                                store.roomList
                                    .updateRoom(detail.message)
                                    .catch((e) =>
                                        console.warn(
                                            `Failed to update room ${detail.message} on push`,
                                            e,
                                        ),
                                    );
                            }
                            break;
                        case 'organization-info':
                            store.organization
                                .update()
                                .catch((e) =>
                                    console.warn('Failed to update organization info on push', e),
                                );
                            break;
                    }
                }
            });

            return () => {
                xiva.destruct();
            };
        } catch (e) {
            console.warn('Failed to init Xiva', e);
        }
    }, [organizationId]);

    return (
        <Page>
            <Page.Head>
                <Page.HeadContent>
                    <Head
                        organization={store.organization}
                        deviceList={store.deviceList}
                        roomList={store.roomList}
                    />
                </Page.HeadContent>
            </Page.Head>
            <Page.Content>
                <Page.Page>
                    <Switch>
                        <Route
                            exact
                            path={routes.app.settings(':organizationId')}
                            component={SettingsPage}
                        />
                        <Route exact path={routes.app.deviceList(':organizationId')}>
                            {useObserver(() => {
                                if (!store.deviceList.loaded) {
                                    if (store.deviceList.loadError) {
                                        return (
                                            <Alert size='l' type='error'>
                                                {store.deviceList.loadError}
                                            </Alert>
                                        );
                                    } else {
                                        return (
                                            <div className={'page-center'}>
                                                <Loading size='s' />
                                            </div>
                                        );
                                    }
                                }

                                if (!store.deviceList.devices.size) {
                                    return (
                                        <Alert size='l' type='info'>
                                            Нет устройств
                                        </Alert>
                                    );
                                }

                                return (
                                    <DeviceList
                                        organization={store.organization}
                                        devices={store.deviceList}
                                        rooms={store.roomList}
                                    />
                                );
                            })}
                        </Route>
                        <Route
                            path={routes.app.history(':organizationId')}
                            component={HistoryPageComponent}
                        />
                        <Route exact path={routes.app.roomList(':organizationId')}>
                            {useObserver(() => {
                                if (!store.roomList.loaded) {
                                    if (store.roomList.loadError) {
                                        return (
                                            <Alert size='l' type='error'>
                                                {store.roomList.loadError}
                                            </Alert>
                                        );
                                    } else {
                                        return (
                                            <div className={'page-center'}>
                                                <Loading size='s' />
                                            </div>
                                        );
                                    }
                                }

                                if (!store.roomList.rooms.size) {
                                    return (
                                        <Alert size='l' type='info'>
                                            Нет комнат
                                        </Alert>
                                    );
                                }

                                return (
                                    <Suspense fallback={
                                        <div className={'page-center'}>
                                            <Loading size='s' />
                                        </div>
                                    }>
                                        <RoomList
                                            organization={store.organization}
                                            rooms={store.roomList}
                                        />
                                    </Suspense>
                                );
                            })}
                        </Route>
                        <Redirect to={routes.app.deviceList(organizationId)} />
                    </Switch>
                </Page.Page>
            </Page.Content>
        </Page>
    );
};

interface HeadProps {
    organization: OrganizationStore;
    deviceList: DeviceListStore;
    roomList: RoomListStore;
}

const Head = observer(({ organization, deviceList, roomList }: HeadProps) => {
    const routes = useRoutes();
    const api = useConsoleApi();
    const appState = useAppStateContext();
    const { showModal } = useModals();

    if (!organization.loaded) {
        return <Loading centered />;
    }

    const b = block('OrganizationPage-Head');
    const menuItems = [
        {
            path: routes.app.deviceList(organization.id),
            exact: true,
            title: 'Устройства',
        },
        {
            path: routes.app.history(organization.id),
            title: 'События',
            disabled: !appState.config.features.history,
        },
        {
            path: routes.app.settings(organization.id),
            title: 'Настройки',
            disabled: !appState.config.features.settings,
        },
    ];

    if (organization.usesRooms) {
        menuItems.splice(1, 0, {
            path: routes.app.roomList(organization.id),
            exact: true,
            title: 'Комнаты',
        });
    }

    const renderAddDeviceModal = (close: () => void) => (
        <DeviceCreation
            createDevice={deviceList.createDevice}
            onSkip={close}
            api={api}
            roomList={roomList}
            organization={organization}
        />
    );

    const renderAddRoomModel = (close: () => void) => (
        <RoomCreation createRoom={roomList.createRoom} onSkip={close} api={api} />
    );

    return (
        <div className={b()}>
            <div className={b('org-info')}>
                {organization.connectOrganization && (
                    <H1 className={b('connect')}>
                        {(appState.user.connect || '')
                            .split(',')
                            .map((x) => parseInt(x, 10))
                            .includes(organization.connectOrganization.id) ? (
                            <Link
                                external
                                to={routes.connect.portal(organization.connectOrganization.id)}
                                target='_blank'
                                title='Перейти в Коннект'
                            >
                                <Icon
                                    url={routes.assets('images/connect.svg')}
                                    cls={b('connect-icon')}
                                />
                                {organization.connectOrganization.name}
                            </Link>
                        ) : (
                            <>
                                <Icon
                                    url={routes.assets('images/connect.svg')}
                                    cls={b('connect-icon')}
                                />
                                {organization.connectOrganization.name}
                            </>
                        )}
                    </H1>
                )}
                <H1 className={b('name')}>{organization.name}</H1>
                <div className={b('promocode')}>
                    <div className={b('promocode-count')}>{organization.promoCount}</div>
                    <Icon url={routes.assets('images/promocode.svg')} cls={b('promocode-icon')} />
                </div>
                <div className={b('controls')}>
                    <Button
                        theme='clear'
                        size='s'
                        onClick={() => showModal({ component: renderAddDeviceModal })}
                    >
                        Добавить устройство
                    </Button>
                    {organization.usesRooms && (
                        <Button
                            theme='clear'
                            size='s'
                            onClick={() => showModal({ component: renderAddRoomModel })}
                        >
                            Добавить комнату
                        </Button>
                    )}
                </div>
            </div>
            <div className={b('nav')}>
                <Menu items={menuItems} />
            </div>
        </div>
    );
});

export default OrganizationPage;
