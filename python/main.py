from fastapi import FastAPI

import sys

sys.path.insert(0, r"C:\Users\mohit\OneDrive\Documents\GitHub\btree-db-copy\build\Release")

import btree_engine as db

app = FastAPI()

DB_FILE = "test.db"
META_FILE = "test_meta.db"

# Instantiate BTree object natively
tree = db.BTree(DB_FILE, META_FILE)

@app.post("/add/{key}/{val}")
def add_elem(key:int, val:int):
    tree.insert(key, val)

@app.get("/print")
def print_tree_endpoint():
    tree.print_tree()
    return {"status": "printed to stdout"}