import './RoomRow.scss';
import React, { FC, ReactNode } from 'react';
import block from 'bem-cn';

const b = block('RoomRow');

interface Props {
    name: string | ReactNode;
    externalRoomId: string | ReactNode;
    status: string | ReactNode;
    numDevices: string | ReactNode;
    controls: string | ReactNode;
}

const RoomRow: FC<Props> = ({ name, externalRoomId, status, numDevices, controls }) => {
    return (
        <div className={b()}>
            <div className={b('name')}>{name}</div>
            <div className={b('externalRoomId')}>{externalRoomId}</div>
            <div className={b('status')}>{status}</div>
            <div className={b('numDevices')}>{numDevices}</div>
            <div className={b('controls')}>{controls}</div>
        </div>
    );
};

export default RoomRow;
