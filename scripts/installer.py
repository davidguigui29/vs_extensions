import re
import sys
import shutil
import requests
import subprocess


class VSCodeExtensionInstaller:
    MARKETPLACE_URL = "https://marketplace.visualstudio.com/items?itemName={}&ssr=false#overview"

    def __init__(self, extension_id, install=False):
        if "." not in extension_id:
            raise ValueError("Extension ID must be in format 'publisher.extension'")
        self.publisher, self.extension = extension_id.split(".")
        self.filename = f"{self.extension}.vsix"
        self.vsix_url = ""
        self.install_flag = install
        self.code_command = self.detect_code_command()

    def detect_code_command(self):
        for cmd in ["code", "codium", "vscodium"]:
            if shutil.which(cmd):
                print(f"Using code CLI: {cmd}")
                return cmd
        raise EnvironmentError("No VS Code CLI tool found (tried 'code', 'codium', 'vscodium').")

    def extract_asset_uri(self, html: str):
        asset_match = re.search(r'"assetUri"\s*:\s*"([^"]+)"', html)
        fallback_match = re.search(r'"fallbackAssetUri"\s*:\s*"([^"]+)"', html)

        if asset_match:
            asset_uri = asset_match.group(1)
            self.vsix_url = f"{asset_uri}/Microsoft.VisualStudio.Services.VSIXPackage"
            print(f"Found VSIX URL via assetUri: {self.vsix_url}")
        elif fallback_match:
            fallback_uri = fallback_match.group(1)
            self.vsix_url = f"{fallback_uri}/Microsoft.VisualStudio.Services.VSIXPackage"
            print(f"Found VSIX URL via fallbackAssetUri: {self.vsix_url}")
        else:
            raise Exception("Could not find assetUri or fallbackAssetUri in page.")

    def fetch_download_url(self):
        print(f"Fetching Marketplace page for {self.publisher}.{self.extension}...")
        url = self.MARKETPLACE_URL.format(f"{self.publisher}.{self.extension}")
        response = requests.get(url)
        if response.status_code != 200:
            raise Exception(f"Failed to fetch Marketplace page (status: {response.status_code})")

        self.extract_asset_uri(response.text)

    def download_vsix(self):
        print(f"Downloading VSIX to {self.filename}...")
        response = requests.get(self.vsix_url)
        if response.status_code != 200:
            raise Exception("Failed to download the VSIX file.")

        with open(self.filename, "wb") as f:
            f.write(response.content)
        print("Download complete.")

    def install_extension(self):
        if not self.install_flag:
            print("Skipping installation. Use '-i' to install.")
            return

        print(f"Installing extension using {self.code_command} from {self.filename}...")
        subprocess.run([self.code_command, "--install-extension", self.filename], check=True)
        print("Installation complete!")

    def run(self):
        self.fetch_download_url()
        self.download_vsix()
        self.install_extension()


if __name__ == "__main__":
    args = sys.argv[1:]

    if not args:
        print("Usage: python installer.py [-i] <publisher.extension>")
        sys.exit(1)

    install_flag = False
    extension_arg = None

    for arg in args:
        if arg == "-i":
            install_flag = True
        elif "." in arg:
            extension_arg = arg

    if not extension_arg:
        print("Error: Missing extension name (e.g., publisher.extension)")
        sys.exit(1)

    try:
        installer = VSCodeExtensionInstaller(extension_arg, install=install_flag)
        installer.run()
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

