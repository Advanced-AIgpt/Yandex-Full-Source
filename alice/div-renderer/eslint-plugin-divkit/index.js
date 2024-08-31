function checkTypedNode(typeNode, node) {
    const keys = Object.keys(typeNode);
    let equals = 0;

    for (const key of keys) {
        const value_key = node.properties.find(el => el.key && el.key.name === key);
        const value = value_key && value_key.value.value;

        if (
            value !== null &&
            (
                typeNode[key] === 'any' ||
                typeNode[key] === value ||
                typeNode[key] === typeof value
            )
        ) {
            equals++;
        }
    }

    return equals === keys.length;
}

const rulesForClasses = [
    [
        'FixedSize',
        {
            type: 'fixed',
            value: 'any',
        },
    ],
    [
        'WrapContentSize',
        {
            type: 'wrap_content',
        },
    ],
    [
        'MatchParentSize',
        {
            type: 'match_parent',
        },
    ],
    [
        'DivStateBlock',
        {
            type: 'state',
            states: 'any',
        },
    ],
    [
        'TextBlock',
        {
            type: 'text',
            text: 'any',
        },
    ],
    [
        'ImageBlock',
        {
            type: 'image',
            image_url: 'any',
        },
    ],
    [
        'SolidBackground',
        {
            type: 'solid',
            color: 'any',
        },
    ],
    [
        'ContainerBlock',
        {
            type: 'container',
            items: 'any',
        },
    ],
    [
        'GalleryBlock',
        {
            type: 'gallery',
            items: 'any',
        },
    ],
];

module.exports = {
    rules: {
        'use-classes': {
            meta: {
                type: 'problem',
                docs: {
                    description: 'Using class items for DivKit',
                },
                schema: [
                    {
                        type: 'object',
                        additionalProperties: false,
                    },
                ],
            },
            create(context) {
                return {
                    'ObjectExpression[properties.length>=1]': node => {
                        const ruleForClass = rulesForClasses.find(el => checkTypedNode(el[1], node));
                        if (ruleForClass) {
                            context.report({
                                node,
                                message: `This structure can be created using the class \`new ${ruleForClass[0]}()\``,
                            });
                        }
                    },
                };
            },
        },
    },
};
