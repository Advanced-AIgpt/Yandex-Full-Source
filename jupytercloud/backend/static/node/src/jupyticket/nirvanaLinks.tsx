import React from "react";
import { Col } from "react-bootstrap";

import { JupyTicketNirvana } from "./structs";
import { ListCard } from "./listCard";
import { LinkRow } from "./linkRow";

import { requestHub } from "../utils";

import NirvanaIcon from "../../assets/nirvanaIcon.svg";

interface INirvanaInstanceInfo {
    workflowId: string;
    instanceId: string;
    name?: string;
    owner?: string;
    updated?: string;
    description?: string;
    status?: string;
    url?: string;
    blink?: boolean;
}

interface NirvanaLinksProps {
    data: JupyTicketNirvana[];
}

const renderNirvanaLink = (data: INirvanaInstanceInfo) => {
    const key = `${data.workflowId}/${data.instanceId}`;
    const name = data.name || key;
    const description = name + (data.description ? "\n\n" + data.description : "");

    return (
        <LinkRow
            key={key}
            id={key}
            dateTime={data.updated}
            login={data.owner}
            loginPlaceholder="No owner"
            href={data.url}
            blink={data.blink}
        >
            {/* TODO: add styled badges to status like in Nirvana itself */}
            <Col xs="auto" className="text-truncate" title={data.status}>
                {data.status}
            </Col>
            <Col className="text-truncate external-link" title={description}>
                {name}
            </Col>
        </LinkRow>
    );
};

export const NirvanaLinks: React.FC<NirvanaLinksProps> = (props) => {
    const [links, setLinks] = React.useState<INirvanaInstanceInfo[]>([]);

    React.useEffect(() => {
        const raw: INirvanaInstanceInfo[] = props.data.map(
            (data: JupyTicketNirvana): INirvanaInstanceInfo => ({
                workflowId: data.workflow_id,
                instanceId: data.instance_id,
                blink: true,
            }),
        );

        setLinks(raw);

        if (!raw.length) {
            return;
        }

        props.data.forEach((element: JupyTicketNirvana, index: number) => {
            // TODO: handle promise reject
            // TODO: make a single point with request
            requestHub({
                uri: "/api/nirvana/workflow_info",
                method: "GET",
                queryArgs: [["workflow_url", `${element.workflow_id}/${element.instance_id}`]],
            }).then((data) => {
                setLinks((prev: INirvanaInstanceInfo[]) => {
                    const d: Record<string, any> = data.json;
                    // NB: using prev without copy breaking everithing to hell
                    const newLinks: INirvanaInstanceInfo[] = [...prev];

                    newLinks[index] = {
                        instanceId: d.instance_id,
                        workflowId: d.workflow_id,
                        name: d.name,
                        description: d.description,
                        updated: d.updated,
                        url: d.url,
                        owner: d.owner,
                        status: d.status,
                    };

                    return newLinks;
                });
            });
        });
    }, [props.data]);

    return (
        <ListCard<INirvanaInstanceInfo>
            title="Nirvana"
            iconClass={NirvanaIcon}
            data={links}
            renderElement={renderNirvanaLink}
        />
    );
};
