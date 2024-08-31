import React from "react";

import { Card, ListGroup, Collapse, Button } from "react-bootstrap";

import { CardHeader } from "../cardHeader";

const VISIBLE_ITEMS = 5;

interface ListCardProps<T> {
    title: string;
    iconClass: React.VFC<React.SVGProps<SVGSVGElement>>;
    data: T[];
    renderElement: (data: T) => React.ReactElement;
}

export const ListCard: <T>(props: ListCardProps<T>) => React.ReactElement = (props) => {
    const [open, setOpen] = React.useState(false);

    const visible = props.data.slice(0, VISIBLE_ITEMS);
    const hidden = props.data.slice(VISIBLE_ITEMS);

    return (
        <Card>
            <CardHeader title={props.title} iconClass={props.iconClass} />
            <ListGroup variant="flush">
                {!visible.length && <ListGroup.Item key="no">No entries yet</ListGroup.Item>}
                {visible.map(props.renderElement)}
            </ListGroup>
            {hidden.length ? (
                <>
                    <Collapse in={open} className="border-0">
                        <ListGroup variant="flush">
                            <>{hidden.map(props.renderElement)}</>
                        </ListGroup>
                    </Collapse>
                    <Card.Footer>
                        <Button variant="primary" onClick={() => setOpen(!open)}>
                            {open ? "Show less" : `Show ${hidden.length} more`}
                        </Button>
                    </Card.Footer>
                </>
            ) : null}
        </Card>
    );
};
