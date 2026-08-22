import sys

sys.path.insert(0, r"C:\Users\mohit\OneDrive\Documents\GitHub\btree-db-copy\build\Release")

import btree_engine as db

DB_FILE = "test.db"
META_FILE = "test_meta.db"

# Instantiate BTree object natively
tree = db.BTree(DB_FILE, META_FILE)

while True:
    try:
        parts = input("> ").strip().split()

        if not parts:
            continue

        command = parts[0].upper()

        if command == "INSERT":
            key = int(parts[1])
            value = int(parts[2])
            tree.insert(key, value)

        elif command == "GET":
            key = int(parts[1])
            val = tree.search(key)
            if val == -1:
                print("Not found")
            else:
                print(val)

        elif command == "DELETE":
            key = int(parts[1])
            tree.delete_key(key)

        elif command == "PRINT":
            tree.print_tree()

        elif command == "RANGE":
            left = int(parts[1])
            right = int(parts[2])

            results = tree.range_query(left, right)
            print("Range query:")
            for k, v in results:
                print(f"Key: {k}, Val: {v}")

        elif command in ("EXIT", "END", "X"):
            del tree # deletes the C++ BTree object, triggering its destructor (which closes the pager)
            break

        else:
            print("Unknown command")

    except (IndexError, ValueError):
        print("Invalid command.")
    except Exception as e:
        print(f"Error: {e}")