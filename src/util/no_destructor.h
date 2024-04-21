#ifndef _LEVEL_DB_XY_NO_DESTRUCTOR_H_
#define _LEVEL_DB_XY_NO_DESTRUCTOR_H_

#include <utility>

namespace ns_no_destructor {

template<typename InstanceType>
class NoDestructor {
public:
    template<typename... ConstructorArgTypes>
    explicit NoDestructor(ConstructorArgTypes&&... constructor_args) {
        static_assert(sizeof(instance_storage_) >= sizeof(InstanceType), "instance_storage_ is not large enough to hold the instance");
        static_assert(alignof(decltype(instance_storage_)) >= alignof(InstanceType), "instance_storage_ does not meet the instance's alignment requirement");
        new (&instance_storage_) InstanceType(std::forward<ConstructorArgTypes>(constructor_args)...);
    }

    ~NoDestructor() = default;

    NoDestructor(NoDestructor const&) = delete;
    NoDestructor& operator=(NoDestructor const&) = delete;

    InstanceType* get() {
        return reinterpret_cast<InstanceType*>(&instance_storage_);
    }

private:
    typename std::aligned_storage<sizeof(InstanceType), alignof(InstanceType)>::type instance_storage_;
};

} //  ns_no_destructor


#endif