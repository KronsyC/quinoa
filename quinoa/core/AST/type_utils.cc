#include "./type_utils.h"
#include "../../lib/error.h"
std::shared_ptr<Type> TypeUtils::get_common_type(std::shared_ptr<Type> type_1, std::shared_ptr<Type> type_2, bool second_pass)
{
    if (*type_1 == *type_2)
        return type_1;
    if (type_1->get<Primitive>() && type_2->get<Primitive>())
    {
        auto &a = *type_1->get<Primitive>();
        auto &b = *type_2->get<Primitive>();

        // Get the mutual primitive type
        if (a.is(PR_int8) && b.is(PR_int16))
            return type_2;

        if (a.is(PR_int8) && b.is(PR_int32))
            return type_2;
        if (a.is(PR_int16) && b.is(PR_int32))
            return type_2;

        if (a.is(PR_int8) && b.is(PR_int64))
            return type_2;
        if (a.is(PR_int16) && b.is(PR_int64))
            return type_2;
        if (a.is(PR_int32) && b.is(PR_int64))
            return type_2;
    }
    if (type_1->get<ListType>() && type_2->get<ListType>())
    {
        auto pointee_t1 = type_1->pointee();
        auto pointee_t2 = type_2->pointee();

        auto& len_1 = type_1->get<ListType>()->size;
        auto& len_2 = type_2->get<ListType>()->size;

        if(len_1->value == len_2->value) {
            auto mut = get_common_type(pointee_t1, pointee_t2);
            return ListType::get(mut, Integer::get(len_1->value));
        }

    }
    if (type_1->get<Ptr>() && type_2->get<Ptr>())
    {
        auto pointee_t1 = type_1->pointee();
        auto pointee_t2 = type_2->pointee();
        auto mut = get_common_type(pointee_t1, pointee_t2);
        return Ptr::get(mut);
    }
    // if (t1->ptr() && t2->list())
    // {
    //     auto t1p = t1->ptr();
    //     auto t2l = t2->list();
    //     auto mem = getCommonType(t1p->to, t2l->elements);
    //     return new TPtr(mem);
    // }
    if (second_pass)
        except(E_NONEQUIVALENT_TYPES, "Failed to get common type between " + type_1->str() + " and " + type_2->str());
    return get_common_type(type_2, type_1, true);
}
