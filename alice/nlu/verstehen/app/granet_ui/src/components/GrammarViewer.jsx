import React from 'react';
import MonacoEditor from 'react-monaco-editor';
import { Card } from 'antd';
import connect from 'storeon/react/connect';
import { text2pars } from '../util';

import GrammarMenu from './GrammarMenu';

class GrammarViewer extends React.Component {
    constructor(props) {
        super(props);
        // This binding is necessary to make `this` work in the callback
        this.onChange = this.onChange.bind(this);
    }

    onChange(newValue, e) {
        this.props.dispatch('currentGrammar/update', newValue);
    }

    editorDidMount(editor, monaco) {
        editor.focus();
    }

    editorWillMount(monaco) {
        if (!monaco.languages.getLanguages().some(({ id }) => id === 'Granet')) {
            // Register a new language
            monaco.languages.register({ id: 'Granet' });

            // Register a tokens provider for the language
            monaco.languages.setMonarchTokensProvider('Granet', {
                mainParts: ['root', 'form', 'filler', 'slots'],
                nonAlphabetical: /[^а-яА-Яё\w]|$/,
                operators: /[+?*|\[\]()]/,

                tokenizer: {
                    root: [
                        {
                            regex: /\s*[\w]+(?=:)|form/,
                            action: 'keyword',
                        },

                        {
                            regex: /\s*\$[\w.]+(?=@nonAlphabetical)/,
                            action: 'type',
                        },
                        {
                            regex: /%[\w]+(?=@nonAlphabetical)/,
                            action: 'tag',
                        },
                        {
                            regex: /@operators/,
                            action: 'tag',
                        },

                        {
                            regex: /\s+#.*$/,
                            action: 'comment',
                        },
                        {
                            regex: /^#.*$/,
                            action: 'comment',
                        },

                        {
                            regex: /"([^"\\]|\\.)*$/,
                            action: 'string.invalid',
                        },
                        {
                            regex: /"/,
                            action: { token: 'string.quote', bracket: '@open', next: '@string' },
                        },
                    ],

                    string: [
                        {
                            regex: /[^\\"]+/,
                            action: 'string',
                        },
                        {
                            regex: /"/,
                            action: { token: 'string.quote', bracket: '@close', next: '@pop' },
                        },
                    ],
                },
            });
        }
    }
    render() {
        const code = this.props.currentGrammar;
        const options = {
            selectOnLineNumbers: true,
            scrollbar: {
                vertical: "visible",
                verticalScrollbarSize: "1px"
            },
            // automaticLayout: true  // uncomment this to update monaco editor on window change, may hurt performance
        };
        return (
            <div>
                <GrammarMenu />
                <MonacoEditor
                    width="100%"
                    height="calc(80vh - 45px)"
                    language="Granet"
                    value={code}
                    options={options}
                    onChange={this.onChange}
                    editorWillMount={this.editorWillMount}
                    editorDidMount={this.editorDidMount}
                />
                <Card style={{ minHeight: "20vh" }}>
                    {text2pars(this.props.searchResultsGranet['compilerMessage'])}
                </Card>
            </div>
        );
    }
}

export default connect('currentGrammar', 'searchResultsGranet', GrammarViewer);