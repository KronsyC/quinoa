#include "./primary.hh"
#include "./container.hh"
#include "./reference.hh"

LLVMType::LLVMType(_Type qn_type) { this->qn_type = qn_type; }

llvm::Type* LLVMType::operator->() { return get_type(); }

bool LLVMType::is_signed() {
    if (auto prim = qn_type->get<Primitive>()) {
#define X(knd)                                                                                                         \
    if (prim->kind == knd)                                                                                             \
        return true;

        X(PR_int8)
        X(PR_int16)
        X(PR_int32)
        X(PR_int64)

        X(PR_float16)
        X(PR_float32)
        X(PR_float64)
    }
    return false;
}

LLVMType::operator llvm::Type*() { return get_type(); }

LLVMValue SourceVariable::assign_ptr(std::map<std::string, Variable>& vars) {
    auto var = scope->get_var(name->str());
    if (!var)
        except(E_ERR, "Failed to read variable: " + name->str());
    return var->as_value();
}

llvm::Type* LLVMType::get_type() const {
    if (!is_explicitly_constructed)
        except(E_INTERNAL, "Attempted to get type of implcitly constructed LLVMType");
    if (!cached_type) {
        auto llty = qn_type->llvm_type();
        if (!llty)
            except(E_INTERNAL, "Failed to get llvm_type for type: " + qn_type->str());
        return llty;
    }
    return cached_type;
}
LLVMValue LLVMValue::load() {
    // must be a pointer
    if (auto ptr_ty = this->type.qn_type->get<Ptr>()) {
        auto loaded_ty = ptr_ty->of->llvm_type();
        auto loaded_val = builder()->CreateLoad(loaded_ty, val);

        return {loaded_val, loaded_ty};
    }

    if (auto ref_ty = this->type.qn_type->get<ReferenceType>()) {
        auto loaded_ty = ref_ty->of->llvm_type();
        auto loaded_val = builder()->CreateLoad(loaded_ty, val);

        return {loaded_val, loaded_ty};
    }
    print();
    except(E_INTERNAL, "Can only load pointers or references, but the value "
                       "was found to be of type: " +
                           type.qn_type->str());
}

void LLVMValue::print() {
    this->val->print(llvm::outs());
    Logger::debug(" < LLVM Value of type: " + this->type.qn_type->str());
}

std::string ContainerRef::str() { return this->name->str(); }

std::string ContainerRef::mangle_str() {
    if (!this->refers_to)
        except(E_INTERNAL, "cannot generate mangle_str from unresolved module member");
    return this->refers_to->full_name().str();
}
bool LLVMType::operator==(LLVMType& other) const { return *other.qn_type->drill() == *qn_type->drill(); }

std::vector<Type*> Block::flatten_types() {
    std::vector<Type*> ret;

    for (auto member : this->content) {
        for (auto t : member->flatten_types()) {
            ret.push_back(t);
        }
    }
    return ret;
}

#include "../llvm_utils.h"
void Scope::decl_new_variable(std::string name, _Type type, bool is_constant) {
    this->vars[name] = Variable(type, nullptr, is_constant);
    this->type_table[name] = type;
}

Variable* Scope::get_var(std::string name) {
    if (vars.contains(name))
        return &vars[name];
    else if (parent_scope)
        return parent_scope->get_var(name);
    else
        return nullptr;
}

_Type Container::get_type(std::string name) {
    for (auto& member : members) {
        if (auto type = dynamic_cast<TypeMember*>(member.ptr)) {
            if (type->name->member->str() == name)
                return type->refers_to;
        }
    }

    if (name == this->name->str()) {
        return get_type();
    }

    // try each compositor sequentially until a match is found
    for (auto cmp : compositors) {
        if (includes(NATIVE_MODULES, cmp->name->str()))
            continue;
        auto cmp_mod = cmp->refers_to;

        if (!cmp_mod)
            except(E_INTERNAL, "unresolved compositor: " + cmp->name->str());
        if (auto typ = cmp_mod->get_type(name))
            return typ;
    }
    return _Type(nullptr);
}

Block* Statement::block() {
    if (auto blk = dynamic_cast<Block*>(scope)) {
        return blk;
    } else
        except(E_INTERNAL, "Failed to find block for expression: " + str());
}

#include "./container_member.hh"
void Block::generate(Method* qn_fn, llvm::Function* func, VariableTable& vars, ControlFlowInfo CFI) {
    for (auto& child : content) {
        child->generate(qn_fn, func, vars, CFI);
    }
}

std::string Method::signature() {
    std::string ret = name->str();

    if (this->generic_params.size()) {
        ret.push_back('<');
        bool first = true;
        for (auto g : generic_params) {
            if (!first)
                ret += ", ";
            ret += g->name->str();
            first = false;
        }
        ret.push_back('>');
    }
    bool first = true;
    ret += "(";
    for (auto p : parameters) {
        if (!first)
            ret += ", ";
        ret += p->name.str() + " : " + p->type->str();
        first = false;
    }
    ret += ") -> " + return_type->str();

    if (acts_upon) {
        ret += "\033[0;3m ( on " + acts_upon->str() + ")\033[0;0m";
    }
    return ret;
}
