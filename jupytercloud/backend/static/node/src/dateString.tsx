import React from "react";
import moment from "moment-timezone";
import { OverlayTrigger, Tooltip } from "react-bootstrap";

interface DateStringProps {
    dateTime: string | number;
    tooltipPrefix?: string;
    id: string;
}

export const DateString: React.VFC<DateStringProps> = (props) => {
    // until we have all things in English, especially tooltipPrefix, we have to keep en locale
    const dateTime = moment.utc(props.dateTime).locale("en");
    const fromNow = dateTime.fromNow();

    // this format choosed to have ~~equal width in different lines
    // due to having auto width in outer cols
    const str = dateTime.tz("Europe/Moscow").format("DD MMM HH:mm");

    return (
        <OverlayTrigger
            placement="auto"
            overlay={
                <Tooltip id={`tooltip-${props.id}`}>
                    {props.tooltipPrefix ? `${props.tooltipPrefix} ${fromNow}` : fromNow}
                </Tooltip>
            }
        >
            {/* NB: for some strange reason React wants an key here */}
            <span className="date-string" key={`date-${props.id}`}>
                {str}
            </span>
        </OverlayTrigger>
    );
};
