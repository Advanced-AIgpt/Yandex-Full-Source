import React from "react";
import { RouteComponentProps } from "react-router-dom";

// importing from 'react-bootstrap', not from 'react-bootstrap/<Element>'
// due to including external react-bootstrap and how webpack works
import { Container, Row, Col, Card } from "react-bootstrap";

import { StaffCard } from "../staffCard";
import { DateString } from "../dateString";

import { JupyTicket as JupyTicketStruct } from "./structs";
import { PropertyRow } from "./propertyRow";
import { EditablePropertyRow } from "./editableProperyRow";
import { ArcadiaLinks } from "./arcadiaLinks";
import { NirvanaLinks } from "./nirvanaLinks";
import { StartrekLinks } from "./startrekLinks";

import { requestHub } from "../utils";
import { CardHeader } from "../cardHeader";

import JupyterCloudIcon from "../../assets/jupyterCloudIcon.svg";

type JupyTicketMatch = {
    jupyTicketId: string;
};

type JupyTicketProps = RouteComponentProps<JupyTicketMatch>;

type JupyTicketState = JupyTicketStruct;

export class JupyTicketPage extends React.Component<JupyTicketProps, JupyTicketState> {
    constructor(props: JupyTicketProps) {
        super(props);

        this.state = {
            id: parseInt(this.props.match.params.jupyTicketId, 10),
            user_name: null,
            created: 0,
            updated: 0,
            title: "%title%",
            description: "%description%",
            arcadia: [],
            nirvana: [],
            startrek: [],
        };
    }

    public componentDidMount = (): void => {
        const jcdata = window.jcdata;
        // first case - page loaded with server-rendered jcdata
        // second case - we navigated on other jupyticket wo page reloading and
        // server rendered jcdata stores wrong jupyticket info
        if (jcdata.jupyticket && jcdata.jupyticket.id === this.state.id) {
            this.setState(window.jcdata.jupyticket);
        } else {
            throw Error("unsopported yet");
        }
    };

    public onTitleChange = (value: string): Promise<void> => {
        // TODO: Add lock on component to ensure all data callbacks
        // are run in call order

        return requestHub({
            uri: `/api/jupyticket/${this.state.id}`,
            method: "POST",
            json: {
                title: value,
            },
        }).then((data) => {
            this.setState(data.json.jupyticket);
        });
    };

    public render = (): React.ReactNode => {
        const { user_name, created, updated, title, description } = this.state;

        return (
            <Container>
                <Row lg="2" md="1" sm="1" xs="1">
                    <Col>
                        <Row className="pb-2 g-0">
                            <Card className="h-100">
                                <CardHeader title="JupyTicket info" iconClass={JupyterCloudIcon} />
                                <Card.Body>
                                    <EditablePropertyRow
                                        title="Title"
                                        value={title}
                                        onChange={this.onTitleChange.bind(this)}
                                    />
                                    <PropertyRow title="Owner">
                                        {user_name && <StaffCard login={user_name} />}
                                    </PropertyRow>
                                    <PropertyRow title="Created">
                                        <DateString dateTime={created * 1000} id="jupyticket-created" />
                                    </PropertyRow>
                                    <PropertyRow title="Updated">
                                        <DateString dateTime={updated * 1000} id="jupyticket-updated" />
                                    </PropertyRow>
                                </Card.Body>
                            </Card>
                        </Row>
                        <Row className="pb-2 g-0">
                            <Card className="h-100">
                                <CardHeader title="Description" iconClass={JupyterCloudIcon} />
                                <Card.Body>
                                    {/* TODO: maybe markdown later */}
                                    <Card.Text className="preserve-line-break">{description}</Card.Text>
                                </Card.Body>
                            </Card>
                        </Row>
                    </Col>
                    <Col>
                        <Row className="pb-2 g-0">
                            <StartrekLinks data={this.state.startrek} />
                        </Row>
                        <Row className="pb-2 g-0">
                            <ArcadiaLinks data={this.state.arcadia} />
                        </Row>
                        <Row className="pb-2 g-0">
                            <NirvanaLinks data={this.state.nirvana} />
                        </Row>
                    </Col>
                </Row>
            </Container>
        );
    };
}
