import fs
import os

SCRIPT_DIR = os.path.dirname(__file__)

TESTS_DIR = f"{SCRIPT_DIR}/programs/"


TESTS_PATHS = [TESTS_DIR + f for f in fs.listdir(TESTS_DIR)]



class TestFile:
    metadata
    content : str

    def __init__(self, content):
        self.content = content
        self.metadata = {}

    def __repr__(self):
        ret = "---"
        for k,v in self.metadata.entries():
            ret += f"{k} -> {v}"
        ret+="---"

        ret += content

        return ret


def open_test(path : str):
    file = open(path, "r")

    contents = str(file.read())
    
    file_obj = TestFile(contents)

    file.close()

    return file_obj

def parse_metadata(test : TestFile):
    if not test.content.startsWith("---"):return
    test.content = test.content[3:]

    end_idx = test.content.indexof("---")

    metadata = test.content[:end_idx].strip()
    test.content = test.content[end_idx+3:]

    metadata_entries = metadata.split("\n")

    for entry in metadata_entries:
        kvp = entry.split(":")
        if len(kvp) != 2:raise Exception("A metadata kvp must only contain data in the form `key : value`")
        key = kvp[0].strip()
        value = kvp[1].strip()

        test.metadata[key] = value
    return


files = [open_test(f) for f in TESTS_PATHS]
files = [parse_metadata(f) for f in files]

