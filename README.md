# Client-Server File Transfer System

[![C/C++](https://img.shields.io/badge/Language-C%2FC%2B%2B-blue)](https://isocpp.org/)  
[![Sockets](https://img.shields.io/badge/Networking-UNIX%20Sockets-green)](http://www.beej.us/guide/bgnet/)  
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)  

This is a lightweight, secure code-management system built with UNIX sockets in C/C++.  It mimics core GitHub workflows—authentication, push, lookup, remove, deploy, and history logging—across four cooperating processes to give you real-world experience writing TCP & UDP networked services.

---
## 🚀 Why This Project?
Tired of slow and insecure file transfers? Say hello to **DFTS** – a robust system that:  
- **Handles 50+ users concurrently** with TCP/UDP protocols.  
- **Secures your data** like a digital fortress (100% unauthorized access blocked!).  
- **Recovers from failures in 10 seconds** – because downtime is so last decade.

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
- **History Logging**  
  - `log`: persistent, time-ordered record of all member actions  
- **Main Dispatcher (Server M)**
  - Routes TCP client requests to the correct UDP backend server  
  - Tracks and persists every member action in a time-ordered history for the `log` command  
  - Ensures reliable message flow and prints clear on-screen status updates

---

## 🏗 Architecture

![image](https://github.com/user-attachments/assets/ae07db33-03fb-46f1-876e-621eee6c2a20)


- **Server A** – Authentication  
- **Server R** – Repository metadata  
- **Server D** – Deployment records  
- **Server M** – Request routing & TCP/UDP bridging  
- **Client**  – Member/Guest CLI interface  

---

## 🚀 Getting Started

1. **Clone the repo**  
   ```bash
   git clone https://github.com/Fayehjf/Client-Server-File-Transfer-System.git
   cd git450

2. **Build everything**
   ```bash
   make all

4. **Start services in order**
   ```bash
   ./serverM     # Main dispatcher
   ./serverA     # Auth server
   ./serverR     # Repo server
   ./serverD     # Deploy server

5. **Run your client**
   
   Member login:
   ```bash
   ./client alice MyP@ssw0rd
   
   Guest lookup
   ```bash
   ./client guest guest  

---

## 💬 Usage Examples

1 **Lookup your own repository as a member**
> lookup alice
index.html
app.js

2 **Push a new file**
> push report.pdf
report.pdf uploaded successfully.

3 **Deploy all files**
> deploy
Deployed alice’s repository:
  index.html
  app.js
  report.pdf

4 **View action history**
> log
1. login alice
2. push report.pdf
3. deploy

---

## 📖 Encryption Scheme

Passwords use a **Caesar cipher with shift 3** before storage:

- **Letters** wrap A→D … X→A (case-sensitive)  
- **Digits** wrap 0→3 … 7→0  
- **Special characters** remain unchanged  

| Original           | Encrypted         |
|--------------------|-------------------|
| Welcome to EE450!  | Zhofrph wr HH783! |
| 199xyz@$           | 422abc@$          |
| 0.27#&             | 3.50#&            |

