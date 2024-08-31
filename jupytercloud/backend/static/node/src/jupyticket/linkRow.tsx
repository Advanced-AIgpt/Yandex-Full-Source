import React from "react";

import { ListGroup, Row, Col } from "react-bootstrap";
import { DateString } from "../dateString";
import { StaffCard } from "../staffCard";

interface ILinkRow {
    id: string;
    login?: string;
    loginPlaceholder: string;
    dateTime?: string | number;
    href?: string;
    blink?: boolean;
}

export const LinkRow: React.FC<ILinkRow> = (props) => (
    <ListGroup.Item key={props.id} className={props.blink && "blink"} action href={props.href}>
        <Row>
            {props.children}
            <Col xs="auto">
                {/* XXX: React thinks that nested <a> is bad
                    (<ListGroup.Item action href> is <a> and <StaffCard /> is <a>) */}
                {props.login ? <StaffCard login={props.login} /> : props.loginPlaceholder}
            </Col>
            <Col xs="auto">
                {props.dateTime && <DateString dateTime={props.dateTime} tooltipPrefix="Updated" id={props.id} />}
            </Col>
        </Row>
    </ListGroup.Item>
);
