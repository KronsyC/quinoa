#include "./type.hh"
#include "./container.hh"
#include "./container_member.hh"

LLVMType get_common_type(LLVMType t1, LLVMType t2, bool repeat){
    if(t1 == t2)return t1;

    auto q1 = t1.qn_type;
    auto q2 = t2.qn_type;

    if(q1->get<Primitive>() && q2->get<Primitive>()){
        auto p1 = q1->get<Primitive>();
        auto p2 = q2->get<Primitive>();

        #define ord(a, b)if(p1->kind == PR_##a && p2->kind == PR_##b)return t2;

        ord(int8, int16)
        ord(int8, int32)
        ord(int8, int64)

        ord(int16, int32)
        ord(int16, int64)

        ord(int32, int64)

        ord(uint8, uint16)
        ord(uint8, uint32)
        ord(uint8, uint64)

        ord(uint16, uint32)
        ord(uint16, uint64)

        ord(uint32, uint64)


        ord(uint8, int16)
        ord(uint8, int32)
        ord(uint8, int64)

        ord(uint16, int32)
        ord(uint16, int64)

        ord(uint32, int64)
    }
    if(repeat){
        return get_common_type(t2, t1, false);
    }
    else except(E_NONEQUIVALENT_TYPES, "Failed to get a type common to " + q2->str() + " and " + q1->str());
}

std::string ParentAwareType::get_name(){
    return this->as_member()->name->str();
}

TypeMember* ParentAwareType::as_member(){
    for(auto mem : this->parent->members){
        if(auto ty = dynamic_cast<TypeMember*>(mem.ptr)){
            if(ty->refers_to.get() ==  (self_ptr ? self_ptr : this))return ty;
        }
    }
    except(E_INTERNAL, "ParentAwareType::as_member failed");
}

GenericTable ParameterizedTypeRef::get_mapped_params(){
    // resolves_to has to be a ParentAwareType
    auto paw = dynamic_cast<ParentAwareType*>(resolves_to->drill());
    if(!paw)except(E_BAD_TYPE, "A Parameterized TypeRef must resolve to a ParentAwareType, but was found to resolve to " + resolves_to->str());

    auto type_entry = paw->as_member();

    auto expected_ga_len = type_entry->generic_args.size();
    auto ga_len = this->params.size();

    if(ga_len != expected_ga_len){
        except(E_BAD_TYPE, "Error while constructing Parameterized Type Ref to " + paw->str() + "\n\t\texpected " + std::to_string(expected_ga_len) + " generic args, but got " + std::to_string(ga_len));
    }

    GenericTable table;
    for(unsigned int i = 0; i < ga_len; i++){
        auto arg_name = type_entry->generic_args[i]->name->str();
        auto arg_type = params[i];

        table[arg_name] = arg_type;
    }
    return table;
}


bool StructType::is_generic() {
    for(auto m : this->members){
        for(auto t : m.second->flatten()){
            if(auto gen = dynamic_cast<Generic*>(t)){
                return true;
            }
        }
    }
    return false;
}

static std::map<std::vector<llvm::Type*>, LLVMType> struct_cache;
LLVMType StructType::llvm_type(GenericTable gen_table) {
    std::vector < llvm::Type * > member_types;
    for (auto member: members) {
        member_types.push_back(member.second->llvm_type(gen_table));
    }
    if(auto st = struct_cache[member_types])return st;

    auto ll_ty = llvm::StructType::create(*llctx(), member_types, "struct:"+this->get_name());

    if(this->is_generic()){
        std::map <std::string, std::shared_ptr<Type>> new_members;

        for(auto pair : members){
            auto name = pair.first;
            auto ty = pair.second;

            if(auto gen = ty->get<Generic>()){
                if(auto nt = gen_table[gen->name->str()]){
                    new_members[name] = nt->drill()->self;
                    continue;
                }

            }
            new_members[name] = ty->drill()->self;
        }

        auto new_t = StructType::get(new_members, this->parent);
        new_t->self_ptr = this;
        if(new_t->is_generic()){
            except(E_INTERNAL, "newly monomorphized struct is still generic");
        }
        Logger::debug("Created monomorph type: " + new_t->str() + ", ref count: " + std::to_string(new_t.use_count()));
        return new_t->llvm_type(gen_table);
    }
    LLVMType ret{ll_ty, self};
    struct_cache[member_types] = ret;
    return ret;
}
