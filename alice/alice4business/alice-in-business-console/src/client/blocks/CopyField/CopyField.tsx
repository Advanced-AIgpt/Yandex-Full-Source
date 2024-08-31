import React, { useRef, useState } from 'react';
import block from 'propmods';
import { Tooltip } from 'lego-on-react';

import './CopyField.scss';

interface Props {
    value: string;
    label: React.ReactNode;
    tooltipTitle?: string;
}

const b = block('CopyField');

const CopyField: React.FC<Props> = ({ label, value, tooltipTitle }) => {
    const valueRef = useRef<HTMLTextAreaElement>(null);
    const labelRef = useRef<HTMLDivElement>(null);

    const [tooltipVisible, setTooltipVisibility] = useState(false);

    const onClick = () => {
        valueRef.current!.select();
        document.execCommand('copy');
        setTooltipVisibility(true);
    };

    return (
        <div {...b()}>
            <textarea {...b('value')} ref={valueRef} value={value} readOnly />
            <span onClick={onClick} ref={labelRef} {...b('label')}>
                {label}
            </span>

            {tooltipVisible && (
                <Tooltip
                    visible={tooltipVisible}
                    anchor={labelRef.current!}
                    theme='normal'
                    autoclosable
                    onOutsideClick={() => setTooltipVisibility(false)}
                    to='right'
                    size='s'
                >
                    {tooltipTitle ? tooltipTitle : 'Скопировано'}
                </Tooltip>
            )}
        </div>
    );
};

export default CopyField;
