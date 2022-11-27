#include "./type.hh"



LLVMType get_common_type(LLVMType t1, LLVMType t2, bool repeat){
    if(t1 == t2)return t1;

    auto q1 = t1.qn_type;
    auto q2 = t2.qn_type;

    if(q1->get<Primitive>() && q2->get<Primitive>()){
        Logger::debug("primitives");
        auto p1 = q1->get<Primitive>();
        auto p2 = q2->get<Primitive>();

        #define ord(a, b)if(p1->kind == a && p2->kind == b)return t2;

        ord(PR_int8, PR_int16)
        ord(PR_int8, PR_int32)
        ord(PR_int8, PR_int64)

        ord(PR_int16, PR_int32)
        ord(PR_int16, PR_int64)

        ord(PR_int32, PR_int64)

    }
    if(repeat){
        return get_common_type(t2, t1, false);
    }
    else except(E_NONEQUIVALENT_TYPES, "Failed to get a type common to " + q2->str() + " and " + q1->str());
}
