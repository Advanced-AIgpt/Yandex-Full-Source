import React from "react";

import { Row, Col } from "react-bootstrap";

interface PropertyRowProps {
    title: string;
    children?: React.ReactNode;
    statusIcon?: React.ReactElement;
    onClick?: () => void;
}

export const PropertyRow: React.FC<PropertyRowProps> = (props) => {
    return (
        <Row onClick={props.onClick}>
            <Col
                xs={props.children ? "3" : null}
                md={props.children ? "2" : null}
                lg={props.children ? "3" : null}
                xl={props.children ? "2" : null}
                className="fw-bolder"
            >
                {props.title}
            </Col>
            {props.children && <Col>{props.children}</Col>}
            {props.statusIcon && (
                <Col xs="auto" className="align-items-center">
                    {props.statusIcon}
                </Col>
            )}
        </Row>
    );
};
