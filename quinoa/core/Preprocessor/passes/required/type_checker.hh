/**
 *
 *
 */

#include "../include.h"

void validate_returns(Method &method) {
    auto &return_type = *method.return_type;

    for (auto node: method.content->flatten()) {
        if (auto ret = dynamic_cast<Return *>(node)) {
            auto prim = return_type.get<Primitive>();

            // method is void and returns value
            if (ret->value && prim && prim->kind == PR_void) {
                except(E_BAD_RETURN, "Method " + method.name->str() + " was declared as void, but returns a value '" +
                                     ret->value->str() + "'");
            }

            // method not void, but retuns void
            if (!ret->value && prim && prim->kind != PR_void) {
                except(E_BAD_RETURN, "Method " + method.name->str() + " has a return type of " + return_type.str() +
                                     " but contains a void return");
            }

            if(!ret->value)continue;
            else{

             auto &ret_value = *ret->value;
            
             auto ret_type = ret_value.type();
             if (!ret_type)except(E_BAD_RETURN, "Return Value '" + ret_value.str() + "' has an unresolved type");
              // Ensure the return value and return type are compatible
              auto distance = return_type.drill()->distance_from(*ret_type->drill());
            if (distance < 0)
                  except(E_BAD_RETURN, "Method " + method.name->str() + " has a return type of '" + return_type.str() +
                                       "', but a return statement was found to return '" + ret_value.type()->str() +
                                       "', which is incompatible with the signature");
              }
        }
    }
}

void check_types(CompilationUnit &unit) {

    for (auto m: unit.get_methods()) {
        if (!m->content)continue;
        validate_returns(*m);
    }
}
