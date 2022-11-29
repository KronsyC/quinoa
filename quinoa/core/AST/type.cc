#include "./type.hh"
#include "./container.hh"
#include "./container_member.hh"

LLVMType get_common_type(LLVMType t1, LLVMType t2, bool repeat){
    if(t1 == t2)return t1;

    auto q1 = t1.qn_type;
    auto q2 = t2.qn_type;

    if(q1->get<Primitive>() && q2->get<Primitive>()){
        Logger::debug("primitives");
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
    for(auto mem : this->parent->members){
        if(auto ty = dynamic_cast<TypeMember*>(mem.ptr)){
            if(ty->refers_to.get() == this){
                return ty->name->str();
            }
        }
    }
    return "unknown";
}

