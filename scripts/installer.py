import re
import sys
import json
import shutil
import requests
import subprocess
from bs4 import BeautifulSoup


class VSCodeExtensionInstaller:
    MARKETPLACE_URL_1 = "https://marketplace.visualstudio.com/items?itemName={}&ssr=false#overview"
    MARKETPLACE_URL_2 = "https://marketplace.visualstudio.com/items?itemName={}&ssr=false#review-details"
    MARKETPLACE_URL_3 = "https://marketplace.visualstudio.com/items?itemName={}&ssr=false#qna"
    MARKETPLACE_URL_4 = "https://marketplace.visualstudio.com/items?itemName={}&ssr=false#version-history"

    def __init__(self, extension_id, install=False, editor_cmd=None):
        if "." not in extension_id:
            raise ValueError("Extension ID must be in format 'publisher.extension'")
        self.publisher, self.extension = extension_id.split(".")
        self.filename = f"{self.extension}.vsix"
        self.vsix_url = ""
        self.install_flag = install
        self.code_command = self.detect_code_command(editor_cmd)

    def detect_code_command(self, preferred=None):
        if preferred:
            if shutil.which(preferred):
                print(f"Using specified code CLI: {preferred}")
                return preferred
            else:
                raise EnvironmentError(f"Specified CLI '{preferred}' not found in PATH.")
        for cmd in ["code", "codium", "vscodium"]:
            if shutil.which(cmd):
                print(f"Using auto-detected code CLI: {cmd}")
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
            print("Trying third fallback (BeautifulSoup JSON scan)...")
            soup = BeautifulSoup(html, "html.parser")
            scripts = soup.find_all("script", type="application/json")
            for script in scripts:
                try:
                    data = json.loads(script.string)
                    json_text = json.dumps(data)
                    match = re.search(
                        r'"assetType"\s*:\s*"Microsoft\.VisualStudio\.Services\.VSIXPackage"\s*,\s*"source"\s*:\s*"([^"]+)"',
                        json_text)
                    if match:
                        self.vsix_url = match.group(1)
                        print(f"Found VSIX URL via embedded JSON: {self.vsix_url}")
                        return
                except Exception:
                    continue
            raise Exception("Could not find assetUri, fallbackAssetUri, or embedded VSIXPackage source in page.")

    def fetch_download_url(self):
        print(f"Fetching Marketplace page for {self.publisher}.{self.extension}...")

        urls = [
            self.MARKETPLACE_URL_1.format(f"{self.publisher}.{self.extension}"),
            self.MARKETPLACE_URL_2.format(f"{self.publisher}.{self.extension}"),
            self.MARKETPLACE_URL_3.format(f"{self.publisher}.{self.extension}"),
            self.MARKETPLACE_URL_4.format(f"{self.publisher}.{self.extension}")
        ]

        for url in urls:
            print(f"Trying URL: {url}")
            response = requests.get(url)
            if response.status_code != 200:
                print(f"Failed to fetch: {url} (status: {response.status_code})")
                continue
            try:
                self.extract_asset_uri(response.text)
                return  # Success
            except Exception as e:
                print(f"Extraction failed on this URL: {e}")
                continue

        raise Exception("Failed to find VSIX download URL from all known Marketplace pages.")

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
        print("Usage: python installer.py [-i] [-e editor] <publisher.extension>")
        sys.exit(1)

    install_flag = False
    extension_arg = None
    editor_cmd = None

    i = 0
    while i < len(args):
        if args[i] == "-i":
            install_flag = True
        elif args[i] == "-e":
            i += 1
            if i < len(args):
                editor_cmd = args[i]
            else:
                print("Error: -e flag requires an argument (code/codium/vscodium).")
                sys.exit(1)
        elif "." in args[i]:
            extension_arg = args[i]
        i += 1

    if not extension_arg:
        print("Error: Missing extension name (e.g., publisher.extension)")
        sys.exit(1)

    try:
        installer = VSCodeExtensionInstaller(extension_arg, install=install_flag, editor_cmd=editor_cmd)
        installer.run()
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
