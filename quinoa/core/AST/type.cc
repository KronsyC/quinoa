#include "./type.hh"
#include "./container.hh"
#include "./container_member.hh"
#include "type.hh"
#include <memory>

LLVMType get_common_type(LLVMType t1, LLVMType t2, bool repeat) {
    if (t1 == t2)
        return t1;

    auto q1 = t1.qn_type;
    auto q2 = t2.qn_type;

    if (q1->get<Primitive>() && q2->get<Primitive>()) {
        auto p1 = q1->get<Primitive>();
        auto p2 = q2->get<Primitive>();

#define ord(a, b)                                                                                                      \
    if (p1->kind == PR_##a && p2->kind == PR_##b)                                                                      \
        return t2;

        ord(int8, int16);
        ord(int8, int32);
        ord(int8, int64);

        ord(int16, int32);
        ord(int16, int64);

        ord(int32, int64);

        ord(uint8, uint16);
        ord(uint8, uint32);
        ord(uint8, uint64);

        ord(uint16, uint32);
        ord(uint16, uint64);

        ord(uint32, uint64);

        ord(uint8, int16);
        ord(uint8, int32);
        ord(uint8, int64);

        ord(uint16, int32);
        ord(uint16, int64);

        ord(uint32, int64);
    }
    if (repeat) {
        return get_common_type(t2, t1, false);
    } else
        except(E_NONEQUIVALENT_TYPES, "Failed to get a type common to " + q2->str() + " and " + q1->str());
}

std::string ParentAwareType::get_name() { return this->as_member()->name->str(); }

TypeMember* ParentAwareType::as_member() {
    for (auto mem : this->parent->members) {
        if (auto ty = dynamic_cast<TypeMember*>(mem.ptr)) {
            if (ty->refers_to.get() == this->getself())
                return ty;
        }
    }
    except(E_INTERNAL, "ParentAwareType::as_member failed");
}

GenericTable ParameterizedTypeRef::get_mapped_params() {
    // resolves_to has to be a ParentAwareType
    auto paw = dynamic_cast<ParentAwareType*>(resolves_to->drill());
    if (!paw)
        except(E_BAD_TYPE, "A Parameterized TypeRef must resolve to a ParentAwareType, but was found to resolve to " +
                               resolves_to->str());

    auto type_entry = paw->as_member();

    auto expected_ga_len = type_entry->generic_args.size();
    auto ga_len = this->params.size();

    if (ga_len != expected_ga_len) {
        except(E_BAD_TYPE, "Error while constructing Parameterized Type Ref to " + paw->str() + "\n\t\texpected " +
                               std::to_string(expected_ga_len) + " generic args, but got " + std::to_string(ga_len));
    }

    GenericTable table;
    for (unsigned int i = 0; i < ga_len; i++) {
        auto arg_name = type_entry->generic_args[i]->name->str();
        auto arg_type = params[i];

        table[arg_name] = arg_type;
    }
    return table;
}
LLVMType StructType::llvm_type(GenericTable gen_table) {

    static std::map<std::pair<std::string, Container*>, LLVMType> type_cache;
    auto& mod = get_active_container();
    auto substituted_struct = self->get<StructType>();
    auto name = "struct:" + substituted_struct->get_name();
    if (type_cache.contains({name, nullptr}))
        return type_cache[{name, nullptr}];

    if (!substituted_struct)
        except(E_INTERNAL, "(bug) substituted variant of a struct was found to NOT be a struct");

    if (substituted_struct->members.empty()) {
        return LLVMType{llvm::PointerType::getUnqual(*llctx()), substituted_struct->self};
    }
    std::vector<LLVMType> member_types;
    for (auto member : substituted_struct->ordered_members) {
        auto type = substituted_struct->members[member];
        auto ll_ty = type->llvm_type(gen_table);
        member_types.push_back(ll_ty);
    }

    std::vector<llvm::Type*> conv_mems;
    for (auto m : member_types) {
        conv_mems.push_back(m);
    }

    auto ll_ty = llvm::StructType::create(*llctx(), conv_mems, name);
    LLVMType ret{ll_ty, substituted_struct->self};
    type_cache[{name, nullptr}] = ret;
    return ret;
}

void ParameterizedTypeRef::apply_generic_substitution() {
    auto paw = this->resolves_to->get<ParentAwareType>();
    if (!paw)
        except(E_INTERNAL,
               "you may only apply_generic_substitution ParameterizedTypeRefs which refer to ParentAwareTypes");
    auto type_member = paw->as_member();

    if (type_member->generic_args.size() != params.size())
        except(E_BAD_TYPE,
               "Argument count on ParameterizedTypeRef to " + paw->str() + " does not match the parameter count");

    for (unsigned i = 0; i < type_member->generic_args.size(); i++) {

        type_member->generic_args[i]->temporarily_resolves_to = params[i];
    }
}

void ParameterizedTypeRef::undo_generic_substitution() {

    auto paw = this->resolves_to->get<ParentAwareType>();
    if (!paw)
        except(E_INTERNAL,
               "you may only undo_generic_substitution ParameterizedTypeRefs which refer to ParentAwareTypes");
    auto type_member = paw->as_member();

    if (type_member->generic_args.size() != params.size())
        except(E_BAD_TYPE,
               "Argument count on ParameterizedTypeRef to " + paw->str() + " does not match the parameter count");

    for (unsigned i = 0; i < type_member->generic_args.size(); i++) {

        type_member->generic_args[i]->temporarily_resolves_to.reset();
    }
}

#include "./constant.hh"
ListType::ListType(_Type type, std::unique_ptr<Integer> size) {
    this->of = type->drill()->self;
    this->size = std::move(size);
}

_Type ListType::clone() { return ListType::get(of->clone(), std::make_unique<Integer>(size->value)); }

ListType::ListType(const ListType& from) {
    this->of = from.of;
    this->size = std::make_unique<Integer>(from.size->value);
}

Type* ListType::drill() {
    // return this with a drilled `of` type (prevents bug with casting to
    // generic arrays)
    auto drilled_of = of->drill();
    if (drilled_of == of.get())
        return this;
    auto new_ptr = ListType::get(drilled_of->self, Integer::get(size->value)).get();
    return new_ptr;
}

std::shared_ptr<ListType> ListType::get(_Type of, std::unique_ptr<Integer> size) {
    return create_heaped(ListType(of, std::move(size)));
}

LLVMType ListType::llvm_type(GenericTable gen_table) {
    return {llvm::ArrayType::get(of->llvm_type(gen_table), size->value), self};
}

std::string ListType::str() { return (of ? of->str() : "?") + "[" + size->str() + "]"; }

bool ListType::operator==(Type& against) {
    if (auto k = against.get<ListType>()) {
        return k->of == of && k->size->value == size->value;
    }
    return false;
}

int ListType::distance_from(Type& target) {
    auto lt = target.get<ListType>();
    if (!lt)
        return -1;
    if (lt->size->value != this->size->value)
        return -1;
    return this->of->distance_from(*lt->of);
}

LLVMType DynListType::llvm_type(GenericTable gen_table) {
    auto src_name = "slice:" + this->of->str();
    auto& cont = get_active_container();
    for (auto t : cont.get_mod().getIdentifiedStructTypes()) {
        if (t->getName() == src_name) {
            return {t, self};
        }
    }
    auto struct_ty = llvm::StructType::get(
        *llctx(),
        {
            builder()->getInt64Ty(),                                                // Size of the slice
            llvm::ArrayType::get(this->of->llvm_type(gen_table), 0)->getPointerTo() // Slice Elements
        });
    return {struct_ty, this->self};
}
