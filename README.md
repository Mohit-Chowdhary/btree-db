# B+ Tree Database

A persistent disk-backed B+ Tree database implemented in modern C++. The project is designed as a lightweight storage engine with separate components for indexing, page management, metadata, caching, crash recovery, and language bindings.

## Features

- Persistent disk-backed B+ Tree index
- Point lookups, insertions, deletions, and range queries
- Automatic node splitting, sibling redistribution, merging, and root propagation
- Linked leaf nodes for efficient sequential traversal
- Disk pager with binary page serialization
- LFU page cache with LRU tie-breaking for page replacement
- Persistent metadata for automatic database reopening
- Write-Ahead Logging (WAL) for crash recovery and durability
- Python bindings using pybind11
- REST API interface built with FastAPI

## Project Structure

```text
.
├── src/
│   ├── bindings.cpp      # Python bindings
│   ├── btree.cpp/.h      # B+ Tree implementation
│   ├── main.cpp          # CLI / testing
│   ├── meta.cpp/.h       # Metadata persistence
│   ├── node.h            # B+ Tree node definition
│   ├── pager.cpp/.h      # Disk pager and page cache
│   ├── record.h          # Record definitions
│   └── wal.cpp/.h        # Write-Ahead Log (WAL)
├── CMakeLists.txt
└── .gitignore
```

## Build

### Configure

```bash
cmake -S . -B build
```

### Compile

```bash
cmake --build build
```

## Run

### CLI (Windows)

```powershell
.\build\Debug\database.exe
```

### Python

```python
import btree_engine
```

### REST API

```bash
uvicorn app:app --reload
```

## Future Work

- Improve WAL recovery and crash handling
- Add concurrent transactions and locking
- Expand the REST API
- Improve performance through further optimization
- Add comprehensive testing and benchmarking
