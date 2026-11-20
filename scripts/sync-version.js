const fs = require('fs');
const path = require('path');

const pkgPath = path.join(__dirname, '..', 'package.json');
const podspecPath = path.join(__dirname, '..', 'ios', 'react-native-audiee.podspec');

const pkg = JSON.parse(fs.readFileSync(pkgPath, 'utf8'));
const version = pkg.version;

console.log(`Syncing version ${version} to podspec...`);

let podspec = fs.readFileSync(podspecPath, 'utf8');
podspec = podspec.replace(/s\.version\s*=\s*["'][^"']+["']/, `s.version      = "${version}"`);

fs.writeFileSync(podspecPath, podspec, 'utf8');
console.log('✓ Successfully synchronized version to ios/react-native-audiee.podspec');
