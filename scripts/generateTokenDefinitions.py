#!/bin/python
from ctypes import cast
from operator import truediv
import os
import time
from typing import List
import json
__DEFPATH__ =  os.path.abspath("data/syntax.defn")
__TGTPATH__ = os.path.abspath("quinoa/compiler/generated/TokenDef.h")

class Definition:
    id:str
    properties:dict

    def __init__(self, id, props):
        self.id = id
        self.properties = props

    def __repr__(self):
        return self.id + " -> " + str(self.properties)


print("------- Quinoa Tokengen -------")
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

def getDefaultValue(type: int):
    if type == PROPTYPE_BOOL: return False
    if type == PROPTYPE_NUMBER: return 0
    if type == PROPTYPE_STRLIST: return []
    if type == PROPTYPE_STR: return ""

def toArrayLiteral(val: List):
    ret = "{"
    first = True
    for v in val:
        if not first: ret+=", "
        ret+= json.dumps(v)
        first = False
    return ret + "}"

definitions_str = ""
definitions_args = "TokenType ttype, std::string name"
definitions_default_assignments="this->ttype = ttype;\nthis->name=name;"
for k, v in STRUCTURE.items():
    val = ""
    if v == PROPTYPE_BOOL: val = f"bool {k} = false"
    if v == PROPTYPE_NUMBER: val = f"int {k} = 0"
    if v == PROPTYPE_STR: val = f"std::string {k} = \"\""
    if v == PROPTYPE_STRLIST: val = f"std::vector<std::string> {k}"
    definitions_str+="\n\t"+val+";"
    definitions_args+=", "+val
    definitions_default_assignments+=f"\n\t\tthis->{k} = {k};"

token_type_def = ""
definitions_initializers = ""
for defi in ALL_DEFINITIONS:
    defname = "TT_"+defi.id
    token_type_def+=f"\n\t\t{defname},"

    definitions_initializers+="\n\tnew "
    definitions_initializers+="TokenDefinition("
    definitions_initializers+=defname + ", " + json.dumps("__"+defi.id)

    for k, v in STRUCTURE.items():
        definitions_initializers+=", "
        value = None
        try:
            value = defi.properties[k]
        except:
            value = getDefaultValue(v)
        
        if type(value) == list:
            definitions_initializers+=toArrayLiteral(value)
        else:
            definitions_initializers+=json.dumps(value)
    definitions_initializers+="),"
    
    



TEMPLATE_CLASS_DEFN = f"""
/**
    THIS FILE IS AUTOMATICALLY GENERATED
    PLEASE DO NOT MAKE EDITS TO THIS FILE
    THEY WILL BE OVERRIDED

    to edit this file, instead edit the template at `scripts/generateTokenDefinitions.py`
    and afterwards, run the script to generate the changes
*/
#pragma once

#include<string>
#include<vector>

enum TokenType{{
    {token_type_def}
}};

class TokenDefinition{{
public:
    TokenType ttype;
    std::string name;
    {definitions_str}

    TokenDefinition({definitions_args}){{
        {definitions_default_assignments}
    }}
}};


std::vector<TokenDefinition*> defs{{
    {definitions_initializers}
}};
"""
print("Generated Source Content Successfully")
tgtfile = open(__TGTPATH__, "w")
tgtfile.write(TEMPLATE_CLASS_DEFN)
print("Wrote to file, exiting....")