import block from 'bem-cn';
import { Select, Spin, TextInput } from 'lego-on-react';
import React, { FC, useEffect, useState } from 'react';
import {
    IDeviceCreation,
    IMyDevice,
    Platform,
    StationPlatform,
    TvPlatform,
} from '../../model/device';
import { Field, Form, Header } from '../Form/Form';
import Submit from '../Settings/Submit';
import './DeviceCreation.scss';
import { createErrorMessage } from '../../utils/alerts/error';
import API from '../../lib/api';
import { IConsoleApi } from '../../lib/console-api';
import { IRoom } from '../../model/room';
import { RoomListStore } from '../../store/room-list';
import { OrganizationStore } from '../../store/organization';

interface Props {
    api: IConsoleApi;
    onSkip: () => void;
    createDevice: (form: IDeviceCreation) => Promise<void>;
    roomList: RoomListStore;
    organization: OrganizationStore;
}

const b = block('DeviceCreation');

const validate = {
    notEmpty: (val: any) => Boolean(val),
};

const defaultForm: IDeviceCreation = {
    platform: StationPlatform.YandexStation,
    deviceId: '',
    externalDeviceId: '',
    note: '',
    roomId: '',
};

const DeviceCreation: FC<Props> = ({ api, onSkip, createDevice, roomList, organization }) => {
    const [form, setForm] = useState(defaultForm);
    const [myDevices, setMyDevices] = useState(Array<IMyDevice>());
    const [isLoading, setIsLoading] = useState(false);
    const [isSubmitting, setIsSubmitting] = useState(false);
    const [errorMessage, setErrorMessage] = useState('');

    useEffect(() => {
        setIsLoading(true);
        api.getMyDevices()
            .then(setMyDevices)
            .finally(() => {
                setIsLoading(false);
            })
            .catch(console.log);
    }, []);

    const setField = (key: keyof IDeviceCreation) => (val: any) => {
        val = Array.isArray(val) ? val[0] : val;
        if (typeof val === 'string') {
            if (val.length > 40) {
                return;
            }
        }
        setErrorMessage('');
        setForm((prev) => ({ ...prev, [key]: val }));
    };

    const closeAndReset = () => {
        onSkip();
        setForm(defaultForm);
    };
    const handleFormSubmit = async (event: any) => {
        event.preventDefault();
        setIsSubmitting(true);
        try {
            await createDevice(form);
            closeAndReset();
        } catch (error) {
            if (API.isError(error) && error.payload && error.payload.unique && error.fields) {
                const fields = new Set(Object.keys(error.fields));
                if (fields.has('deviceId') && fields.has('platform') && fields.size === 2) {
                    setErrorMessage('Устройство с этим ID уже зарегистрировано в системе');
                } else if (
                    fields.has('organizationId') &&
                    fields.has('externalDeviceId') &&
                    fields.size === 2
                ) {
                    setErrorMessage('Этот идентификатор для API уже используется в организации');
                } else {
                    setErrorMessage(createErrorMessage(error, 'Что-то пошло не так'));
                }
            } else {
                if (error.fields) {
                    delete error.fields.organizationId;
                }

                setErrorMessage(createErrorMessage(error, 'Что-то пошло не так'));
            }
        } finally {
            setIsSubmitting(false);
        }
    };

    const isValidForm = () => {
        return Object.entries(form).every(
            ([key, val]) => !['deviceId', 'platform'].includes(key) || validate.notEmpty(val),
        );
    };

    const { platform, deviceId, externalDeviceId, note, roomId } = form;

    const myDeviceOnChange = (val: string | string[]) => {
        const id = Array.isArray(val) ? val[0] : val;
        const device = myDevices.find((item) => item.deviceId === id);
        if (!device) {
            return;
        }

        setField('deviceId')(device.deviceId);
        setField('platform')(device.platform);
        setField('note')(device.note || '');
    };

    const platformOnChange = (val: string | string[]) =>
        setField('platform')(Array.isArray(val) ? val[0] : val);

    return (
        <div className={b()}>
            <Form onSubmit={handleFormSubmit}>
                <Header>Добавление нового устройства</Header>
                {(isLoading || myDevices.length > 0) && (
                    <Field label=' '>
                        {isLoading ? (
                            <Spin size='s' progress />
                        ) : (
                            <Select
                                theme='normal'
                                placeholder='Мои устройства'
                                size='s'
                                type='radio'
                                onChange={myDeviceOnChange}
                            >
                                {myDevices.map((device, idx) => (
                                    <Select.Item val={device.deviceId} key={idx}>
                                        {device.deviceId}: {device.note}
                                    </Select.Item>
                                ))}
                            </Select>
                        )}
                    </Field>
                )}
                <Field label='ID устройства' required>
                    <TextInput
                        theme='normal'
                        size='s'
                        text={deviceId}
                        onChange={setField('deviceId')}
                        hasClear
                        placeholder='UUID / Device ID'
                    />
                </Field>
                <Field label='Тип устройства'>
                    <Select
                        theme='normal'
                        placeholder='Тип устройства'
                        size='s'
                        type='radio'
                        val={platform}
                        onChange={platformOnChange}
                    >
                        <Select.Group title='Станции'>
                            <Select.Item val={StationPlatform.YandexStation}>
                                Яндекс.Станция
                            </Select.Item>
                            <Select.Item val={StationPlatform.YandexMini}>
                                Яндекс.Станция Мини
                            </Select.Item>
                            <Select.Item val={StationPlatform.YandexMini2}>
                                Яндекс.Станция Мини 2
                            </Select.Item>
                            <Select.Item val={StationPlatform.YandexStation2}>
                                Яндекс.Станция Макс
                            </Select.Item>
                            <Select.Item val={StationPlatform.YandexLight}>
                                Яндекс.Станция Лайт
                            </Select.Item>
                            <Select.Item val={StationPlatform.YandexMidi}>
                                Яндекс.Станция 2
                            </Select.Item>
                        </Select.Group>
                        {organization.usesRooms && (
                            <Select.Group title='ТВ'>
                                <Select.Item val={TvPlatform.CV9632}>CV 9632</Select.Item>
                                <Select.Item val={TvPlatform.CVTE351}>CVTE 351</Select.Item>
                                <Select.Item val={TvPlatform.RTK2861}>RTK 2861</Select.Item>
                                <Select.Item val={TvPlatform.RTK2871}>RTK 2871</Select.Item>
                                <Select.Item val={TvPlatform.Module}>Яндекс.Модуль</Select.Item>
                            </Select.Group>
                        )}
                    </Select>
                </Field>
                <Field label='Идентификатор для API'>
                    <TextInput
                        theme='normal'
                        size='s'
                        text={externalDeviceId}
                        hasClear
                        onChange={setField('externalDeviceId')}
                        placeholder='Используется в выгрузке'
                    />
                </Field>
                <Field label='Пометка'>
                    <TextInput
                        theme='normal'
                        size='s'
                        text={note}
                        hasClear
                        onChange={setField('note')}
                        placeholder='Где расположено устройство?'
                    />
                </Field>
                {organization.usesRooms && (
                    <Field label='Комната'>
                        <Select
                            theme='normal'
                            size='s'
                            type='radio'
                            val={roomId}
                            onChange={setField('roomId')}
                        >
                            {roomList.visibleList.map((room, idx) => (
                                <Select.Item val={room.id}>{room.name}</Select.Item>
                            ))}
                        </Select>
                    </Field>
                )}
                <Submit disabled={!isValidForm()} size='s' text='Сохранить' />
                {errorMessage && (
                    <span style={{ marginTop: 10, color: 'rgb(158, 0, 0)' }}>{errorMessage}</span>
                )}
            </Form>

            {isSubmitting && (
                <div className={b('progress')}>
                    <Spin size='s' progress />
                </div>
            )}
        </div>
    );
};

export default DeviceCreation;
