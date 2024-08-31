import React from 'react';
import { Capability } from './Capability';

export const Capabilities = ({ capabilities = [] }) => {
    if (!capabilities || !capabilities.length) {
        return null;
    }

    return (
        <div className="Capabilities">
            <h3>Состояние:</h3>
            {capabilities.map(capability => (
                <Capability
                    key={capability.type}
                    capability={capability}
                />
            ))}
        </div>
    );
};
