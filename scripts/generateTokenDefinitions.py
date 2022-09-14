#!/bin/python
from ctypes import cast
import os
import time
from typing import List
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

ALL_DEFINITIONS:List[Definition] = []

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
        value = parts if parts else True
        if type(value) == list and len(value) == 1:
            try:
                value = int(value[0])
            except: pass
        props[dirname] = value
    else:
        tokname = line
        ALL_DEFINITIONS.append(Definition(tokname, props))
        props = {}

end = time.time()

elapsed = end - begin

print(f"Successfully Parsed Definitions in {round(elapsed*1000, 3)}ms")

PROPTYPE_BOOL = 0
PROPTYPE_STR = 1
PROPTYPE_STRLIST = 2
PROPTYPE_NUMBER = 3
STRUCTURE = {}

# Resolve Types Into the structure, the structure will be used as a template for the 'definition' class
for defi in ALL_DEFINITIONS:
    for k, v in defi.properties.items():
        TARGET_TYPE=None
        if type(v) == bool:
            TARGET_TYPE = PROPTYPE_BOOL
        if type(v) == int:
            TARGET_TYPE = PROPTYPE_NUMBER
        if type(v) == list:
            if len(v) == 1:
                TARGET_TYPE = PROPTYPE_STR
            else:
                TARGET_TYPE = PROPTYPE_STRLIST
        if k in STRUCTURE:
            if STRUCTURE[k] != TARGET_TYPE:
                if STRUCTURE[k] == PROPTYPE_STR and TARGET_TYPE == PROPTYPE_STRLIST:
                    pass
                elif TARGET_TYPE == PROPTYPE_STR and STRUCTURE[k] == PROPTYPE_STRLIST:
                    TARGET_TYPE=PROPTYPE_STRLIST
                else:
                    raise Exception("Failed to get type for property: "+ k)
        STRUCTURE[k] = TARGET_TYPE
print(STRUCTURE)
print("")