import React from 'react';

import Modal from 'antd/es/modal';

import connect from 'storeon/react/connect';

import infoText from '../resources/info';

class InfoModal extends React.Component {
    constructor(props) {
        super(props);
        this.handleOk = this.handleOk.bind(this);
        this.handleCancel = this.handleCancel.bind(this);
    }

    handleOk = e => {
        this.props.dispatch('info/visible', false);
    };

    handleCancel = e => {
        this.props.dispatch('info/visible', false);
    };

    render() {
        const content = infoText;
        return (
            <Modal
              title="How-to"
              visible={this.props.info['visible']}
              onOk={this.handleOk}
              onCancel={this.handleCancel}
              cancelButtonProps={{style: {visibility: "hidden"}}}
            >
                {content}
            </Modal>
        );
    }
}

export default connect('info', InfoModal);
