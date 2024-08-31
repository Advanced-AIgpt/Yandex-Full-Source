import { AlertComponentPropsWithStyle, positions, AlertPosition, Provider as AlertProvider, transitions } from 'react-alert';
import React, { FC } from 'react';
import Alert from '../../blocks/Alert/Alert';

const AlertPopupOptions = {
    position: positions.BOTTOM_LEFT,
    timeout: 10000,
    offset: '0 20px',
    transition: transitions.FADE,
};

export const AlertWrap: FC<{timeout?: number, position?: AlertPosition}> = ({ children, timeout, position  }) => (
    <AlertProvider
        template={AlertPopup}
        {...AlertPopupOptions}
        timeout={timeout || AlertPopupOptions.timeout}
        position={position || AlertPopupOptions.position}
    >
        {children}
    </AlertProvider>
);

export const AlertPopup: FC<AlertComponentPropsWithStyle> = ({ message, options, close }) => {
    const { type } = options;
    const style = {
        margin: '-34px 10% 40px 10%',
        minWidth: '700px',
        zIndex: 1000,
        width: '105%',
    };
    return (
        <div style={style}>
            <Alert type={type || 'error'} close={close}>
                {message}
            </Alert>
        </div>
    );
};
