#pragma once

#include "./include.h"

#include "./required/compositor_resolver.hh"
#include "./required/array_validator.hh"
#include "./required/call_qualifier.hh"
#include "./required/importer.hh"
#include "./required/type_resolver.hh"
#include "./required/property_resolver.hh"
#include "./required/instance_call_resolver.hh"
void process_required(CompilationUnit* unit){
    resolve_imports(unit);
    resolve_compositors(*unit);
    resolve_props(*unit);
    bool resolvedTypes = false;
    bool resolvedCalls = false;
    bool resolvedInstances = false;
    Logger::enqueueMode(true);
    int run = 2;
    while (run)
    {
      Logger::clearQueue();
      auto typeres = resolve_types(*unit);
      auto res = qualify_calls(*unit);
      auto ins = resolveInstanceCalls(*unit);
      resolvedCalls = res.first;
      resolvedTypes = typeres.first;
      resolvedInstances = ins.first;

      if(resolvedCalls && resolvedTypes && resolvedInstances){
        run--;
        continue;
      }
      
      // if nothing was resolved this iteration
      if (!res.second && !typeres.second && !ins.second)
      {
        Logger::printQueue();
        error("Type-Call Resolution Failed with " + std::to_string(res.second) + " resolved calls and " + std::to_string(typeres.second) + " resolved types");
      }
    }
    Logger::clearQueue();
    Logger::enqueueMode(false);
}