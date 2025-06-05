
## 🚀 VS Code Extension Downloader & Installer

This project provides multiple scripts to **download (and optionally install)** Visual Studio Code extensions using the official Marketplace API — **without using VS Code itself**.

Available in:

* 🐍 Python
* 💻 C++
* ⚙️ C
* 💡 JavaScript (Node.js)
* 🐚 Bash

---

### 📦 Supported Features

* Download any extension using its `publisher.extension` ID
* Install with VS Code CLI (optional)
* Cross-platform (depends on selected language)
* No scraping or hacks — uses official Marketplace API

---

### 📁 Scripts Directory

```bash
scripts/
├── installer.c       # Pure C version (using libcurl)
├── installer.cpp     # C++ version (using libcurl)
├── installer.js      # Node.js version
├── installer.py      # Python version
└── installer.sh      # Bash version using curl
```
---

### 📦 Clone the Repository

```bash
git clone https://github.com/davidguigui29/vs_extensions.git
cd vs_extensions
```

---


### ⚙️ Dependencies


### 🐍 Python Requirements

If you want to use the Python script, install the required dependencies first.

#### Option 1: Install via `requirements.txt`

```bash
pip install -r requirements.txt
```

#### Option 2: Manual installation

```bash
pip install beautifulsoup4==4.13.4 requests==2.32.3
```

> ⚠️ It’s recommended to use a virtual environment (`python3 -m venv venv && source venv/bin/activate`) before installing.

---

And your project structure might look like:

```
vs_extensions/
├── instruction.txt
├── README.md
├── requirements.txt
└── scripts
    ├── installer.c
    ├── installer.cpp
    ├── installer.js
    ├── installer.py
    └── installer.sh
```


### ⚙️ C and 💻 C++ require only `curl` or `libcurl`:

#### ✅ Option 1: **Standard Install Without Backports**

```bash
sudo apt update
sudo apt install libcurl4-openssl-dev
```

#### 🛠 Option 2: **Enable `bookworm-backports`**

If needed:

```bash
sudo nano /etc/apt/sources.list
# Add:
deb http://deb.debian.org/debian bookworm-backports main

sudo apt update
sudo apt install -t bookworm-backports libcurl4-openssl-dev
```

#### 🧪 Final Check

```bash
ls /usr/include/curl/curl.h
```

---

### 🧰 Build / Run Examples

#### 🐍 Python

```bash
python3 scripts/installer.py [-i] ms-python.python
```

#### ⚙️ C

```bash
gcc scripts/installer.c -lcurl -o c_installer
./c_installer -i ms-python.python
```

#### 💻 C++

```bash
g++ scripts/installer.cpp -lcurl -o cpp_installer
./cpp_installer -i ms-python.python
```

#### 💡 Node.js

```bash
node scripts/installer.js -i ms-python.python
```

#### 🐚 Bash

```bash
bash scripts/installer.sh -i ms-python.python
```

> Use `-i` flag to install the extension after download (requires `code(vs-code)`, `codium(VSCodium)`, or `vscodium` to be installed)

---