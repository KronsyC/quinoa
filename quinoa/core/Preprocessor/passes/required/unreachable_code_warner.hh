#pragma once

#include "../include.h"


void warn_unreachable_code(Block &block) {

    int returned_node_idx = -1;
    int i = 0;
    bool has_errored = false;

    for (auto code: block.content) {
        if (returned_node_idx != -1 && !has_errored) {
            Logger::warn("Unreachable Code Detected at\n" + block.str());
            has_errored = true;
        }
        if (code->returns() == ReturnChance::DEFINITE)returned_node_idx = i;
        i++;
    }

    for (auto code: block.flatten()) {
        if (code->scope != &block) {
            continue;
        }

        if (auto nest = dynamic_cast<Block *>(code)) {
            warn_unreachable_code(*nest);
        }
    }

    // Delete the redundant nodes
    if (returned_node_idx != -1) {
        int del_count = block.content.len() - (returned_node_idx + 1);
        for (; del_count > 0; del_count--) {
            block.content.pop();
        }
    }
}


void warn_unreachable_code(CompilationUnit &unit) {
    for (auto method: unit.get_methods()) {
        if (!method->content)continue;
        warn_unreachable_code(*method->content);
    }
}
