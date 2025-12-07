# Remote File System (RFS) with Versioning

A multithreaded client–server remote file system supporting **file upload, download, versioning, deletion, listing, and server shutdown**.

## Features

### ✔ WRITE
Upload a local file. Existing files are versioned automatically:
```
file → file.v1 → file.v2 → ...
```

### ✔ GET
Download files.  
Supports:
- newest version
- specific version via `-v`

### ✔ RM
Deletes a file **and all versioned copies** or removes a directory.

### ✔ LS
Lists all versions and timestamps.

### ✔ STOP
Shuts down the server remotely.

## Project Structure
```
rfs.c / rfs.h        # Client
server.c / server.h  # Server
rfs_root/            # Storage directory
```

## Build
```
gcc -pthread server.c -o server
gcc rfs.c -o rfs
```

## Run Server
```
./server
```

## Client Usage

### WRITE
```
./rfs WRITE local.txt remote/path/file.txt
```

### GET
```
./rfs GET remote/path/file.txt
./rfs GET -v 3 remote/path/file.txt
```

### RM
```
./rfs RM remote/path/file.txt
```

### LS
```
./rfs LS remote/path/file.txt
```

### STOP
```
./rfs STOP
```

## Versioning Behavior
- Original file becomes `.v1`
- `.v1` becomes `.v2`
- Newest file stays as the base name

## Thread Safety
- `pthread_mutex_t` protects filesystem access
- One thread per client connection

## Networking
- Fixed 5‑byte commands (`WRITE`, `GET  `, `LS   `, `RM   `, `STOP `)
- `send_all()` and `recv_all()` ensure full transmission

## Error Handling
Server returns structured codes for:
- not found
- directory not empty
- read/write/size errors