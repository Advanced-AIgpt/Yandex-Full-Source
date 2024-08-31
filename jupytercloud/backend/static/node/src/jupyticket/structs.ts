export interface JHData {
    base_url: string;
    prefix: string;
    user: string;
    admin_access: boolean;
    options_form: boolean;
}

export interface JupyTicketStartrek {
    startrek_id: string;
    created: number;
}

export interface JupyTicketNirvana {
    workflow_id: string;
    instance_id: string;
    created: number;
}

export interface JupyTicketArcadia {
    user_name: string;
    path: string;
    revision: string;
    link: string;
    shared: number;
    message?: string;
}

export interface JupyTicket {
    id: number;
    user_name: string;
    created: number;
    updated: number;
    title: string;
    description?: string;
    arcadia: JupyTicketArcadia[];
    nirvana: JupyTicketNirvana[];
    startrek: JupyTicketStartrek[];
}

export interface JCData {
    jupyticket?: JupyTicket;
    startrek_front: string;
    arcanum_front: string;
}

declare global {
    interface Window {
        jhdata: JHData;
        jcdata?: JCData;
    }
}
