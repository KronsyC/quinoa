/**
 * Containers are entities present at the top-level of a program,
 * the current containers in quinoa are:
 *
 * - Module
 * - Seed
 * - Struct (wip)
 */

#pragma once

#include "./ast.hh"
#include "./container_member.hh"
#include "./include.hh"
#include "./reference.hh"
#include "./type.hh"
#include <llvm/IR/LLVMContext.h>
#include <memory>

class CompilationUnit;

class TopLevelEntity : public ANode {
  public:
    CompilationUnit* parent = nullptr;
    bool is_imported = false;
    std::unique_ptr<Scope> scope;
};

class Import : public TopLevelEntity {
  public:
    LongName target;
    LongName alias;
    bool is_stdlib = false;

    Import() = default;
};

enum ContainerType { CT_NOTYPE, CT_MODULE, CT_SEED, CT_STRUCT };

class Container : public TopLevelEntity {
  public:
    struct MemberDeclaration {
        std::string name;
        llvm::Constant* initializer;
        _Type type;
        bool is_constant;
        bool is_public;
    };

    // These are the compiler voodoo modules, they have no
    // real implementation
    static inline std::vector<std::string> NATIVE_MODULES = {"Exported", "Entry", "CompilerImplemented"};
    std::unique_ptr<Name> name = std::make_unique<Name>("unknown");
    std::shared_ptr<Name> name_space;

    std::vector<MemberDeclaration> member_decls;
    Vec<ContainerMember> members;
    Vec<Generic> generics;
    Vec<ContainerRef> compositors;
    ContainerType type = CT_NOTYPE;
    // Map all aliases
    std::map<std::string, LongName> aliases;

    Vec<ContainerMember*> inherited_members;

    std::shared_ptr<ContainerRef> get_ref() {
        if (!self_ref) {
            self_ref = std::make_shared<ContainerRef>();
            self_ref->refers_to = this;
            self_ref->name = std::make_unique<LongName>(this->full_name());
        }
        return self_ref;
    }

    LongName full_name() const {
        LongName ret;
        if (name_space)
            ret.parts.push(*name_space);
        if (!name)
            except(E_INTERNAL, "container has no name?");
        ret.parts.push(*name);
        return ret;
    }

    ContainerRef* get_compositor(std::string comp_name) {
        for (auto comp : compositors) {
            if (comp->name->str() == comp_name) {
                return comp;
            }
        }
        return nullptr;
    }

    bool has_compositor(std::string comp_name) {
        if (get_compositor(comp_name))
            return true;
        return false;
    }

    _Type get_type(std::string name = "_");

    std::vector<Method*> get_methods() {
        std::vector<Method*> ret;
        for (auto& member : members) {
            if (auto method = dynamic_cast<Method*>(member.ptr)) {
                ret.push_back(method);
            }
        }
        for (auto member : inherited_members) {
            if (auto method = dynamic_cast<Method*>(*member.ptr)) {
                ret.push_back(method);
            }
        }
        return ret;
    }

    std::vector<Property*> get_properties() {
        std::vector<Property*> ret;

        for (auto& member : members) {
            if (auto prop = dynamic_cast<Property*>(member.ptr)) {
                ret.push_back(prop);
            }
        }

        return ret;
    }

    llvm::Module& get_mod() {
        if (!ll_ctx) {
            ll_ctx = std::make_unique<llvm::LLVMContext>();
            ll_ctx->setOpaquePointers(false);
        }

        if (ll_mod)
            return *ll_mod;
        auto ctx = std::make_unique<llvm::LLVMContext>();
        auto mod = std::make_unique<llvm::Module>(this->full_name().str(), *ll_ctx);
        this->ll_mod = std::move(mod);
        return *ll_mod;
    }
    std::unique_ptr<llvm::Module> take_mod() {
        if (ll_mod)
            return std::move(ll_mod);
        else {
            except(E_INTERNAL, "Container has no llvm module");
        }
    }
    llvm::IRBuilder<>& get_builder() {
        this->get_mod();
        if (builder)
            return *builder;
        this->builder = std::make_unique<llvm::IRBuilder<>>(*ll_ctx);
        return *this->builder;
    }

  private:
    std::shared_ptr<ContainerRef> self_ref;
    std::unique_ptr<llvm::Module> ll_mod;
    static inline std::unique_ptr<llvm::LLVMContext> ll_ctx;
    std::unique_ptr<llvm::IRBuilder<>> builder;
};
