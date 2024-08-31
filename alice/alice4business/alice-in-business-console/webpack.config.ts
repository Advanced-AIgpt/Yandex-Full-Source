import * as path from 'path';
import * as webpack from 'webpack';
import * as glob from 'glob';
import MiniCssExtractPlugin from 'mini-css-extract-plugin';
import OptimizeCSSAssetsPlugin from 'optimize-css-assets-webpack-plugin';
import UglifyJsPlugin from 'uglifyjs-webpack-plugin';
import nodeExternals from 'webpack-node-externals';

const { PASKILLS_URL_ROOT = '/b2b/' } = process.env;

const commonConfig: webpack.Configuration = {
    resolve: {
        extensions: ['.js', '.jsx', '.ts', '.tsx'],
    },
    module: {
        rules: [
            {
                test: /\.tsx?$/,
                use: 'awesome-typescript-loader',
            },
            {
                test: /\.i18n\//,
                use: 'webpack-bem-i18n-loader',
            },
        ],
    },
    plugins: [
        new webpack.ContextReplacementPlugin(/moment[\/\\]locale$/, /ru/),
        new webpack.ProvidePlugin({
            React: 'react',
        }),
        new webpack.DefinePlugin({
            'process.env': {
                BEM_LANG: JSON.stringify('ru'),
            },
        }),
    ],
    optimization: {
        minimizer: [
            new UglifyJsPlugin({
                cache: true,
                parallel: true,
            }),
            new OptimizeCSSAssetsPlugin(),
        ],
    },
    stats: 'errors-only',
};

export default (env: any, options: webpack.Configuration) => {
    const CLIENT_ENTRY_DIR = 'src/client';
    const CLIENT_OUTPUT_DIR = path.resolve(__dirname, 'build/assets');

    const SERVER_ENTRY_DIR = 'src/server/views';
    const SERVER_OUTPUT_DIR = path.resolve(__dirname, 'build/server/views');

    return [
        {
            ...commonConfig,
            name: 'client bundles',
            target: 'web',
            entry: glob.sync(`./${CLIENT_ENTRY_DIR}/*.tsx`).reduce(
                (entries, file) => ({
                    ...entries,
                    [path.basename(file, '.tsx')]: file,
                }),
                {},
            ),
            output: {
                path: CLIENT_OUTPUT_DIR,
                publicPath: PASKILLS_URL_ROOT,
                filename: 'js/[name].js',
            },
            module: {
                ...commonConfig.module,
                rules: [
                    ...commonConfig.module!.rules,
                    {
                        test: /\.css$/,
                        use: [MiniCssExtractPlugin.loader, 'css-loader', 'postcss-loader'],
                    },
                    {
                        test: /\.less$/,
                        use: [MiniCssExtractPlugin.loader, 'css-loader', 'less-loader'],
                    },
                    {
                        test: /\.scss$/,
                        use: [MiniCssExtractPlugin.loader, 'css-loader', 'sass-loader'],
                    },
                    {
                        test: /\.svg$/,
                        issuer: [/\.s?css$/, /\.less/],
                        exclude: /\.inline\.svg$/,
                        use: 'url-loader',
                    },
                    {
                        test: /\.(gif|png)$/,
                        issuer: /\.css$/,
                        include: /node_modules\/lego-on-react/,
                        use: 'url-loader',
                    },
                    {
                        test: /\.(woff2?|eot|ttf)$/,
                        issuer: /\.css$/,
                        use: 'file-loader',
                    },
                    {
                        test: /\.svg$/,
                        exclude: [/node_modules/, /\.inline\.svg$/],
                        use: ['svg-react-loader', 'image-webpack-loader'],
                    },
                    {
                        test: /\.inline\.svg$/,
                        exclude: /node_modules/,
                        use: 'url-loader',
                    },
                ],
            },
            plugins: [
                new MiniCssExtractPlugin({
                    filename: 'css/[name].css',
                    chunkFilename: '[id].css',
                }),
                ...commonConfig.plugins!,
            ],
        } as webpack.Configuration,

        {
            ...commonConfig,
            name: 'ssr views',
            target: 'node',
            entry: glob.sync(`./${SERVER_ENTRY_DIR}/*.tsx`).reduce(
                (entries, file) => ({
                    ...entries,
                    [path.basename(file, '.tsx')]: file,
                }),
                {},
            ),
            output: {
                path: SERVER_OUTPUT_DIR,
                filename: '[name].bundle.js',
                libraryTarget: 'commonjs2',
                libraryExport: 'default',
            },
            externals: [
                nodeExternals({
                    modulesFromFile: {
                        include: 'dependencies',
                    },
                }),
                (ctx: string, request: string, cb: (err?: any, res?: string) => void) => {
                    const absPath = path.resolve(ctx, request);
                    const projectRootRelativePath = path.relative(__dirname, absPath);

                    if (
                        projectRootRelativePath.startsWith('src/') &&
                        !projectRootRelativePath.startsWith(CLIENT_ENTRY_DIR) &&
                        !projectRootRelativePath.startsWith(SERVER_ENTRY_DIR)
                    ) {
                        const resultPath =
                            options.mode === 'production' ? request : path.relative(SERVER_OUTPUT_DIR, absPath);

                        cb(null, `commonjs ${resultPath}`);
                    } else {
                        cb();
                    }
                },
            ],
            module: {
                ...commonConfig.module,
                rules: [
                    ...commonConfig.module!.rules,
                    {
                        test: /\.(c|le|sc)ss$/,
                        use: 'null-loader',
                    },
                    {
                        test: /\.svg$/,
                        exclude: [/node_modules/, /\.inline\.svg$/],
                        use: 'null-loader',
                    },
                    {
                        test: /\.inline\.svg$/,
                        exclude: /node_modules/,
                        use: 'null-loader',
                    },
                ],
            },
        } as webpack.Configuration,
    ];
};
