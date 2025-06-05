#!/usr/bin/env node

/**
 * Node.js script to download and optionally install a VS Code extension from the Marketplace.
 * Usage: node installer.js [-i] <publisher.extension>
 */

const https = require('https');
const fs = require('fs');
const path = require('path');
const { execSync, spawnSync } = require('child_process');

function usage() {
  console.log('Usage: node installer.js [-i] <publisher.extension>');
  process.exit(1);
}

function requireCmd(cmds) {
  for (const cmd of cmds) {
    try {
      execSync(`command -v ${cmd}`, { stdio: 'ignore' });
    } catch {
      console.error(`Missing required command: ${cmd}`);
      process.exit(1);
    }
  }
}

function detectCodeCommand() {
  for (const cmd of ['code', 'codium', 'vscodium']) {
    try {
      execSync(`command -v ${cmd}`, { stdio: 'ignore' });
      console.log(`Using code CLI: ${cmd}`);
      return cmd;
    } catch {}
  }
  throw new Error("No VS Code CLI tool found (tried 'code', 'codium', 'vscodium').");
}

function fetchHtml(url) {
  return new Promise((resolve, reject) => {
    https.get(url, { headers: { 'User-Agent': 'Node.js' } }, (res) => {
      if (res.statusCode !== 200) {
        reject(`Failed to fetch Marketplace page (status: ${res.statusCode})`);
        return;
      }
      let data = '';
      res.on('data', chunk => data += chunk);
      res.on('end', () => resolve(data));
    }).on('error', reject);
  });
}

function extractAssetUri(html) {
  const assetUriMatch = html.match(/"assetUri"\s*:\s*"([^"]+)"/);
  const fallbackUriMatch = html.match(/"fallbackAssetUri"\s*:\s*"([^"]+)"/);
  if (assetUriMatch) {
    const assetUri = assetUriMatch[1];
    const vsixUrl = `${assetUri}/Microsoft.VisualStudio.Services.VSIXPackage`;
    console.log(`Found VSIX URL via assetUri: ${vsixUrl}`);
    return vsixUrl;
  } else if (fallbackUriMatch) {
    const fallbackUri = fallbackUriMatch[1];
    const vsixUrl = `${fallbackUri}/Microsoft.VisualStudio.Services.VSIXPackage`;
    console.log(`Found VSIX URL via fallbackAssetUri: ${vsixUrl}`);
    return vsixUrl;
  } else {
    throw new Error("Could not find assetUri or fallbackAssetUri in page.");
  }
}

function downloadVsix(url, filename) {
  return new Promise((resolve, reject) => {
    const file = fs.createWriteStream(filename);
    https.get(url, (res) => {
      if (res.statusCode !== 200) {
        reject('Failed to download the VSIX file.');
        return;
      }
      res.pipe(file);
      file.on('finish', () => {
        file.close(() => {
          console.log('Download complete.');
          resolve();
        });
      });
    }).on('error', (err) => {
      fs.unlinkSync(filename);
      reject(err.message);
    });
  });
}

function installExtension(codeCmd, filename) {
  console.log(`Installing extension using ${codeCmd} from ${filename}...`);
  const result = spawnSync(codeCmd, ['--install-extension', filename], { stdio: 'inherit' });
  if (result.status !== 0) {
    throw new Error('Installation failed.');
  }
  console.log('Installation complete!');
}

// ---- Main logic ----
const args = process.argv.slice(2);

if (args.length === 0) usage();

let installFlag = false;
let extensionId = '';

for (const arg of args) {
  if (arg === '-i') {
    installFlag = true;
  } else if (arg.includes('.')) {
    extensionId = arg;
  }
}
if (!extensionId) {
  console.error("Error: Missing extension name (e.g., publisher.extension)");
  usage();
}
if (!extensionId.includes('.')) {
  console.error("Extension ID must be in format 'publisher.extension'");
  process.exit(1);
}

const [publisher, extension] = extensionId.split('.');
const filename = `${extension}.vsix`;
const marketplaceUrl = `https://marketplace.visualstudio.com/items?itemName=${publisher}.${extension}&ssr=false#overview`;

(async () => {
  requireCmd(['curl']); // Used only for the check; node handles network
  console.log(`Fetching Marketplace page for ${publisher}.${extension}...`);
  let html;
  try {
    html = await fetchHtml(marketplaceUrl);
  } catch (err) {
    console.error('Error fetching Marketplace page:', err);
    process.exit(1);
  }

  let vsixUrl;
  try {
    vsixUrl = extractAssetUri(html);
  } catch (err) {
    console.error('Error extracting VSIX URL:', err);
    process.exit(1);
  }

  try {
    await downloadVsix(vsixUrl, filename);
  } catch (err) {
    console.error('Error downloading VSIX:', err);
    process.exit(1);
  }

  if (installFlag) {
    let codeCmd;
    try {
      codeCmd = detectCodeCommand();
    } catch (err) {
      console.error(err.message);
      process.exit(1);
    }
    try {
      installExtension(codeCmd, filename);
    } catch (err) {
      console.error(err.message);
      process.exit(1);
    }
  } else {
    console.log('Skipping installation. Use \'-i\' to install.');
  }
})();