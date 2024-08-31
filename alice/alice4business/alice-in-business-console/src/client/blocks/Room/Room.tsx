import block from 'propmods';

import React from 'react';
import { observer } from 'mobx-react';
import { RoomStore } from '../../store/room';
import { OrganizationStore } from '../../store/organization';
import { RoomListStore } from '../../store/room-list';
import { Button, Icon } from 'lego-on-react';
import Link from '../Link/Link';
import RoomRow from '../RoomRow/RoomRow';
import './Room.scss';
import { Status } from '../../model/device';
import { OnlineStatus, PromoStatus } from '../../model/room';
import { useModals } from '../../utils/modals/hooks';
import { createErrorMessage } from '../../utils/alerts/error';
import { useAlert } from 'react-alert';
import EditableField from '../Device/EditableFeild';
import StatusView from '../StatusView/StatusView';
import { useRoutes } from '../../context/routes';
import { serializeQueryParams } from '../../lib/utils';

const b = block('Room');

interface Props {
    roomStore: RoomStore;
    roomListStore: RoomListStore;
    organizationStore: OrganizationStore;

    even?: boolean;
}

const Room = observer(({ roomStore, roomListStore, organizationStore, even }: Props) => {
    const { showConfirm } = useModals();
    const alert = useAlert();
    const routes = useRoutes();

    const roomAction = async (handler: () => void | Promise<unknown>, errorMsg: string) => {
        try {
            await handler();
        } catch (error) {
            alert.error(createErrorMessage(error, errorMsg));
            console.error(error);
        }
    };

    const activateRoomHandler = () =>
        roomAction(
            () => roomStore.activateRoom(),
            'Не удалось активировать устройства комнаты. Повторите попытку позже',
        );

    const resetRoomHandler = () =>
        roomAction(
            () => roomStore.resetRoom(),
            'Не удалось сбросить устройства комнаты. Повторите попытку позже',
        );

    const roomRoute =
        routes.app.deviceList(organizationStore.id) +
        '?' +
        serializeQueryParams({ q: roomStore.id });

    const saveRoomNameHandler = async (value: string) => await roomStore.renameRoom(value);
    const saveExternalRoomIdHandler = async (value: string) =>
        await roomStore.editExternalRoomId(value);
    const activatePromoHandler = () =>
        showConfirm({
            text: 'Вы точно хотите активировать промокод?',
            onConfirm: () =>
                roomAction(
                    async () => roomStore.activatePromo(),
                    'Не удалось активировать промокод',
                ),
        });

    const deleteRoomHandler = async () =>
        roomAction(() => roomListStore.removeRoom(roomStore.id), 'Не удалось удалить комнату');

    const renderControls = () => {
        const ChangeStatusHandler =
            roomStore.status === Status.Inactive
                ? () =>
                      showConfirm({
                          text: 'Вы точно хотите активировать все устройства комнаты?',
                          onConfirm: activateRoomHandler,
                      })
                : () =>
                      showConfirm({
                          text: 'Вы точно хотите сбросить все устройства комнаты?',
                          onConfirm: resetRoomHandler,
                      });
        const ChangeStatusMsg = roomStore.status === Status.Inactive ? 'Активировать' : 'Сбросить';

        return (
            <div {...b('controls')}>
                <Button
                    theme={roomStore.status !== Status.Active ? 'action' : 'normal'}
                    size='s'
                    progress={roomStore.pending}
                    disabled={roomStore.pending || roomStore.numDevices === 0}
                    onClick={ChangeStatusHandler}
                >
                    {ChangeStatusMsg}
                </Button>
                <div {...b('sub-controls')}>
                    <Button
                        theme='clear'
                        size='s'
                        onClick={() =>
                            showConfirm({
                                text: 'Вы точно хотите удалить комнату?',
                                onConfirm: deleteRoomHandler,
                            })
                        }
                    >
                        <Icon
                            url={routes.assets('images/delete.svg')}
                            style={{ backgroundSize: 14 }}
                        />
                        Удалить
                    </Button>
                </div>
            </div>
        );
    };

    return (
        <div {...b({ even })}>
            <RoomRow
                name={
                    <EditableField
                        title={'Редактирование названия'}
                        value={roomStore.name || ''}
                        saveChanges={saveRoomNameHandler}
                        maxLength={40}
                    />
                }
                externalRoomId={
                    <EditableField
                        title={'Редактирование идентификатора'}
                        value={roomStore.externalRoomId || ''}
                        saveChanges={saveExternalRoomIdHandler}
                        maxLength={40}
                    />
                }
                status={
                    <StatusView
                        activatePromoHandler={activatePromoHandler}
                        hasPromo={roomStore.promoStatus === PromoStatus.Applied}
                        status={roomStore.status}
                        online={roomStore.onlineStatus === OnlineStatus.Online}
                        pending={roomStore.pending}
                        isActivatedByCustomer={roomStore.promoStatus === PromoStatus.NotAvailable}
                    />
                }
                numDevices={
                    <Link to={roomRoute}>
                        <Button theme='clear' size='s'>
                            {roomStore.numDevices}
                        </Button>
                    </Link>
                }
                controls={renderControls()}
            />
        </div>
    );
});

export default Room;
