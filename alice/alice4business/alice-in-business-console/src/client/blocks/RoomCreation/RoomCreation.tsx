import block from 'bem-cn';
import { Select, Spin, TextInput } from 'lego-on-react';
import React, { FC, useEffect, useState } from 'react';
import { Field, Form, Header } from '../Form/Form';
import Submit from '../Settings/Submit';
import './RoomCreation.scss';
import { createErrorMessage } from '../../utils/alerts/error';
import { IConsoleApi } from '../../lib/console-api';
import { IRoomCreation } from '../../model/room';

interface Props {
    api: IConsoleApi;
    onSkip: () => void;
    createRoom: (form: IRoomCreation) => Promise<void>;
}

const b = block('RoomCreation');

const validate = {
    notEmpty: (val: any) => Boolean(val),
};

const defaultForm: IRoomCreation = {
    name: '',
    externalRoomId: '',
};

const RoomCreation: FC<Props> = ({ api, onSkip, createRoom }) => {
    const [form, setForm] = useState(defaultForm);
    const [isSubmitting, setIsSubmitting] = useState(false);
    const [errorMessage, setErrorMessage] = useState('');

    const setField = (key: keyof IRoomCreation) => (val: any) => {
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
            await createRoom(form);
            closeAndReset();
        } catch (error) {
            setErrorMessage(createErrorMessage(error, 'Что-то пошло не так'));
        } finally {
            setIsSubmitting(false);
        }
    };

    const isValidForm = () => {
        return Object.entries(form).every(
            ([key, val]) => !['name'].includes(key) || validate.notEmpty(val),
        );
    };

    const { name, externalRoomId } = form;

    return (
        <div className={b()}>
            <Form onSubmit={handleFormSubmit}>
                <Header>Добавление новой комнаты (группы устройств)</Header>
                <Field label='Название комнаты' required>
                    <TextInput
                        theme='normal'
                        size='s'
                        text={name}
                        onChange={setField('name')}
                        hasClear
                        placeholder='Название'
                    />
                </Field>
                <Field label='Идентификатор для API'>
                    <TextInput
                        theme='normal'
                        size='s'
                        text={externalRoomId}
                        hasClear
                        onChange={setField('externalRoomId')}
                        placeholder='Используется в выгрузке'
                    />
                </Field>
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

export default RoomCreation;
