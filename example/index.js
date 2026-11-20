import { AppRegistry } from 'react-native';
import App from './App';

try {
  // Expo environment bootstrap
  const { registerRootComponent } = require('expo');
  registerRootComponent(App);
} catch {
  // Bare React Native CLI bootstrap
  AppRegistry.registerComponent('AudieeExample', () => App);
}
