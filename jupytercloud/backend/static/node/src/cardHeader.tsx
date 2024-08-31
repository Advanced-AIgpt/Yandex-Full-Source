import React from "react";

import { Card, Col, Row } from "react-bootstrap";

interface ICardHeader {
    title: string;
    iconClass: React.VFC<React.SVGProps<SVGSVGElement>>;
}

export const CardHeader: React.VFC<ICardHeader> = (props) => {
    const icon = React.createElement(props.iconClass, { width: "1em", height: "1em", className: "img-fluid h5 my-0" });
    return (
        <Card.Header className="container-fluid">
            <Row>
                <Col xs="auto">{icon}</Col>
                <Col as="h5" className="my-0">
                    {props.title}
                </Col>
            </Row>
        </Card.Header>
    );
};
