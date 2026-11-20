const path = require('path');

let getDefaultConfig;
try {
  getDefaultConfig = require('expo/metro-config').getDefaultConfig;
} catch {
  getDefaultConfig = require('@react-native/metro-config').getDefaultConfig;
}

const root = path.resolve(__dirname, '..');
const config = getDefaultConfig(__dirname);

config.watchFolders = [root];
config.resolver.extraNodeModules = {
  'react-native-audiee': root,
};

module.exports = config;
