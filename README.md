# Client-Server File Transfer System

[![C/C++](https://img.shields.io/badge/Language-C%2FC%2B%2B-blue)](https://isocpp.org/)  
[![Sockets](https://img.shields.io/badge/Networking-UNIX%20Sockets-green)](http://www.beej.us/guide/bgnet/)  
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)  

This is a lightweight, secure code-management system built with UNIX sockets in C/C++.  It mimics core GitHub workflows—authentication, push, lookup, remove, deploy, and history logging—across four cooperating processes to give you real-world experience writing TCP & UDP networked services.

---

## 🌟 Key Features

- **User Authentication (Server A)**  
  - Case-sensitive login with 3-shift Caesar-cipher password encryption  
  - Guest access (“guest”/“guest”) vs. member access (validated against `members.txt`)  
- **Repository Management (Server R)**  
  - `push <filename>`: add or overwrite file metadata in `filenames.txt`  
  - `lookup <username>`: list public filenames for any user  
  - `remove <filename>`: delete a file entry from your repo  
- **Deployment (Server D)**  
  - `deploy`: batch-deploy all of a user’s files into `deployed.txt`  
- **History Logging (Extra Credit)**  
  - `log`: persistent, time-ordered record of all member actions  
- **Main Dispatcher (Server M)**  
  - Routes TCP client requests to the correct UDP backend server  
  - Ensures reliable message flow and prints clear on-screen status updates

---

## 🏗 Architecture

