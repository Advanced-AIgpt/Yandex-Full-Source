import React from "react";

import StaffCardNative from "staff-card";

interface StaffCardProps {
    login: string;
}

export class StaffCard extends React.Component<StaffCardProps> {
    private nodeRef: React.RefObject<HTMLAnchorElement>;

    constructor(props: StaffCardProps) {
        super(props);
        this.nodeRef = React.createRef();
    }

    public componentDidMount = (): void => {
        new StaffCardNative(this.nodeRef.current, this.props.login);
    };

    public render = (): React.ReactNode => {
        return (
            <a
                href={"//staff.yandex-team.ru/" + this.props.login}
                target="_blank"
                ref={this.nodeRef}
                className="login-link"
                rel="noreferrer"
            >
                {this.props.login}
            </a>
        );
    };
}
