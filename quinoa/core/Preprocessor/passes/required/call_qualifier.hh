#pragma once

#include "../include.h"
#include "../../../AST/advanced_operators.hh"

struct MatchRanking {
    Method *against = nullptr;

    /**
     * Different ranking metrics
     * (organized by evaluation order)
    */

    bool possible = false;

    // Number of arguments which are matched as varargs (lower is better)
    bool vararg_count = 0;

    // number of generic-reliant parameters (lower is better, overrides general compatibility (when applicable))
    int generic_count = 0;

    // general compatibility (Type Matching)
    int general_compat = 0;


    // If there is a match, add this to the generated definitions
    std::vector <std::shared_ptr<Type>> generate_with;

    MatchRanking(bool possible = false) {
        this->possible = possible;
    }

    auto print() {
        Logger::debug(
                "=== Rank for '" + (against ? against->name->str() : "unknown") + "' === \n" \
              "\t\tIs it possible? " + (possible ? "yes" : "no") + "\n" \
              "\t\tNo. of var-args: " + std::to_string(vararg_count) + "\n" \
              "\t\tNo. of generic parameters: " + std::to_string(generic_count) + "\n" \
              "\t\tGeneral Compatibility: " + std::to_string(general_compat)

        );
    }
};


/*
 * Create a ranking object based on the method-call pairing
 */
MatchRanking rank_method_against_call(Method *method, CallLike *call, bool is_static = true) {

    if (!method)except(E_INTERNAL, "Failed to create ranking object for method call (method is null)");

    MatchRanking ranking(true);
    ranking.against = method;

    if (method->acts_upon && is_static)return MatchRanking();
    else if (!method->acts_upon && !is_static)return MatchRanking();

    // Compare parameter counts (if applicable)
    if (!method->is_variadic()) {
        if (method->parameters.len() != call->args.len())return MatchRanking();
    } else {
        // Call must have at least `len(parameters) - 1` arguments
        if (call->args.len() < method->parameters.len() - 1)return MatchRanking();

        ranking.vararg_count = call->args.len() - (method->parameters.len() - 1);

    }


    if (method->generic_params.size()) {
        if (call->type_args.size() != method->generic_params.size())return MatchRanking();

        // The current implementation of the compiler requires generics to be explicitly
        // defined, later versions of the compiler may change this


        // TODO: Check the number of generic-reliant parameters (for use in ranking)

        ranking.generate_with = call->type_args;
        for (unsigned int i = 0; i < method->generic_params.size(); i++) {
            method->generic_params[i]->temporarily_resolves_to = call->type_args[i];
        }

    }
    ranking.possible = true;
    for (size_t i = 0; i < call->args.len(); i++) {
        auto arg_t = call->args[i].type();

        if (!arg_t) {
            ranking.possible = false;
            break;
        }
        auto &param_t = *method->get_parameter(i)->type;

        // Compare the types of the arg and param
        auto score = arg_t->distance_from(param_t);

        if (score == -1) {
            ranking.possible = false;
            break;
        }

        ranking.general_compat += score;
    }
    return ranking;
}

enum SelectionStage {
    INITIAL,
    VAR_ARGS,
    GENERICS,
    RATING
};

Method *select_best_ranked_method(std::vector <MatchRanking> &ranks, SelectionStage stage = SelectionStage::INITIAL) {

    switch (stage) {
        case SelectionStage::INITIAL: {
            std::vector <MatchRanking> suitors;
            for (auto r: ranks) {
                if (!r.possible)continue;
                suitors.push_back(r);
            }
            return select_best_ranked_method(suitors, SelectionStage::VAR_ARGS);

        }
        case SelectionStage::VAR_ARGS: {
            int smallest_vararg_count = -1;
            for (auto &r: ranks) {
                if (smallest_vararg_count == -1)smallest_vararg_count = r.vararg_count;
                else if (r.vararg_count < smallest_vararg_count)smallest_vararg_count = r.vararg_count;
            }
            std::vector <MatchRanking> suitors;
            for (auto &r: ranks) {
                if (r.vararg_count <= smallest_vararg_count)suitors.push_back(r);
            }
            return select_best_ranked_method(suitors, SelectionStage::GENERICS);
        }
        case SelectionStage::GENERICS: {
            int smallest_generic_count = -1;
            for (auto &r: ranks) {
                if (smallest_generic_count == -1)smallest_generic_count = r.generic_count;
                else if (r.generic_count < smallest_generic_count)smallest_generic_count = r.generic_count;
            }
            std::vector <MatchRanking> suitors;
            for (auto r: ranks) {
                if (r.generic_count <= smallest_generic_count)suitors.push_back(r);
            }
            return select_best_ranked_method(suitors, SelectionStage::RATING);
        }
        case SelectionStage::RATING: {
            int best_rating = -1;
            for (auto &r: ranks) {
                if (best_rating == -1)best_rating = r.general_compat;
                else if (r.general_compat < best_rating)best_rating = r.general_compat;
            }

            std::vector <MatchRanking> suitors;
            for (auto r: ranks) {

                if (r.general_compat <= best_rating)suitors.push_back(r);
            }
            if (suitors.size() == 0)return nullptr;

            // if there are more than one suitor, the call is ambiguous
            if (!suitors[0].against)except(E_INTERNAL, "Suitor has no `against` attribute");
            auto call_name = suitors[0].against->name->str();
            if (suitors.size() > 1)except(E_BAD_CALL, "Call to " + call_name + " is ambiguous");

            if (suitors[0].generate_with.size()) {
                suitors[0].against->generate_usages.push(suitors[0].generate_with);

            }
            return suitors[0].against;
        }
        default: except(E_INTERNAL, "bad selection stage");
    }
}

Method *get_best_target(MethodCall *call, CompilationUnit &unit) {
    // Find the module that the call targets
    Container *target = nullptr;
    auto call_mod_name = call->name->container->name->str();
    for (auto cont: unit.get_containers()) {
        if (cont->type != CT_MODULE)continue;
        if (call_mod_name == cont->full_name().str()) {
            target = cont;
            break;
        }
    }
    if (!target) {
        except(E_BAD_CALL, "Failed to locate module '" + call_mod_name + "' for call");
    }

    // Find all methods with the name
    std::vector < Method * > methods;

    auto call_method_name = call->name->member->str();

    for (auto method: target->get_methods()) {
        if (call_method_name != method->name->member->str())continue;
        methods.push_back(method);

    }

    if (methods.size() == 0)
        except(E_BAD_CALL, "No methods of '" + call_mod_name + "' have the name '" + call_method_name + "'");

    std::vector <MatchRanking> ranks;
    for (auto method: methods) {
        auto rank = rank_method_against_call(method, call);
        ranks.push_back(rank);
    }

    auto best_method = select_best_ranked_method(ranks);
    return best_method;
}

Method *get_best_target(MethodCallOnType *call, CompilationUnit &unit) {
    // Find the module that the call targets
    auto target_ty = call->call_on->type();
    if (!target_ty)return nullptr;
    auto target_paw_ty = target_ty->get<ParentAwareType>();
    if (!target_paw_ty) {


        Logger::error("You may only define methods for structs and enums");
        return nullptr;
    }

    auto target_mod = target_paw_ty->parent;

    // Find all methods with the name and compatible actionable type
    std::vector < Method * > methods;

    for (auto method: target_mod->get_methods()) {
        if (
                method->name->member->str() == call->method_name->str()
                && call->call_on->type()->distance_from(*method->acts_upon) >= 0
                ) {
            methods.push_back(method);
        }
    }
    if (methods.size() == 0)
        except(E_BAD_CALL, "No methods of '" + target_mod->name->str() + "' have the name '" +
                           call->method_name->str() + "'");

    std::vector <MatchRanking> ranks;
    for (auto method: methods) {

        auto rank = rank_method_against_call(method, call, false);
        ranks.push_back(rank);
    }

    auto best_method = select_best_ranked_method(ranks);
    return best_method;
}

std::pair<bool, int> qualify_calls(Method &code, CompilationUnit &unit) {
    int resolvedCount = 0;
    bool success = true;
    for (auto item: code.content->flatten()) {
        if (auto call = dynamic_cast<MethodCall *>(item)) {
            if (call->target)continue;
            auto best_fn = get_best_target(call, unit);
            if (best_fn) {
                call->target = best_fn;
                resolvedCount++;
            } else {
                Logger::error("Failed to resolve call " + call->str());
                success = false;
            }
        }
        if (auto inst_call = dynamic_cast<MethodCallOnType *>(item)) {

            if (inst_call->target)continue;
            auto best_fn = get_best_target(inst_call, unit);
            if (best_fn) {
                inst_call->target = best_fn;
                resolvedCount++;
            } else {
                Logger::error("Failed to resolve call " + inst_call->str());
                success = false;
            }
        }
    }
    return {success, resolvedCount};
}

std::pair<bool, int> qualify_calls(CompilationUnit &unit) {

    int count = 0;
    bool success = true;
    // Attempt to Qualify all Calls
    for (auto method: unit.get_methods()) {

        // skip out on signatures
        if (!method->content)continue;


        auto result = qualify_calls(*method, unit);
        if (!result.first)success = false;
        count += result.second;
    }
    return {success, count};
}