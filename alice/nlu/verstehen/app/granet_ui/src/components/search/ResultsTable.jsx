import React from 'react';
import Button from 'antd/es/button';
import Table from 'antd/es/table';
import Typography from 'antd/es/typography';

import connect from 'storeon/react/connect';

import Header from './Header';

const { Text } = Typography;
const ButtonGroup = Button.Group;

class ResultsTable extends React.Component {
    constructor(props) {
        super(props);
        // This binding is necessary to make `this` work in the callback
        this.addUserPos = this.addUserPos.bind(this);
        this.addUserNeg = this.addUserNeg.bind(this);
        this.onSelectChange = this.onSelectChange.bind(this);
    }
    getText = (idx) => {
        if (this.props[this.props.searchResultCol]['texts']) {
            return this.props[this.props.searchResultCol].texts[idx];
        }
        return this.props[this.props.searchResultCol].results[idx];
    }

    onSelectChange(selectedRowKeys) {
        this.props.dispatch(this.props.searchResultCol + '/selected/update', selectedRowKeys);
    }

    addUserPos = (appIdx) => {
        let idx;
        for (let i = 0; i < this.props[this.props.searchResultCol].appIds.length; i++) {
            if (this.props[this.props.searchResultCol].appIds[i] === appIdx) {
                idx = i;
            }
        }
        const sample = { text: this.getText(idx), appIdx: this.props[this.props.searchResultCol].appIds[idx] };
        this.props.dispatch('userSamples/add', {
            key: 'positives',
            value: sample
        });
        this.props.dispatch('addButtons/remove', { col: this.props.searchResultCol, idx: idx });
        this.props.dispatch(this.props.searchResultCol + '/removeEntry', this.props[this.props.searchResultCol].appIds[idx]);
        this.props.dispatch(this.props.searchResultCol + '/selected/remove', appIdx);
    }

    addUserNeg = (appIdx) => {
        let idx;
        for (let i = 0; i < this.props[this.props.searchResultCol].appIds.length; i++) {
            if (this.props[this.props.searchResultCol].appIds[i] === appIdx) {
                idx = i;
            }
        }
        const sample = { text: this.getText(idx), appIdx: this.props[this.props.searchResultCol].appIds[idx] };
        this.props.dispatch('userSamples/add', {
            key: 'negatives',
            value: sample
        });
        this.props.dispatch('addButtons/remove', { col: this.props.searchResultCol, idx: idx });
        this.props.dispatch(this.props.searchResultCol + '/removeEntry', this.props[this.props.searchResultCol].appIds[idx]);
        this.props.dispatch(this.props.searchResultCol + '/selected/remove', appIdx);
    }

    componentDidMount = () => {
        this.props.dispatch('addButtons/reset');
    }

    render() {
        const selectedRowKeys = this.props[this.props.searchResultCol].selected;
        const rowSelection = {
            selectedRowKeys,
            onChange: this.onSelectChange,
            hideDefaultSelections: true,
            selections: [
                {
                    key: 'addToPos',
                    text: 'Add selected to positives',
                    onSelect: () => {
                        selectedRowKeys.forEach(appIdx => this.addUserPos(appIdx));
                    }
                },
                {
                    key: 'addToNeg',
                    text: 'Add selected to negatives',
                    onSelect: () => {
                        selectedRowKeys.forEach(appIdx => this.addUserNeg(appIdx));
                    }
                }
            ]
        };

        let columns = [];

        if (this.props.settings['showAddButtons']) {
            columns.push({
                dataIndex: 'buttons',
                width: '74px'
            });
        }
        columns.push({
            title: <Header
                searchIdx={this.props.searchIdx}
                time={this.props[this.props.searchResultCol]['time']} />,
            dataIndex: 'sample'
        })
        let data;
        try {
            if (this.props.settings['showOccurance']) {
                columns.push({
                    title: "Occ.",
                    dataIndex: 'occ',
                    width: '7em',
                    render: (text, record, idx) => { return <Text type="secondary">{parseFloat(record.occ * 100).toFixed(2)}%</Text> },
                    defaultSortOrder: 'descend',
                    sorter: (a, b) => { return a.occ - b.occ; }
                });
            }

            if (this.props.settings['showClfScore']) {
                columns.push({
                    title: "Score",
                    dataIndex: 'score',
                    width: '7em',
                    render: (text, record, idx) => { return <Text type="secondary">{parseFloat(record.score).toFixed(2)}</Text> },
                    sorter: (a, b) => { return a.score - b.score; }
                });
            }

            const getData = (searchCol, nSamples = this.props.settings['linesToShow']) => {
                
                const showAlmostMatchedSample = (item, idx) => {
                    if (this.props[searchCol]['almostMatchedMarkup'] &&
                        this.props[searchCol]['almostMatchedMarkup'].length > 0 &&
                        !this.props[searchCol]['almostMatchedInProgress']) {
                        return this.props[searchCol].almostMatchedMarkup[idx].map((nested) => {
                            if (nested[1] === 1) {
                                return <Text strong>{nested[0]}</Text>;
                            }
                            return <Text>{nested[0]}</Text>
                        })
                    }
                    return item;
                };

                return this.props[searchCol].results.slice(0, nSamples).map((item, idx) => {
                    const buttonPlusStyle = this.props.addButtons[searchCol][idx] === 'pos' ? { backgroundColor: '#52c41a50' } : {}
                    const buttonMinusStyle = this.props.addButtons[searchCol][idx] === 'neg' ? { backgroundColor: '#f5222d50' } : {}

                    return {
                        buttons: <ButtonGroup size="small">
                            <Button
                                icon="plus"
                                disabled={this.props.addButtons[searchCol][idx] !== undefined}
                                onClick={() => { this.addUserPos(this.props[searchCol].appIds[idx]) }}
                                shape="circle" size="small"
                                style={buttonPlusStyle} />
                            <Button
                                icon="minus"
                                disabled={this.props.addButtons[searchCol][idx] !== undefined}
                                onClick={() => { this.addUserNeg(this.props[searchCol].appIds[idx]) }}
                                shape="circle" size="small"
                                style={buttonMinusStyle} />
                        </ButtonGroup>,
                        sample: <div>{showAlmostMatchedSample(item, idx)}</div>,
                        occ: this.props[searchCol]['occuranceRate'][idx],
                        score: this.props[searchCol]['clfScore'][idx],
                        idx: idx,
                        key: this.props[searchCol].appIds[idx]
                    }
                });
            }

            data = getData(this.props.searchResultCol);

        } catch (e) {
            if (e instanceof TypeError) {
                console.error(e);
                console.warn('Clearing localStorage');
                localStorage.clear();
            }
        }
        return (
            <div>
                <Table
                    rowSelection={rowSelection}
                    columns={columns}
                    dataSource={data}
                    scroll={{ y: "calc(100vh - 65px - 45px)" }}
                    pagination={false}
                    tableLayout={undefined}
                    rowKey={record => record.key}
                />
            </div>
        );
    }
}

export default connect('searchResultsGranet', 'searchResultsVerstehen', 'settings', 'addButtons', ResultsTable);