import React from 'react';
import connect from 'storeon/react/connect';

import Button from 'antd/es/button';
import Col from 'antd/es/col';
import Input from 'antd/es/input';
import InputNumber from 'antd/es/input-number';
import Table from 'antd/es/table';
import Menu from 'antd/es/menu';
import Slider from 'antd/es/slider';
import Switch from 'antd/es/switch';
import Popover from 'antd/es/popover';

import { text2pars } from '../util';

const { Search } = Input;

class Query extends React.Component {
    handleClick(nonterminal, interval) {
        this.props.dispatch('synonyms/search', { interval: interval, nonterminal: nonterminal });
    }

    render() {
        const tokens = this.props.synonyms.query.map(elem => {
            if (!elem[0]) {
                return elem[1];
            }
            return (
                <Popover content={<div>{text2pars(elem[0])}</div>}>
                    <Button
                        type='link'
                        onClick={(e) => this.handleClick(elem[0], elem[2])}
                        size='small'
                    >
                        {elem[1]}
                    </Button>
                </Popover>
            );
        })
        return (
            <div>
                {tokens}
            </div>
        );
    }
}

Query = connect('synonyms', Query)

class SynonymsTab extends React.Component {
    onSearch = (value) => {
        this.props.dispatch('synonyms/search', { text: value, interval: undefined });
        this.props.dispatch('synonyms/expandGrammar');
    }
    onAddToGrammar = (text) => {
        this.props.dispatch('synonyms/expandGrammar', text);
    }
    onSwitchSettings = (key) => {
        this.props.dispatch('synonyms/update', { [key]: !this.props.synonyms[key] });
    }
    onChangeSettings = (key, value) => {
        this.props.dispatch('synonyms/update', { [key]: value });
    }

    render() {
        const columns = [
            {
                title: <Query />,
                dataIndex: 'result',
                render: (text, record) => {
                    return <Button 
                        type='link' 
                        size='small' 
                        onClick={() => this.onAddToGrammar(record.result)}
                    >
                        { record.result }
                    </Button>
                }
            },
            {
                title: 'Score',
                dataIndex: 'score',
                width: '7em',
                render: (text, record, idx) => { return parseFloat(record.score).toExponential(2)}
            }
        ]
        const data = this.props.synonyms.results.map((el, idx) => {
            return {
                key: idx,
                result: el[0],
                score: el[1]
            }
        })
        const searchArea = <Search
            placeholder="input search text"
            onSearch={this.onSearch}
            style={{ width: "100%" }}
        />;

        return (
            <div>
                <Col span={13}>
                    <div>
                        <Table
                            size="small"
                            title={() => searchArea }
                            style={{ width: "100%" }}
                            columns={columns}
                            dataSource={data}
                            scroll={{ y: "calc(100vh - 41px - 49px - 45px)" }}
                            pagination={false}
                        />
                    </div>
                </Col>
                <Col span={11}>
                    <Menu selectable={false}>
                        <Menu.Item key="grammarSynonyms">
                            <Switch
                                checked={this.props.synonyms['grammarSynonyms']}
                                onChange={() => this.onSwitchSettings("grammarSynonyms")}
                                size="small"
                                style={{ marginRight: 12 }}
                            />
                            Fetch synonyms from grammar
                        </Menu.Item>
                        <Menu.Item key="nMasks">
                            <InputNumber
                                min={1}
                                max={3}
                                value={this.props.synonyms['nMasks']}
                                onChange={(val) => { this.onChangeSettings('nMasks', val) }}
                                style={{ marginRight: 12 }}
                            />
                            Number of words per token
                        </Menu.Item>
                        <Menu.Item key="nSamples">
                            <InputNumber
                                min={1}
                                max={100}
                                value={this.props.synonyms['nSamples']}
                                onChange={(val) => { this.onChangeSettings('nSamples', val) }}
                                style={{ marginRight: 12 }}
                            />
                            Max number of variants per word
                        </Menu.Item>
                        <Menu.Item key="noise">
                            <Col span={6}>
                                <Slider
                                    min={0}
                                    max={0.1}
                                    step={0.01}
                                    value={this.props.synonyms['noise']}
                                    onChange={(val) => { this.onChangeSettings('noise', val) }}
                                    style={{ marginRight: 12 }}
                                />
                            </Col>
                            <Col span={6}>
                                <InputNumber
                                    min={0}
                                    max={0.1}
                                    step={0.01}
                                    value={this.props.synonyms['noise']}
                                    onChange={(val) => { this.onChangeSettings('noise', val) }}
                                    style={{ marginRight: 12 }}
                                />
                            </Col>
                            <Col span={4}>
                                Use noise
                            </Col>
                        </Menu.Item>
                    </Menu>
                </Col>
            </div>
        );
    };
}

export default connect('synonyms', SynonymsTab);
