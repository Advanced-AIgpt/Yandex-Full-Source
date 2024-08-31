import React from 'react';
import './Capability.css';

const getShortTypeName = type => type.replace(/^devices\.capabilities\./, '');

const TypeName = ({ capability }) => {
    return capability.parameters
        && capability.parameters.name
        || [getShortTypeName(capability.type), capability.parameters && capability.parameters.instance].filter(Boolean).join('/');
};

const Unit = ({ capability }) => {
    switch (capability.parameters && capability.parameters.unit) {
        case 'unit.percent':
            return '%';
        case 'unit.temperature.celsius':
            return '°C';
        default:
            return null;
    }
};

const OnOffCapability = ({ capability }) => {
    const { state } = capability;
    const getValue = () => {
        if (!state) {
            return 'Не известно, включено ли';
        }

        return state.value ? 'Включено' : 'Выключено';
    };

    return (
        <div className={`Capability Capability_type_on-off ${state && state.value === true ? 'Capability_value_on' : ''} ${state && state.value === false ? 'Capability_value_off' : ''}`}>
            <div className="Capability-type">
                <TypeName capability={capability} />
            </div>
            <div className="Capability-value">
                {getValue()}
            </div>
        </div>
    );
};

const RangeCapability = ({ capability }) => {
    const { state } = capability;
    const getValue = () => {
        if (!state || typeof state.value === 'undefined') {
            return '--';
        }

        return state.value;
    };

    return (
        <div className="Capability Capability_type_range">
            <div className="Capability-type">
                <TypeName capability={capability} />
            </div>
            <div className="Capability-value">
                {getValue()}
                <Unit capability={capability} />
            </div>
        </div>
    );
};

const ColorSettingCapability = ({ capability }) => {
    const { state } = capability;
    const getValue = () => {
        if (!state || typeof state.value === 'undefined') {
            return '--';
        }

        if (state.instance === 'temperature_k') {
            return `${state.value}K`;
        }

        if (state.instance === 'hsv') {
            return `h:${state.value.h},s:${state.value.s},v:${state.value.v}`;
        }

        if (state.instance === 'rgb') {
            return `rgb:${state.value}`;
        }

        return state.value;
    };

    return (
        <div className="Capability Capability_type_color">
            <div className="Capability-type">
                <TypeName capability={capability} />
            </div>
            <div className="Capability-value">
                {getValue()}
            </div>
        </div>
    );
};

const ModeCapability = ({ capability }) => {
    const { state } = capability;
    const getValue = () => {
        if (!state || typeof state.value === 'undefined') {
            return '--';
        }

        if (state.instance === 'fan_speed') {
            return `fan_speed:${state.value}`;
        }

        if (state.instance === 'swing') {
            return `swing:${state.value}`;
        }

        if (state.instance === 'thermostat') {
            return `thermostat:${state.value}`;
        }

        return state.value;
    };

    return (
        <div className="Capability Capability_type_mode">
            <div className="Capability-type">
                <TypeName capability={capability} />
            </div>
            <div className="Capability-value">
                {getValue()}
            </div>
        </div>
    );
};

const ToggleCapability = ({ capability }) => {
    const { state } = capability;
    const getValue = () => {
        if (!state || typeof state.value === 'undefined') {
            return 'Не известно, включено ли';
        }

        return state.value ? 'Включено' : 'Выключено';
    };

    return (
        <div className="Capability Capability_type_toggle">
            <div className="Capability-type">
                <TypeName capability={capability} />
            </div>
            <div className="Capability-value">
                {getValue()}
            </div>
        </div>
    );
};

export const Capability = ({ capability }) => {
    switch (capability.type) {
        case 'devices.capabilities.on_off':
            return <OnOffCapability capability={capability} />;
        case 'devices.capabilities.color_setting':
            return <ColorSettingCapability capability={capability} />;
        case 'devices.capabilities.range':
            return <RangeCapability capability={capability} />;
        case 'devices.capabilities.mode':
            return <ModeCapability capability={capability} />;
        case 'devices.capabilities.toggle':
            return <ToggleCapability capability={capability} />;
        default:
            return null;
    }
};
