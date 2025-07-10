# SimpleProxy

## Overview

simpleProxy is a lightweight HTTP/HTTPS proxy server written in C. It supports forwarding both plain HTTP traffic and HTTPS CONNECT tunneling with basic authentication. It uses non-blocking sockets and efficient event handling to handle multiple clients simultaneously.

### How It Works

- Listens on a configured port for incoming client connections.
- Parses HTTP requests, including CONNECT method for HTTPS tunneling.
- Establishes connections to target servers on behalf of clients.
- Forwards data bidirectionally between clients and remote servers.
- Supports basic user authentication.
- Handles both HTTP requests and raw TCP forwarding for HTTPS.
- Logs basic connection and error information.

## simpleProxy Installation from Latest GitHub .deb Package

1. Download the latest .deb package from GitHub releases:

   - Go to the GitHub releases page of simpleProxy:
     https://github.com/julecko/simpleProxy/releases

   - Find the latest release and download the file named like:
     simpleproxy-<version>-Linux.deb

   Example using wget (replace URL with actual latest .deb link):

   wget https://github.com/julecko/simpleProxy/releases/download/<version>/simpleproxy-<version>-Linux.deb

2. Install the downloaded .deb package:

   sudo dpkg -i simpleproxy-<version>-Linux.deb

3. Fix any missing dependencies (if needed):

   sudo apt-get install -f

4. Verify installation:

   simpleproxy-conf --help

5. Run initial setup:

   sudo simpleproxy-conf setup

6. Check service status:

   systemctl status simpleproxy.service

7. Configure your system or browser to use the proxy:

   http://127.0.0.1:3128

## Notes:

- The package includes the proxy binary and the simpleproxy-conf tool.
- Root permissions are required for setup and service management.
- For detailed usage, see 'simpleproxy-conf --help' or the command list under.

## simpleproxy-conf Commands

The simpleproxy-conf tool provides commands to configure, manage, and maintain your simpleProxy server easily.

### Available Commands

- `setup`
  Interactive setup of proxy configurations including port and database settings.
  Requires root privileges (use sudo).

- `setup-service`
  Sets up and enables the systemd service for simpleProxy.

- `migrate`
  Creates necessary database tables for user management and other features.

- `drop`
  Drops all tables from the database (use with caution).
  

- `reset`
  Drops and recreates all tables, effectively resetting the database.

- `add -u <username>`
  Adds a new user to the proxy user database. You will be prompted for a password.

- `remove -u <username>`
  Removes an existing user from the database.

Examples

### Setup configuration (requires sudo)
`sudo simpleproxy-conf setup`

### Enable and start proxy service
`sudo simpleproxy-conf setup-service`

### Create database tables
`simpleproxy-conf migrate`

### Add a new user 'john'
`simpleproxy-conf add -u john`

### Remove user 'john'
`simpleproxy-conf remove -u john`

### Reset database
`simpleproxy-conf reset`
