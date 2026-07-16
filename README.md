# B+ Tree Database

A persistent disk-backed B+ Tree database implemented in modern C++. The project is organized into independent components that mirror the architecture of a real storage engine, separating indexing, page management, metadata, and record storage.

### Features

- Persistent disk-backed B+ Tree with automatic recovery after restart
- Point lookups, insertions, deletions, and ordered range traversal
- Node splitting, sibling redistribution, merging, and parent propagation
- Linked leaf nodes for efficient sequential scans
- Disk pager with binary page serialization
- LFU page cache with LRU tie-breaking for efficient page replacement
- Metadata pages for persistent root tracking and database reopening
- Modular architecture separating storage, indexing, and page management

## Project Structure

```text
.
├── src/
│   ├── btree.cpp/.h      # B+ Tree implementation
│   ├── pager.cpp/.h      # Disk pager and cache manager
│   ├── meta.cpp/.h       # Metadata persistence
│   ├── node.h            # B+ Tree node definition
│   ├── record.h          # Record definitions
│   └── main.cpp          # CLI / driver
├── CMakeLists.txt
└── .gitignore
```

## Build and Run

### Configure

```bash
cmake -S . -B build
```

### Build

```bash
cmake --build build
```

### Run (Windows)

```powershell
.\build\Debug\database.exe
```
