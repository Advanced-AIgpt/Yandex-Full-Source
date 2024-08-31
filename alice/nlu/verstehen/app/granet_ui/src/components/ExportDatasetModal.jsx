import React from 'react';

import Modal from 'antd/es/modal';
import Typography from 'antd/es/typography';

import connect from 'storeon/react/connect';

import { text2pars } from '../util';

const { Text } = Typography;

class ExportDatasetPreview extends React.Component {
    constructor(props) {
        super(props);
    }

    render() {
        return (
            <div>
                <Text strong> Preview:</Text>
                <br/>
                {text2pars(this.props.exportDataset.preview)}
            </div>
        );
    }
}

ExportDatasetPreview = connect('exportDataset', ExportDatasetPreview)

class ExportDatasetModal extends React.Component {
    constructor(props) {
        super(props);
        this.handleOk = this.handleOk.bind(this);
        this.handleCancel = this.handleCancel.bind(this);
    }

    handleOk = e => {
        this.props.dispatch('exportDataset/download');
    };

    handleCancel = e => {
        this.props.dispatch('exportDataset/update', { visible: false });
    };

    render() {
        const content = <ExportDatasetPreview />
        return (
            <Modal
                title="Export validated dataset"
                visible={this.props.exportDataset['visible']}
                okText='Download'
                onOk={this.handleOk}
                onCancel={this.handleCancel}
                cancelButtonProps={{ style: { visibility: "hidden" } }}
            >
                {content}
            </Modal>
        );
    }
}

export default connect('exportDataset', ExportDatasetModal);
