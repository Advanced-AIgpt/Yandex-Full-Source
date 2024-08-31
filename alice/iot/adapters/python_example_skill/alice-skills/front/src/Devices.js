import React from 'react';
import { Device } from './Device';
import './Devices.css';

export const Devices = ({ devices = [] }) => {
    if (!devices || !devices.length) {
        return (
            <div className="Devices Devices_empty">
                <h1>Нет устройств</h1>
            </div>
        );
    }

    return (
        <div className="Devices">
            <h1>Список устройств</h1>
            {devices.map(info => (
                <Device
                    key={info.id}
                    info={info}
                />
            ))}
        </div>
    );
};
