const path = require('path');
const { getDefaultConfig } = require('@react-native/metro-config');

const root = path.resolve(__dirname, '..');

const config = getDefaultConfig(__dirname);

config.watchFolders = [root];
config.resolver.extraNodeModules = {
  'react-native-audiee': root,
};

module.exports = config;
