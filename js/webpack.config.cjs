const path = require("path");
const webpack = require("webpack");

const isProduction = process.env.NODE_ENV == "production";

const config = {
  entry: "./soundswallower.bundle.js",
  output: {
    path: path.resolve("umd"),
    publicPath: "",
    filename: "bundle.js",
    library: {
      name: "soundswallower",
      type: "umd",
      export: "default",
    },
  },
  plugins: [
    new webpack.IgnorePlugin({
      resourceRegExp: /\.wasm$/,
    }),
  ],
};

module.exports = () => {
  if (isProduction) {
    config.mode = "production";
  } else {
    config.mode = "development";
  }
  return config;
};
