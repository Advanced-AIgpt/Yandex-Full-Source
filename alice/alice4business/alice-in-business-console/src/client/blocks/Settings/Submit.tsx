import block from 'propmods';
import React from 'react';

import { Button, ButtonSize, Spin } from 'lego-on-react';

import './Submit.scss';

interface SubmitProps {
    text: string;
    message?: string;
    severity?: 'info' | 'error';
    inProgress?: boolean;
    disabled?: boolean;
    size?: ButtonSize;
}

const bs = block('Submit');

export default function Submit(props: SubmitProps) {
    const { text, message = '', severity = 'info', inProgress = false, disabled = false, size = 'l' } = props;
    return (
        <div {...bs()}>
            <Button theme='action' size={size} type='submit' disabled={disabled || inProgress}>
                {text}
            </Button>
            {inProgress && <Spin size='s' progress />}
            {message && <span {...bs('message', { severity })}>{message}</span>}
        </div>
    );
}
