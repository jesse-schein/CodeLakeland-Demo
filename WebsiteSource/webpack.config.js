var path = require('path');
const UglifyJSPlugin = require('uglifyjs-webpack-plugin');
const CompressionPlugin = require('compression-webpack-plugin')
const HtmlWebpackPlugin = require('html-webpack-plugin')
var HtmlWebpackInlineSourcePlugin = require('html-webpack-inline-source-plugin');

const webpack = require('webpack');

module.exports = {
  stats: "minimal",
    module:{
        rules: [
            {
                test: /\html$/,
                loader: 'html-loader'
            },
            {
                test: /\.css$/,
                use: ['style-loader','css-loader']
            },
            {
                test: /\.js$/,
                exclude: /(node_modules|bower_components)/,
                use: {
                    loader: 'babel-loader',
                    options: {
                      presets: ['@babel/preset-env']
                    }
                }
            },

        ]
    },
  entry: './src/index.js',
  output: {
    path: path.resolve(__dirname, '../data'),
    filename: './assets/main.js'
  },
  plugins: [
    new HtmlWebpackPlugin({
        template: './src/index.html',
        filename: './www/index.html',
        inject: false
    }),
    new webpack.ProvidePlugin({
        jQuery: 'jquery',
        $: 'jquery',
        jquery: 'jquery'
    }),
    new CompressionPlugin({
        test: /\.(js|html)/,
        deleteOriginalAssets: true
      }),
  ],
};
