/**
 * Expo Config Plugin for react-native-audiee
 * Automatically configures microphone permissions and audio background modes
 * for iOS Info.plist and Android AndroidManifest.xml during `expo prebuild`.
 */
const withAudiee = (config, props = {}) => {
  // 1. Configure iOS Info.plist
  if (!config.ios) {
    config.ios = {};
  }
  if (!config.ios.infoPlist) {
    config.ios.infoPlist = {};
  }

  // Microphone recording permission
  config.ios.infoPlist.NSMicrophoneUsageDescription =
    props.microphonePermission ||
    config.ios.infoPlist.NSMicrophoneUsageDescription ||
    'Allow $(PRODUCT_NAME) to access your microphone for studio-quality audio recording.';

  // Background audio mode for playback and recording
  const existingModes = config.ios.infoPlist.UIBackgroundModes || [];
  if (!existingModes.includes('audio')) {
    config.ios.infoPlist.UIBackgroundModes = [...existingModes, 'audio'];
  }

  // 2. Configure Android Permissions
  if (!config.android) {
    config.android = {};
  }
  const permissions = config.android.permissions || [];
  const requiredPermissions = ['RECORD_AUDIO', 'MODIFY_AUDIO_SETTINGS'];

  for (const perm of requiredPermissions) {
    if (!permissions.includes(perm)) {
      permissions.push(perm);
    }
  }
  config.android.permissions = permissions;

  return config;
};

module.exports = withAudiee;
