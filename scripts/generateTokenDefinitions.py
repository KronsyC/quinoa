#!/bin/python
import os
import time
__DEFPATH__ =  os.path.abspath("data/syntax.defn")

class Definition:
    id:str
    properties:dict

    def __init__(self, id, props):
        self.id = id
        self.properties = props

    def __repr__(self):
        return self.id + " -> " + str(self.properties)


print("------- Quinoa Tokengen  -------")
print(f"Searching for {__DEFPATH__}")
def_file = open(__DEFPATH__, "r")
print("Located the file Successfully")
print("Parsing Definitions...")

begin = time.time()

ALL_DEFINITIONS = []

# Accumulated Property Directives
props = {}
for line in def_file:
    line = line.strip()
    if line == "": continue
    if line.startswith("#"): continue
    if line.startswith("@"):
        line = line[1:]
        parts = line.split(" ")
        dirname = parts[0]
        parts = parts[1:]
        props[dirname] = parts if parts else True
    else:
        tokname = line
        ALL_DEFINITIONS.append(Definition(tokname, props))
        props = {}

end = time.time()

elapsed = end - begin

print(f"Successfully Parsed Definitions in {round(elapsed*1000, 3)}ms")
for defi in ALL_DEFINITIONS:
    print(defi)
print("")
print("")