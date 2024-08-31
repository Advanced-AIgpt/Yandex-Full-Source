import urljoin from "url-join";

import React from "react";
import { Col } from "react-bootstrap";

import { JupyTicketArcadia } from "./structs";
import { ListCard } from "./listCard";
import { LinkRow } from "./linkRow";

import ArcanumIcon from "../../assets/arcanumIcon.svg";

interface ArcadiaLinksProps {
    data: JupyTicketArcadia[];
}

const renderArcadiaLink = (data: JupyTicketArcadia) => {
    const key = `${data.path}:${data.revision}:${data.shared}`;
    const path = data.path + "@" + (data.revision ? data.revision : "");
    const href = urljoin(window.jcdata.arcanum_front, data.path, "?rev=" + data.revision);

    // TODO: also we want to have overlay with commit message later
    return (
        <LinkRow
            key={key}
            id={key}
            dateTime={data.shared * 1000}
            login={data.user_name}
            loginPlaceholder="Unknown user"
            href={href}
        >
            <Col className="text-truncate external-link" title={path}>
                {path}
            </Col>
        </LinkRow>
    );
};

export const ArcadiaLinks: React.VFC<ArcadiaLinksProps> = (props) => {
    return (
        <ListCard<JupyTicketArcadia>
            title="Arcanum"
            iconClass={ArcanumIcon}
            data={props.data}
            renderElement={renderArcadiaLink}
        />
    );
};
