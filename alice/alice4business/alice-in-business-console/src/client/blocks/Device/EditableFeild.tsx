import block from 'bem-cn';
import { Button, Select, TextInput, Tooltip } from 'lego-on-react';
import React, { FC, ReactNode, useRef, useState } from 'react';
import API from '../../lib/api';
import { useRoutes } from '../../context/routes';
import { truncate } from '../../lib/utils';
import Loading from '../Loading/Loading';
import Submit from '../Settings/Submit';
import './EditableFeild.scss';
import { useModals } from '../../utils/modals/hooks';

const b = block('EditableField');

interface Props {
    value: string;
    title: string;
    saveChanges: (value: string) => void;
    maxLength: number;
}

interface ModalProps extends Props {
    close: () => void;
}

const Modal: FC<ModalProps> = ({ value, saveChanges, maxLength = Infinity, title, close }) => {
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
            if (API.isUniqueConstrainError(error)) {
                setErrorMsg('Значение должно быть уникальным');
            } else {
                setErrorMsg('Что-то пошло не так');
            }
            setIsError(true);
        } finally {
            setIsProgress(false);
        }
    };

    const cancel = () => {
        setCurrent(value);
        close();
    };
    const onChange = (val: string) => {
        isError && setIsError(false);
        maxLength >= val.length && setCurrent(val);
    };

    const onSubmit = (e: any) => {
        e.preventDefault();
        void onSave();
    };
    return (
        <form onSubmit={onSubmit} className={b('modal')}>
            {title && <div className={b('title')}>{title}</div>}
            <TextInput size='s' theme='normal' onChange={onChange} text={current} hasClear />
            <div className={b('error', { show: isError })}>{errorMsg}</div>
            <div className={b('controls')}>
                <Submit
                    disabled={isProgress || isError || !Boolean(current)}
                    size='s'
                    text='Сохранить'
                />
                <Button size='s' disabled={isProgress} theme='normal' onClick={cancel}>
                    Отмена
                </Button>
                {isProgress && <Loading size='xxs' />}
            </div>
        </form>
    );
};

const EditableField: FC<Props> = (props) => {
    const routes = useRoutes();
    const [tooltipVisible, setTooltipVisible] = useState(false);
    const valueRef = useRef(null);
    const { showModal } = useModals();
    const { value } = props;

    return (
        <div className={b()}>
            <div ref={valueRef} onClick={() => setTooltipVisible(true)} className={b('value')}>
                {truncate(props.value, 15)}
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
                    {value}
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

export default EditableField;
