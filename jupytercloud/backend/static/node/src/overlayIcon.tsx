import React from "react";

import { Icon } from "react-bootstrap-icons";
import { OverlayTrigger, Tooltip, Spinner } from "react-bootstrap";

interface OverlayIconProps {
    title: string;
    children: React.ReactElement;
}

export const OverlayIcon: React.FC<OverlayIconProps> = (props) => {
    return (
        <OverlayTrigger
            placement="auto"
            overlay={(innerProps) => (
                <Tooltip id="icon-tooltip" key="icon-tooltip" {...innerProps}>
                    {props.title}
                </Tooltip>
            )}
        >
            {props.children}
        </OverlayTrigger>
    );
};

interface OverlayBootstrapIconProps {
    title: string;
    icon: Icon;
    onClick?: () => void;
}

export const OverlayBootstrapIcon: React.VFC<OverlayBootstrapIconProps> = (props) => (
    <OverlayIcon title={props.title}>
        {React.createElement(props.icon, {
            width: "1rem",
            height: "1rem",
            className: "action-icon",
            onClick: props.onClick,
        })}
    </OverlayIcon>
);

interface OverlaySpinnerProps {
    title: string;
}

export const OverlaySpinner: React.VFC<OverlaySpinnerProps> = (props) => (
    <OverlayIcon title={props.title}>
        <Spinner animation="border" role="status" size="sm" />
    </OverlayIcon>
);
