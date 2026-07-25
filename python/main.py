from fastapi import FastAPI

import sys

sys.path.insert(0, r"C:\Users\mohit\OneDrive\Documents\GitHub\btree-db\build\Debug")

import btree_engine as db

app = FastAPI()

DB_FILE = "test.db"
META_FILE = "test_meta.db"

tree = db.btree_open(DB_FILE, META_FILE)

@app.post("/add/{key}/{val}")
def add_elem(key:int, val:int):
    db.insert(tree,key,val);

@app.get("/print")
def print():
    return db.print_tree(tree)