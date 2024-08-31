import React from 'react';

import Col from 'antd/es/col';
import Table from 'antd/es/table';
import Typography from 'antd/es/typography';
import Icon from 'antd/es/icon';
import Input from 'antd/es/input';
import Button from 'antd/es/button';
import Popover from 'antd/es/popover';
import Dropdown from 'antd/es/dropdown';
import Menu from 'antd/es/menu';
import message from 'antd/es/message';

import connect from 'storeon/react/connect';

import { text2pars, generate_uuidv4 } from '../util';

const { Search } = Input;
const { Text } = Typography;

class Header extends React.Component {
    constructor(props) {
        super(props);

        // This binding is necessary to make `this` work in the callback
        this.onAdd = this.onAdd.bind(this);
    }

    onAdd(value) {
        this.props.dispatch('userSamples/add', { key: this.props.type, value: { text: value } });
    }

    render() {
        return (
            <Search
                addonBefore={this.props.type}
                placeholder={"Add " + this.props.type}
                enterButton="Add"
                style={{ width: "90%", margin: "0 5%", textAlign: "center" }}
                onChange={this.onChange}
                onSearch={this.onAdd}
            />
        );
    }
}

Header = connect('userSamples', Header)


class UserSamples extends React.Component {
    constructor(props) {
        super(props);
        // This binding is necessary to make `this` work in the callback
        this.onDeleteClick = this.onDeleteClick.bind(this);
        this.onItemClick = this.onItemClick.bind(this);
        this.onSelectChange = this.onSelectChange.bind(this);
    }

    onItemClick(e, item) {
        if (e.key == 'delete') {
            this.onDeleteClick(item.uuid)
        }
        else if (e.key == 'synonyms') {
            if (item.isPositive) {
                this.props.dispatch('appTab/update', 'synonyms');
                this.props.dispatch('synonyms/search', { text: item.text });
            } else {
                message.error('Sample cannot be matched');
            }
        }
    }

    onDeleteClick(uuid) {
        this.props.dispatch('userSamples/' + this.props.type + '/delete', uuid);
    }

    onSelectChange(selectedRowKeys) {
        this.props.dispatch('userSamples/selected/update', { key: this.props.type, value: selectedRowKeys });
    }

    render() {
        const showText = (item) => {
            const content = (this.props.settings.showMarkup) ? item.markup : item.text;
            if (item.isPositive) {
                return <Text style={{ color: "#52c41a" }}>
                    {content}
                </Text>
            }
            return <Text type="danger">
                {content}
            </Text>
        }

        const rowMenu = (item) => {
            return (
                <Menu onClick={(e) => this.onItemClick(e, item)} selectable={false}>
                    <Menu.Item key='delete'>
                        Delete item
                    </Menu.Item>
                    <Menu.Item key='synonyms'>
                        Search for synonyms
                    </Menu.Item>
                </Menu>
            )
        };

        const selectedRowKeys = this.props.userSamples[this.props.type + 'Selected'];
        const rowSelection = {
            selectedRowKeys,
            onChange: this.onSelectChange,
            hideDefaultSelections: true,
            selections: [
                {
                    key: 'remove',
                    text: 'Remove selected',
                    onSelect: () => {
                        selectedRowKeys.forEach(uuid => this.onDeleteClick(uuid));
                    }
                }
            ]
        };

        const columns = [
            {
                title: <Header type={this.props.type} />,
                dataIndex: 'sample',
                defaultSortOrder: (this.props.type == 'positives') ? 'ascend' : 'descend',
                sorter: (a, b) => { return a.isPositive - b.isPositive }
            }
        ]


        let data = this.props.userSamples[this.props.type];
        data = data.map((item, idx) => {
            return {
                sample:
                    <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                        <Popover 
                            content={<div>{text2pars(item.debugInfo)}</div>}
                            trigger="click"
                        >
                            {showText(item)}
                        </Popover>
                        <Dropdown overlay={rowMenu(item)}>
                            <Button type="link" shape="circle" icon="more" style={{ float: "right" }} />
                        </Dropdown>
                    </div>,
                isPositive: item.isPositive,
                text: item.text,
                debugInfo: text2pars(item.debugInfo),
                uuid: item.uuid,
                key: item.uuid
            }
        });

        return (
            <div>
                <Table
                    rowSelection={rowSelection}
                    columns={columns}
                    dataSource={data}
                    scroll={{ y: "calc(100vh - 65px - 45px)" }}
                    pagination={false}
                    rowKey={record => record.uuid}
                />
            </div>
        );
    }
}


UserSamples = connect('appTab', 'userSamples', 'settings', UserSamples)


class EditorTab extends React.Component {
    render() {
        return (
            <div>
                <Col span={12}>
                    <UserSamples type='positives' />
                </Col>
                <Col span={12}>
                    <UserSamples type='negatives' />
                </Col>
            </div>
        );
    }
}

export default EditorTab;