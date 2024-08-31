import urljoin from "url-join";

import React from "react";
import { Col } from "react-bootstrap";

import { JupyTicketStartrek } from "./structs";
import { ListCard } from "./listCard";
import { LinkRow } from "./linkRow";

import { requestHub } from "../utils";
import StartrekIcon from "../../assets/trackerIcon.svg";

const MAX_DESCRIPTION = 300;

interface ITicketInfo {
    key: string;
    updated?: string;
    title?: string;
    description?: string;
    assignee?: string;
    blink?: boolean;
}

interface StartrekLinksProps {
    data: JupyTicketStartrek[];
}

const renderStartrekLink = (data: ITicketInfo) => {
    const href = urljoin(window.jcdata.startrek_front, data.key);

    // Unicode-aware string slicing
    const rawDescription = data.description && [...data.description];
    let description = rawDescription && rawDescription.slice(0, MAX_DESCRIPTION).join("");

    if (rawDescription && rawDescription.length > MAX_DESCRIPTION) {
        description += "<...>";
    }

    return (
        <LinkRow
            key={data.key}
            id={data.key}
            dateTime={data.updated}
            login={data.assignee}
            loginPlaceholder="Not assigned"
            href={href}
            blink={data.blink}
        >
            <Col xs="3" className="text-truncate" title={data.key}>
                {data.key}
            </Col>
            <Col className="text-truncate external-link" title={description}>
                {data.title}
            </Col>
        </LinkRow>
    );
};

export const StartrekLinks: React.FC<StartrekLinksProps> = (props) => {
    const [tickets, setTickets] = React.useState<ITicketInfo[]>([]);

    React.useEffect(() => {
        const raw: ITicketInfo[] = props.data.map(
            (data: JupyTicketStartrek): ITicketInfo => ({
                key: data.startrek_id,
                blink: true,
            }),
        );

        setTickets(raw);

        if (!raw.length) {
            return;
        }

        // TODO: handle promise reject
        requestHub({
            uri: "/api/statrek/get_tickets_info",
            method: "GET",
            queryArgs: raw.map((t) => ["ticket_id", t.key]),
        }).then((data) => {
            const newTickets: ITicketInfo[] = Object.keys(data.json.tickets).map((key: string) => {
                const ticketInfo: any = data.json.tickets[key];
                const r: ITicketInfo = { key };

                if (ticketInfo) {
                    r.updated = ticketInfo.updatedAt;
                    r.title = ticketInfo.summary;
                    r.description = ticketInfo.description;
                    if (ticketInfo.assignee) {
                        r.assignee = ticketInfo.assignee.id;
                    }
                }

                return r;
            });

            setTickets(newTickets);
        });
    }, [props.data]);

    return (
        <ListCard<ITicketInfo>
            title="Startrek"
            iconClass={StartrekIcon}
            data={tickets}
            renderElement={renderStartrekLink}
        />
    );
};
