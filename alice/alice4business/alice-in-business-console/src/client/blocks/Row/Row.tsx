import './Row.scss';
import React, { FC, ReactNode } from 'react';
import block from 'bem-cn';

const b = block('Row');

interface Props {
    deviceId: string | ReactNode;
    externalDeviceId: string | ReactNode;
    note: string | ReactNode;
    room: string | ReactNode;
    status: string | ReactNode;
    controls: string | ReactNode;
    showRoom: boolean;
}

const Row: FC<Props> = ({ deviceId, externalDeviceId, note, room, status, controls, showRoom }) => {
    return (
        <div className={b()}>
            <div className={b('deviceId')}>{deviceId}</div>
            <div className={b('externalDeviceId')}>{externalDeviceId}</div>
            <div className={b('note')}>{note}</div>
            {showRoom && <div className={b('room')}>{room}</div>}
            <div className={b('status')}>{status}</div>
            <div className={b('controls')}>{controls}</div>
        </div>
    );
};

export default Row;
