#!/bin/python

"""
Read All the tests provided in `/src` and generate proper quinoa source code from them

Test generator tags are wrapped in [= =]
They are internally converted to compiler assertions, with descriptive error messages
"""

import os
import sys
from typing import List
TESTS_DIR = os.path.realpath("tests/src")
OUT_DIR = os.path.realpath("tests/gen")
TEST_FILES = os.listdir(TESTS_DIR)
TESTS = {}

ASSERT = "__ASSERT__"

BOILERPLATE_BEFORE = f"""
import @lang::assertions as {ASSERT}; 

module Test is Entry{{

    func main(argc:int, argv:c_str*)->int{{
"""
BOILERPLATE_AFTER = f"""
        return 0;
    }}
}}
"""

TT_NUM = 0
TT_IDENT = 1
TT_EQ = 2

class Token:
    typ: int
    val: any

    def  __init__(self, typ, val=None):
        self.typ = typ
        self.val = val


def wrap(file:str)->str:
    file = "\n"+file
    file = file.replace("\n", "\n\t")[1:]
    return BOILERPLATE_BEFORE + file + BOILERPLATE_AFTER

def get_tok(str:str):
    if str == "equals":return Token(TT_EQ)
    elif str.isnumeric(): return Token(TT_NUM, int(str))
    return Token(TT_IDENT, str)

def tokify(content:str) -> List[Token]:
    toks = []
    strs = content.strip().split(" ")
    for s in strs:
        toks.append(get_tok(s))
    return toks

def codify(tok:Token)->str:
    if tok.typ == TT_IDENT: return tok.val
    if tok.typ == TT_NUM: return str(tok.val)
    raise Exception("Failed to Codify")

def gen_expects_assertion(content:str):
    tokens = tokify(content)
    if len(tokens) != 3: raise Exception("Only Binary Op Assertions are currently supported")
    op = tokens[1]
    o1 = codify(tokens[0])
    o2 = codify(tokens[2])
    if op.typ == TT_EQ:return f"{ASSERT}::equals({o1}, {o2});"

def make_assertion(assertion:str):
    # Generate language assertion code from the pseudocode syntax
    if assertion.startswith("expects"):return gen_expects_assertion(assertion[8:])
    else: raise Exception()

def generate_assert_calls(table):
    for i, assertion in table.items():
        table[i] = make_assertion(assertion)
    return table

def make_assertions(file:str) -> str:
    ASSERTION_IDX_MAP = {}

    # Locate Assertions, remove them and push them to the map
    while(1):
        try:
            assertion_op_idx = file.index("[=")
            assertion_cl_idx = file.index("=]")+2
            
            file_before_assertion = file[:assertion_op_idx]
            file_after_assertion = file[assertion_cl_idx:]

            assertion_content = file[assertion_op_idx+2:assertion_cl_idx-2].strip()
            ASSERTION_IDX_MAP[assertion_op_idx] = assertion_content

            file = file_before_assertion+file_after_assertion

        except ValueError:
            break

    assert_calls = generate_assert_calls(ASSERTION_IDX_MAP)

    # Inject the transformed calls into the code
    offset = 0
    for k,v in assert_calls.items():
        k = k+offset
        offset+=len(v)
        f_before_idx = file[:k]
        f_after_idx = file[k:]

        file = f_before_idx+v+f_after_idx


    return file

def generate_test(name:str, file:str):
    print(f"Generating Test For '{name}'")
    file = make_assertions(file)

    # create the corresponding source file
    newfile = wrap(file)
    f = open(f"{OUT_DIR}/{name}.qn", "w")
    f.write(newfile)
    f.close()


# Load the tests into a convenient dictionary
for test in TEST_FILES:
    if not test.endswith(".tq"):continue
    testname = ".".join(test.split(".")[:-1])
    path = f"{TESTS_DIR}/{test}"
    
    file = open(path, "r")
    content = file.read()
    content = content.replace("\n\n", "\n")
    TESTS[testname] = content

for name, file in TESTS.items():
    generate_test(name, file)

