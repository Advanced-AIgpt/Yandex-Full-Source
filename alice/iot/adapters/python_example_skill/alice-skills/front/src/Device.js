import React from 'react';
import { Capabilities } from './Capabilities';
import './Device.css';

export const Device = ({ info: { name, id, type, capabilities } }) => {
    return (
        <div className="Device">
            <h2>{name}</h2>
            <h3>Тип: {type}</h3>
            <h3>ID: {id}</h3>
            <Capabilities
                capabilities={capabilities}
            />
        </div>
    );
};
