#pragma once


#include "./include.h"
#include "./meta/link_extern.hh"


void process_metadata(CompilationUnit &unit) {
    link_extern(unit);
}