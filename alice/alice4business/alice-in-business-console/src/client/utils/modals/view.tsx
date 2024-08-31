import React, { FC, ReactNode } from 'react';
import { Button, Modal as LegoModal } from 'lego-on-react';
import block from 'bem-cn';
import './style.scss';
import { ModalProps as ModalPropsDefault } from './service';

export interface ConfirmProps {
    text: ReactNode;
    confirmText?: string;
    onConfirm: () => void;
    rejectText?: string;
    onReject?: () => void;
}

export const Confirm: FC<ConfirmProps> = ({ text, confirmText, onConfirm, rejectText, onReject }) => {
    const b = block('confirm');
    return (
        <div className={b()}>
            <div className={b('title')}>{text}</div>
            <div className={b('controls')}>
                <Button theme='action' size='m' onClick={onConfirm}>
                    {confirmText || 'Да'}
                </Button>
                <Button theme='normal' size='m' onClick={onReject}>
                    {rejectText || 'Нет'}
                </Button>
            </div>
        </div>
    );
};

interface ModalProps extends ModalPropsDefault {
    closeFn: () => void;
}

export const Modal: FC<ModalProps> = ({ component, visible, closeFn, closeOnOutsideClick, zIndex }) => {
    const b = block('Modal');
    return (
        <LegoModal
            zIndexGroupLevel={zIndex}
            autoclosable
            visible={visible}
            directions={['bottom-left']}
            onOutsideClick={closeOnOutsideClick ? closeFn : undefined}
        >
            <div className={b('content')}>{component}</div>
        </LegoModal>
    );
};
