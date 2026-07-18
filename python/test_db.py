import sys

sys.path.insert(0, r"C:\Users\mohit\OneDrive\Documents\GitHub\btree-db\build\Debug")

import btree_engine as db

DB_FILE = "test.db"
META_FILE = "test_meta.db"

tree = db.btree_open(DB_FILE, META_FILE)

while True:
    try:
        parts = input("> ").strip().split()

        if not parts:
            continue

        command = parts[0].upper()

        if command == "INSERT":
            key = int(parts[1])
            value = int(parts[2])
            db.insert(tree, key, value)

        elif command == "GET":
            key = int(parts[1])
            val = db.search(tree, key)
            if val == -1:
                print("Not found")
            else:
                print(val)

        elif command == "DELETE":
            key = int(parts[1])
            db.delete_key(tree, key)

        elif command == "PRINT":
            db.print_tree(tree)

        elif command == "RANGE":
            left = int(parts[1])
            right = int(parts[2])

            results = db.range_query(tree, left, right)
            print("Range query:")
            for k, v in results:
                print(f"Key: {k}, Val: {v}")

        elif command in ("EXIT", "END", "X"):
            db.btree_close(tree)
            break

        else:
            print("Unknown command")

    except (IndexError, ValueError):
        print("Invalid command.")
    except Exception as e:
        print(f"Error: {e}")