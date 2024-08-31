// shared config (dev and prod)
const webpack = require("webpack");
const { resolve } = require("path");
const AssetsPlugin = require('assets-webpack-plugin');
const MiniCssExtractPlugin = require("mini-css-extract-plugin");
const MomentTimezoneDataPlugin = require("moment-timezone-data-webpack-plugin");
const MomentLocalesPlugin = require('moment-locales-webpack-plugin');

module.exports = {
  resolve: {
    extensions: [".js", ".jsx", ".ts", ".tsx", ".svg"],
  },
  context: resolve(__dirname, "../../"),
  entry: "./src/index.tsx",
  module: {
    rules: [
      {
        test: /\.tsx?$/,
        use: "ts-loader",
        exclude: /node_modules/,
      },
      {
        test: /\.css$/,
        use: [MiniCssExtractPlugin.loader, "css-loader"],
        exclude: /node_modules/,
      },
      {
        test: /\.(scss|sass)$/,
        use: [MiniCssExtractPlugin.loader, "css-loader", "sass-loader"],
        exclude: /node_modules/,
      },
      {
        test: /\.(jpe?g|png|gif)$/i,
        use: [
          "file-loader?hash=sha512&digest=hex&name=img/[contenthash].[ext]",
          "image-webpack-loader?bypassOnDebug&optipng.optimizationLevel=7&gifsicle.interlaced=false",
        ],
        exclude: /node_modules/,
      },
      {
        test: /\.svg$/,
        exclude: /node_modules/,
        use: [
          {
            loader: "@svgr/webpack",
            options: {
              dimensions: false,
            }
            // Сейчас *.svg файл обрабатывается babel-ом, а декларация написана ручками
            // Я не понял, как от этого избавиться
            // options: {
            //   typescript: true,
            //   babel: false,
            //   ext: "tsx",
            // },
          }
        ],
      },
    ],
  },
  externals: {
    'react': 'React',
    'react-dom' : 'ReactDOM',
    'react-bootstrap': 'ReactBootstrap',
  },
  plugins: [
      new AssetsPlugin({
        path: resolve(__dirname, "../../dist"),
        removeFullPathAutoPrefix: true,
      }),
      new webpack.WatchIgnorePlugin({
        paths: [
          /\.js$/,
          /\.d\.ts$/
        ]
      }),
      new MomentTimezoneDataPlugin({
        matchZones: ["Europe/Moscow", "UTC"]
      }),
      new MomentLocalesPlugin({
        localesToKeep: ['en', 'ru'],
      }),
  ],
  performance: {
    hints: false,
  },
  output: {
    filename: "[name].bundle.js",
    path: resolve(__dirname, "../../dist"),
    publicPath: "/",
  },
};
