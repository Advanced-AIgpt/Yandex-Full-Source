import React, { Component } from 'react';
import ReactDOM from 'react-dom';

const POSITION = 'left';
const FORM_ID = '10016227';
const SCRIPT_SRC =
    'https://yandex.ru/global-notifications/butterfly/stable/butterfly.js';
const CONTAINTER_ID = 'yndxbux-script-container';

interface IYndxBugProps {
    dataData: string;
    dataText: string;
}

export default class YndxBug extends Component<IYndxBugProps> {
    componentDidMount(): void {
        const script = document.createElement('script');

        script.src = SCRIPT_SRC;
        script.id = 'yndxbug';
        script.crossOrigin = '';
        script.async = true;
        script.setAttribute('position', POSITION);
        script.setAttribute('modules', 'forms,info');
        script.setAttribute('form', FORM_ID);
        script.setAttribute('data-data', this.props.dataData);
        script.setAttribute('data-text', this.props.dataText);
        script.setAttribute('data-domain', 'yandex');
        script.setAttribute('screenshot', 'true');

        document.getElementById(CONTAINTER_ID).appendChild(script);
    }

    render() {
        return <div id={CONTAINTER_ID} />;
    }
}

export function addYndxBug(
    container: HTMLElement,
    dataData = '',
    dataText = ''
): void {
    const yndxBug = <YndxBug dataData={dataData} dataText={dataText} />;

    ReactDOM.render(yndxBug, container);
}
