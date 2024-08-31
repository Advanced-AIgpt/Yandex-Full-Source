// production config
const { merge } = require("webpack-merge");
const { resolve } = require("path");
const commonConfig = require("./common");
const MiniCssExtractPlugin = require("mini-css-extract-plugin");

module.exports = merge(commonConfig, {
    mode: "production",
    devtool: "source-map",
    output: {
        path: resolve(__dirname, "../../dist"),
        filename: '[name]-[fullhash].bundle.js',
    },
    plugins: [
        new MiniCssExtractPlugin({
            filename: "[name].[fullhash].css",
            chunkFilename: "[id].[fullhash].css",
        }),
    ]
});
