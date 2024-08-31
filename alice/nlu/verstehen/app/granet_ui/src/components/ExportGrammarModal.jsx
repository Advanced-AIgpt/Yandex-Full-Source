import React from 'react';

import Modal from 'antd/es/modal';
import Typography from 'antd/es/typography';

import connect from 'storeon/react/connect';

const { Text } = Typography;
const { Paragraph } = Typography;

class ExportGrammarModal extends React.Component {
    constructor(props) {
        super(props);
        this.handleOk = this.handleOk.bind(this);
        this.handleCancel = this.handleCancel.bind(this);
    }

    handleOk = e => {
        this.props.dispatch('export/visible/update', false);
    };

    handleCancel = e => {
        this.props.dispatch('export/visible/update', false);
    };

    render() {
        const content = (this.props.exportGrammar['requestFailed']) ? 
            <Text type="danger">{this.props.exportGrammar['grammarAsExperiment']}</Text>
            : 
            (<Paragraph copyable ellipsis={{ rows: 3, expandable: true }}>
                {this.props.exportGrammar['grammarAsExperiment']}
            </Paragraph>);
        return (
            <Modal
              title="Print grammar as experiment"
              visible={this.props.exportGrammar['visible']}
              onOk={this.handleOk}
              onCancel={this.handleCancel}
              cancelButtonProps={{style: {visibility: "hidden"}}}
            >
                {content}
            </Modal>
        );
    }
}

export default connect('exportGrammar', ExportGrammarModal);
