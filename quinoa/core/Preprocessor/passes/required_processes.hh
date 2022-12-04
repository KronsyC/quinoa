#pragma once

#include "./include.h"

#include "./required/compositor_resolver.hh"
#include "./required/array_validator.hh"
#include "./required/call_qualifier.hh"
#include "./required/type_resolver.hh"
#include "./required/type_checker.hh"
#include "./required/type_table_builder.hh"
#include "./required/variable_usage_validator.hh"
#include "./required/unreachable_code_warner.hh"
#include "./required/type_ref_resolver.hh"



MaybeError apply_method_processes(Method& fn){

    // Each method can be independently resolved
    //
    // loop until:
    // 1. All processes fail (ie no progress was made)
    // 2. All processes succeed (ie finished)

    std::vector<std::string> errors;

    bool red_zone = false;

    while(true){

        errors.clear();

        auto callq = qualify_calls(fn);
        auto typer = resolve_types(fn);


        if(!callq.first && !typer.first && (callq.second || typer.second)){
            // no progress was made and there were errors* (important, prevents edge case)
            // iterate one more time
            // if the next iteration fails
            // the whole compilation fails
            // otherwise, there was simply a
            // co-dependence that needed resolution

            if(red_zone){
                // error
                if(callq.second)for(auto e : *callq.second)errors.push_back(e);
                if(typer.second)for(auto e : *typer.second)errors.push_back(e);
                break;
            }
            else{
                // one more chance
                red_zone = true;
                continue;
            }
        }
        red_zone = false;

        // if no progress was made and no errors were thrown, it is safe to break
        // otherwise, continue

        if(!callq.second && !typer.second)break;
    }

    if(errors.size())return errors;
    else return MaybeError();

}


void process_required(CompilationUnit *unit) {
    resolve_compositors(*unit);
    resolve_type_references(*unit);

    std::vector<std::string> error_messages;

    for(auto method : unit->get_methods()){
        if(!method->content)continue;
        auto result = apply_method_processes(*method);

        if(result.has_value()){
            for(auto err : result.value()){
                error_messages.push_back(err);
            }
        }
    }

    if(error_messages.size()){

        std::string error_message = std::string("\n") + (error_messages.size() > 10 ? "------------ Resolution Error Starts Here ------------" : "Resolution Error Below");

        for(auto e : error_messages){
            error_message += "\n>>\t" + e;
        }
        error_message+="\n\nA Resolution Error has been encountered, see above for more information";

        except(E_ERR, error_message);
    }

    Logger::clearQueue();
    Logger::enqueueMode(false);
    build_type_table(*unit);
    validate_array_literals(*unit);
    check_types(*unit);
    validate_variable_usage(*unit);
    warn_unreachable_code(*unit);

}