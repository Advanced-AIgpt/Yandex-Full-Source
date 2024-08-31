import block from 'bem-cn';
import React, { FC } from 'react';
import './Alert.scss';

const b1 = block('Alert');

interface AlertProps {
    type: 'info' | 'success' | 'error';
    close?: () => void;
    size?: 's' | 'm' | 'l';
}

export const Alert: FC<AlertProps> = ({ children, type = 'error', close, size = 'm' }) => {
    return (
        <div className={b1({ type, size })}>
            <span className={b1('icon')} />
            <span style={{ flex: 2 }} className={b1('message')}>
                {children}
            </span>
            {close && (
                <button onClick={close} className={b1('close-btn')}>
                    <span className={b1('close-icon')}>×</span>
                </button>
            )}
        </div>
    );
};

export default Alert;
