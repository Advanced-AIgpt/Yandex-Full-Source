import React, { FunctionComponent } from 'react';
import { Row, Card } from 'react-bootstrap';

interface INirvanaPanelProps {
    title: string;
    bodyWrap?: boolean;
    border?: string;
}

export const NirvanaPanel: FunctionComponent<INirvanaPanelProps> = props => {
    return (
        <Row className="align-items-center jcne-card-box">
            <Card className="jcne-card" border={props.border}>
                <Card.Header>{props.title}</Card.Header>
                {props.bodyWrap ? (
                    <Card.Body className="jcne-card-body">
                        {props.children}
                    </Card.Body>
                ) : (
                    props.children
                )}
            </Card>
        </Row>
    );
};

NirvanaPanel.defaultProps = {
    border: null,
    bodyWrap: true
};
