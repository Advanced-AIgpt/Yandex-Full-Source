import React from 'react';

export function getDisplayName<T>(Component: React.ComponentType<T>) {
    return (
        Component.displayName || Component.name || (Component.constructor && Component.constructor.name) || 'Component'
    );
}
