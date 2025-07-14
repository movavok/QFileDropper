# QFileDropper

[![Made with Qt](https://img.shields.io/badge/Made%20with-Qt-41CD52?logo=qt&logoColor=white)](https://www.qt.io/)
[![Language: C++](https://img.shields.io/badge/Language-C%2B%2B-blue.svg?logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Windows](https://img.shields.io/badge/Platform-Windows-0078D6?logo=windows&logoColor=white)](https://github.com/movavok/QFileDropper/releases/latest)
[![Release](https://img.shields.io/github/v/release/movavok/QFileDropper?label=release)](https://github.com/movavok/QFileDropper/releases)

**QFileDropper** is a Qt-based application for sending and receiving files between clients via a central server.

---

## 🔹 Features

### 🖥️ Client (DropperClient)
- Send files of any size
- Progress bars for upload/download
- Stores received files locally
- Local database saves IP, port, and login
- Sound effects for success and cancellation

### 🗄️ Server (DropperServer)
- Accepts multiple client connections
- Relays files between all connected clients

---

## 🖼️ Screenshots

### 🔹 Server Connection (Client Side)
Client login screen with IP, port, and username input.  
Includes checkboxes to save connection info in a local database.  
<img src="readme/screenshot_SerCon.png" width="450"/>

### 🔹 Sending & Receiving Files
Client interface during active file transfers with upload/download progress bars.  
<img src="readme/screenshot_Sen&RecFiles.png" width="900"/>

### 🔹 Received Files Directory
Received files are saved locally and displayed as clickable links.  
<img src="readme/screenshot_RecFilesDir.png" width="290"/>

### 🔹 Server Logs (Terminal)
The server terminal displays incoming connections, file transfers, and status messages.  
<img src="readme/screenshot_Server.png" width="400"/>

---

## 📥 Download

⬇️ [Download the latest release](https://github.com/movavok/QFileDropper/releases/latest)

> Please download the ZIP archive under "Assets".  
> Ignore the auto-generated "Source code" links below the release.