#!/bin/python
import json
import os
from typing import List

__DEFPATH__ = os.path.abspath("data/syntax.defn")
__TGTPATH__ = os.path.abspath("quinoa/GenMacro.h")


class Definition:
    id: str
    properties: dict

    def __init__(self, id, props):
        self.id = id
        self.properties = props

    def __repr__(self):
        return self.id + " -> " + str(self.properties)


def_file = open(__DEFPATH__, "r")

ALL_DEFINITIONS: List[Definition] = []

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

        if type(value) == list:
            int_value = []
            try:
                for v in value:
                    int_value.append(int(v))
                if len(int_value) == 1:
                    value = int_value[0]
                else:
                    value = int_value
            except:
                pass

        props[dirname] = value
    else:
        tokname = line
        ALL_DEFINITIONS.append(Definition(tokname, props))
        props = {}

PROPTYPE_BOOL = 0
PROPTYPE_STR = 1
PROPTYPE_STRLIST = 2
PROPTYPE_NUM = 3
PROPTYPE_NUMLIST = 4
STRUCTURE = {}

# Resolve Types Into the structure, the structure will be used as a template for the 'definition' class
for defi in ALL_DEFINITIONS:
    for k, v in defi.properties.items():
        TARGET_TYPE = None
        if type(v) == bool:
            TARGET_TYPE = PROPTYPE_BOOL
        if type(v) == int:
            TARGET_TYPE = PROPTYPE_NUM
        if type(v) == list:
            if type(v[0]) == int and len(v) == 1:
                TARGET_TYPE = PROPTYPE_NUM
            elif type(v[0]) == int:
                TARGET_TYPE = PROPTYPE_NUMLIST
            elif len(v) == 1:
                TARGET_TYPE = PROPTYPE_STR
            else:
                TARGET_TYPE = PROPTYPE_STRLIST
        if k in STRUCTURE:
            if STRUCTURE[k] != TARGET_TYPE:
                if STRUCTURE[k] == PROPTYPE_STR and TARGET_TYPE == PROPTYPE_STRLIST:
                    pass
                elif TARGET_TYPE == PROPTYPE_STR and STRUCTURE[k] == PROPTYPE_STRLIST:
                    TARGET_TYPE = PROPTYPE_STRLIST
                else:
                    raise Exception("Failed to get type for property: " + k)
        STRUCTURE[k] = TARGET_TYPE


def getDefaultValue(type: int):
    if type == PROPTYPE_BOOL: return False
    if type == PROPTYPE_NUM: return 0
    if type == PROPTYPE_NUMLIST: return []
    if type == PROPTYPE_STRLIST: return []
    if type == PROPTYPE_STR: return ""


def toArrayLiteral(val: List):
    ret = "{"
    first = True
    for v in val:
        if not first: ret += ", "
        ret += json.dumps(v)
        first = False
    return ret + "}"


definitions_str = ""
definitions_args = ""
definitions_default_assignments = ""
for k, v in STRUCTURE.items():
    val = ""
    if v == PROPTYPE_BOOL: val = f"bool {k} = false"
    if v == PROPTYPE_NUM: val = f"int {k} = 0"
    if v == PROPTYPE_STR: val = f"std::string {k} = \"\""
    if v == PROPTYPE_STRLIST: val = f"std::vector<std::string> {k} = {{}}"
    if v == PROPTYPE_NUMLIST: val = f"std::vector<int> {k} = {{}}"
    definitions_str += val + ";\n"
    definitions_args += "\n, " + val
    definitions_default_assignments += f"this->{k} = {k};\n"

token_type_def = ""
definitions_initializers = ""
for defi in ALL_DEFINITIONS:
    defname = "TT_" + defi.id
    token_type_def += f"{defname},\n"

    definitions_initializers += "new "
    definitions_initializers += "TokenDefinition("
    definitions_initializers += defname + ", " + json.dumps(defi.id)

    for k, v in STRUCTURE.items():
        definitions_initializers += ", "
        value = None
        try:
            value = defi.properties[k]
        except:
            value = getDefaultValue(v)

        if type(value) == list:
            definitions_initializers += toArrayLiteral(value)
        else:
            definitions_initializers += json.dumps(value)
    definitions_initializers += "),\n"

indentation_types = ""
indentation_mappings = ""
for defi in ALL_DEFINITIONS:
    if "ind" in defi.properties:
        generic_name = defi.id[2:]
        enum_name = "IND_" + generic_name + "s"
        indentation_types += enum_name + ",\n "
        indentation_mappings += f"{{{enum_name}, {{TT_l_{generic_name}, TT_r_{generic_name}}}}},\n"

infix_enum_members = ""
unary_enum_members = ""
infix_enum_mappings = ""
postfix_enum_mappings = ""
prefix_enum_mappings = ""
for defi in ALL_DEFINITIONS:
    if "prefix" in defi.properties:
        ename = "PRE_" + defi.id
        unary_enum_members += ename + ", \n"
        prefix_enum_mappings += "{ TT_" + defi.id + ", " + ename + "}, \n"
    if "postfix" in defi.properties:
        ename = "POST_" + defi.id
        unary_enum_members += ename + ", \n"
        postfix_enum_mappings += "{ TT_" + defi.id + ", " + ename + "}, \n"
    if "infix" in defi.properties:
        ename = "BIN_" + defi.id
        infix_enum_members += ename + ", \n"
        infix_enum_mappings += "{ TT_" + defi.id + ", " + ename + "}, \n"

primitives_enum_members = ""
primitives_enum_mappings = ""
primitives_enum_names = ""
primitives_enum_groups = ""
for defi in ALL_DEFINITIONS:
    if "type" in defi.properties:
        name = "PR_" + defi.id
        primitives_enum_members += name + ",\n"
        primitives_enum_mappings += "{ TT_" + defi.id + ", " + name + "},\n"
        primitives_enum_names += "{" + name + ", " + "\"" + defi.properties["alias"][0] + "\"},\n"
        primitives_enum_groups += "{" + name + ", " + "\"" + defi.properties["type"][0] + "\"" + "},\n"

# Intrinsic related macros
intrinsics_enum_members = ""
intrinsics_enum_mappings = ""
intrinsics_enum_names = ""

for defi in ALL_DEFINITIONS:
    if "intrinsic" in defi.properties:
        if len(defi.properties["alias"]) != 1: raise Exception("An intrinsic must have exactly one alias")
        name = defi.id
        intrinsics_enum_members += name + ",\n"
        intrinsics_enum_mappings += "{ \"" + defi.properties["alias"][0] + "\", " + name + "},\n"
        intrinsics_enum_names += "{ " + name + ", " + "\"" + defi.properties["alias"][0] + "\"},\n"


# To add a new macro to the source code
# Simply Write it's generator above
# and add it to the list of macros
MACRO_KVP = {
    "DEFINITIONS_STR": definitions_str,
    "DEFINITIONS_ARGS": definitions_args,
    "DEFINITIONS_DEFAULT_ASSIGNMENTS": definitions_default_assignments,
    "DEFINITIONS_INITIALIZERS": definitions_initializers,
    "DEFINITIONS_ENUM_MEMBERS": token_type_def,

    "INDENTATION_TYPES": indentation_types,
    "INDENTATION_MAPPINGS": indentation_mappings,

    "INFIX_ENUM_MEMBERS": infix_enum_members,
    "INFIX_ENUM_MAPPINGS": infix_enum_mappings,

    "UNARY_ENUM_MEMBERS": unary_enum_members,

    "PREFIX_ENUM_MAPPINGS": prefix_enum_mappings,
    "POSTFIX_ENUM_MAPPINGS": postfix_enum_mappings,

    "PRIMITIVES_ENUM_MEMBERS": primitives_enum_members,
    "PRIMITIVES_ENUM_MAPPINGS": primitives_enum_mappings,
    "PRIMITIVES_ENUM_NAMES": primitives_enum_names,
    "PRIMITIVES_ENUM_GROUPS": primitives_enum_groups,

    "INTRINSICS_ENUM_MEMBERS" : intrinsics_enum_members,
    "INTRINSICS_ENUM_NAMES" : intrinsics_enum_names,
    "INTRINSICS_ENUM_MAPPINGS" : intrinsics_enum_mappings,
}
CONTENTS = "#pragma once\n\n"

for entry in MACRO_KVP.keys():
    e = MACRO_KVP[entry].replace("\n", "\\\n\t")
    MACRO = f"#define {entry} \\\n\t{e}"
    CONTENTS += MACRO + "\n\n"

tgtfile = open(__TGTPATH__, "w")
tgtfile.write(CONTENTS)
tgtfile.close()
