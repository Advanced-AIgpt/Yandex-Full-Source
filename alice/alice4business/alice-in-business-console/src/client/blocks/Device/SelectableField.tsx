import block from 'bem-cn';
import { Button, Select, TextInput, Tooltip } from 'lego-on-react';
import { isArray } from 'lodash';
import React, { FC, ReactNode, useRef, useState } from 'react';
import { useRoutes } from '../../context/routes';
import { truncate } from '../../lib/utils';
import { IDeviceRoom } from '../../model/room';
import { useModals } from '../../utils/modals/hooks';
import Loading from '../Loading/Loading';
import Submit from '../Settings/Submit';
import './EditableFeild.scss';

const b = block('EditableField');

interface Props {
    value: any;
    choices: any[];
    title: string;
    saveChanges: (value: any) => void;
    nameMapper: (v: any) => string;
    keyMapper: (v: any) => string;
}

interface ModalProps extends Props {
    close: () => void;
}

const Modal: FC<ModalProps> = ({
    value,
    choices,
    nameMapper,
    keyMapper,
    saveChanges,
    title,
    close,
}) => {
    const [current, setCurrent] = useState(value);
    const [isProgress, setIsProgress] = useState(false);
    const [isError, setIsError] = useState(false);
    const [errorMsg, setErrorMsg] = useState(null as ReactNode);

    const onSave = async () => {
        setIsProgress(true);
        try {
            await saveChanges(current);
            close();
        } catch (error) {
            console.log(error);
            setErrorMsg('Что-то пошло не так');
            setIsError(true);
        } finally {
            setIsProgress(false);
        }
    };

    const cancel = () => {
        setCurrent(value);
        close();
    };

    const onSubmit = (e: any) => {
        e.preventDefault();
        void onSave();
    };

    const onSelect = (val: string | string[]) => {
        if (isArray(val)) {
            val = val[0];
        }

        const newVal = val ? choices.find((c) => keyMapper(c) === val) : null;
        setCurrent(newVal);
    };

    return (
        <form onSubmit={onSubmit} className={b('modal')}>
            {title && <div className={b('title')}>{title}</div>}
            <Select
                theme='normal'
                size='s'
                type='radio'
                onChange={onSelect}
                val={current ? keyMapper(current) : ''}
            >
                <Select.Item val=''>-- Без комнаты --</Select.Item>
                {choices.map((choice) => (
                    <Select.Item val={keyMapper(choice)}>{nameMapper(choice)}</Select.Item>
                ))}
            </Select>
            <div className={b('error', { show: isError })}>{errorMsg}</div>
            <div className={b('controls')}>
                <Submit disabled={isProgress || isError} size='s' text='Сохранить' />
                <Button size='s' disabled={isProgress} theme='normal' onClick={cancel}>
                    Отмена
                </Button>
                {isProgress && <Loading size='xxs' />}
            </div>
        </form>
    );
};

export const SelectableField: FC<Props> = (props) => {
    const routes = useRoutes();
    const [tooltipVisible, setTooltipVisible] = useState(false);
    const valueRef = useRef(null);
    const { showModal } = useModals();
    const { value, nameMapper } = props;

    return (
        <div className={b()}>
            <div ref={valueRef} onClick={() => setTooltipVisible(true)} className={b('value')}>
                {value ? truncate(nameMapper(value) || '', 15) : ''}
            </div>
            {tooltipVisible && (
                <Tooltip
                    visible={tooltipVisible}
                    theme='normal'
                    autoclosable
                    onOutsideClick={() => setTooltipVisible(false)}
                    to='right'
                    size='xs'
                    anchor={valueRef.current!}
                >
                    {value ? nameMapper(value) : ''}
                </Tooltip>
            )}
            <button
                style={{ backgroundImage: `url(${routes.assets('images/edit.svg')}` }}
                onClick={() =>
                    showModal({
                        key: 'EDITABLE',
                        component: (close) => <Modal {...props} close={close} value={value} />,
                    })
                }
                className={b('button')}
            />
        </div>
    );
};
