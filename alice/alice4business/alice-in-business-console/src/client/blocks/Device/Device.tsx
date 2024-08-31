import block from 'propmods';
import { observer } from 'mobx-react';
import React from 'react';
import { useAlert } from 'react-alert';
import { useRoutes } from '../../context/routes';
import { Status } from '../../model/device';
import { DeviceStore } from '../../store/device';
import { createErrorMessage } from '../../utils/alerts/error';
import { useModals } from '../../utils/modals/hooks';
import Row from '../Row/Row';
import StatusView from '../StatusView/StatusView';
import './Device.scss';
import EditableField from './EditableFeild';
import { OrganizationStore } from '../../store/organization';
import { DeviceListStore } from '../../store/device-list';
import { Button, Icon } from 'lego-on-react';
import Link from '../Link/Link';
import { useAppStateContext } from '../App/App';
import { RoomListStore } from '../../store/room-list';
import { SelectableField } from './SelectableField';
import { IDeviceRoom } from '../../model/room';

const b = block('Device');

interface Props {
    deviceStore: DeviceStore;
    deviceListStore: DeviceListStore;
    organizationStore: OrganizationStore;
    roomListStore: RoomListStore;

    even?: boolean;
}

const Device = observer(
    ({ deviceStore, deviceListStore, organizationStore, roomListStore, even }: Props) => {
        const routes = useRoutes();
        const appState = useAppStateContext();
        const { showConfirm } = useModals();
        const alert = useAlert();

        const saveNote = async (value: string) => await deviceStore.editDeviceNote(value);
        const saveExternalId = async (value: string) =>
            await deviceStore.editDeviceExternalId(value);
        const saveRoom = async (room: IDeviceRoom | null) => await deviceStore.editDeviceRoom(room);

        const deviceAction = async (handler: () => void | Promise<unknown>, errorMsg: string) => {
            try {
                await handler();
            } catch (error) {
                alert.error(createErrorMessage(error, errorMsg));
                console.error(error);
            }
        };

        const deleteDeviceHandler = () =>
            deviceAction(
                () => deviceListStore.removeDevice(deviceStore.id),
                'Не удалось удалить устройство',
            );

        const activatePromoHandler = () =>
            deviceAction(async () => {
                const res = await deviceStore.activatePromo();
                if (organizationStore.promoCount > 0) {
                    organizationStore.promoCount--;
                }
                return res;
            }, 'Не удалось активировать промокод');

        const activateDeviceHandler = () =>
            deviceAction(
                deviceStore.activateDevice,
                'Не удалось активировать устройство. Повторите попытку позже',
            );

        const resetDeviceHandler = () =>
            deviceAction(
                deviceStore.resetDevice,
                'Не удалось сбросить устройство. Повторите попытку позже',
            );

        const printHandler = async () => {
            const pdfWindow = window.open(routes.api(`devices/${deviceStore.id}/agreement`));
            pdfWindow!.print();
        };

        const roomNameMapper = (room: IDeviceRoom) => room?.name;
        const roomKeyMapper = (room: IDeviceRoom) => room?.id;

        const renderControls = () => {
            const ChangeStatusHandler =
                deviceStore.status === Status.Inactive
                    ? () =>
                          showConfirm({
                              text: 'Вы точно хотите активировать устройство?',
                              onConfirm: activateDeviceHandler,
                          })
                    : () =>
                          showConfirm({
                              text: 'Вы точно хотите сбросить устройство?',
                              onConfirm: resetDeviceHandler,
                          });
            const ChangeStatusMsg =
                deviceStore.status === Status.Inactive ? 'Активировать' : 'Сбросить';

            return (
                <div {...b('controls')}>
                    <Button
                        theme={deviceStore.status !== Status.Active ? 'action' : 'normal'}
                        size='s'
                        progress={deviceStore.pending}
                        disabled={deviceStore.pending}
                        onClick={ChangeStatusHandler}
                    >
                        {ChangeStatusMsg}
                    </Button>
                    <div {...b('sub-controls')}>
                        {appState.config.features.history && (
                            <Link
                                to={routes.app.deviceHistory(
                                    deviceStore.organizationId,
                                    deviceStore.id,
                                )}
                            >
                                <Button theme='clear' size='s'>
                                    <Icon
                                        url={routes.assets('images/history.svg')}
                                        style={{ backgroundSize: 14 }}
                                    />
                                    История
                                </Button>
                            </Link>
                        )}
                        <Button
                            theme='clear'
                            size='s'
                            disabled={
                                deviceStore.status === Status.Reset ||
                                deviceStore.pending ||
                                deviceStore.isActivatedByCustomer
                            }
                            onClick={printHandler}
                        >
                            <Icon
                                url={routes.assets('images/printer.svg')}
                                style={{ backgroundSize: 14 }}
                            />
                            Печать
                        </Button>
                        <Button
                            theme='clear'
                            size='s'
                            onClick={() =>
                                showConfirm({
                                    text: 'Вы точно хотите удалить устройство?',
                                    onConfirm: deleteDeviceHandler,
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
                <Row
                    deviceId={<span {...b('device-id')}> {deviceStore.deviceId}</span>}
                    showRoom={organizationStore.usesRooms}
                    externalDeviceId={
                        <EditableField
                            title={'Редактирование идентификатора'}
                            value={deviceStore.externalDeviceId || ''}
                            saveChanges={saveExternalId}
                            maxLength={40}
                        />
                    }
                    note={
                        <EditableField
                            title={'Редактирование пометок'}
                            value={deviceStore.note || ''}
                            saveChanges={saveNote}
                            maxLength={40}
                        />
                    }
                    room={
                        <SelectableField
                            title={'Редактирование комнаты'}
                            value={deviceStore.room || null}
                            nameMapper={roomNameMapper}
                            keyMapper={roomKeyMapper}
                            saveChanges={saveRoom}
                            choices={roomListStore.deviceRooms}
                        />
                    }
                    status={
                        <StatusView
                            activatePromoHandler={() =>
                                deviceStore.agreementAccepted
                                    ? showConfirm({
                                          text: 'Вы хотите потратить промокод?',
                                          onConfirm: activatePromoHandler,
                                      })
                                    : showConfirm({
                                          text:
                                              'Для активации промо-кода необходимо распечатать и подписать Соглашение',
                                          confirmText: 'Печать',
                                          onConfirm: printHandler,
                                          rejectText: 'Отмена',
                                      })
                            }
                            hasPromo={deviceStore.hasPromo}
                            online={deviceStore.online}
                            status={deviceStore.status}
                            pending={deviceStore.pending}
                            isActivatedByCustomer={deviceStore.isActivatedByCustomer}
                        />
                    }
                    controls={renderControls()}
                />
            </div>
        );
    },
);

export default Device;
