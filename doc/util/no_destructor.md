# 基本概念
1. NoDestructor是一个wrapper，它创建一个静态区存储的对象，这个对象只在第一次访问时执行构造函数一次，永不执行析构函数，看名字NoDestructor也能知道这一点。基本上声明为static的全局对象，也不需要析构，内存随进程退出而释放，不需要做收尾工作，如果需要做，那就单独声明一个Uninitialize()函数，在进程退出的地方显式调用。
2. NoDestructor在内部存储着真正的对象指针，所以避免了间接的指针，就是说，需要这个对象指针时，不用去拿以前得到的指针间接使用，直接用NoDestructor就能拿到原始指针，并且**用NoDestructor也就不会用到malloc，好处就是不是在heap申请的内存，是放在全局静态区的**。c++11规定static local变量的初始化是线程安全的，所以static的NoDestructor初始化也是线程安全的。
3. 因为析构函数不会被调用，所以当声明的对象在stack里，或者是成员变量，当退出当前stack，或所在class对象销毁时，NoDestructor的对象不会释放内存，就会有内存泄漏！！！！
4. 所以，用NoDestructor时，最好只创建静态对象！！！

## 为什么静态对象要禁用析构函数？

1. **生命周期管理**：静态对象的生命周期与程序的生命周期相同，它们在程序启动时创建，直到程序结束时才销毁。因此，静态对象的析构函数不会在正常的对象生命周期结束时被调用。相反，它们会在程序退出时自动销毁。禁用析构函数可以防止在对象销毁时执行意外的清理操作或资源释放。
2. **顺序问题**：程序中的静态对象的创建和销毁顺序是不确定的。不同编译单元中的静态对象的初始化顺序是未定义的。如果静态对象具有相互依赖关系，析构函数的调用顺序可能会导致问题。为了避免这种顺序问题，禁用静态对象的析构函数是一种可行的解决方法。

---
leveldb提供了byte级别的比较器，见util/comparator.cc，这个文件中提供的BytewiseComparator类对象通过无析构函数的单例提供，代码如下：
```C++
const Comparator* BytewiseComparator() {
    static NoDestructor<BytewiseComparatorImpl> singleton;
    return singleton.get();
}
```
这里单例通过static语义实现，保证第一个到达此函数control的thread初始化对象且无竞争，这里主要分析NoDesturctor类的实现，源码在util/no_destructor.h中提供，代码很少，只有短短20行，但是包含很多知识：
```C++
template <typename InstanceType>
class NoDestructor {
public:
    template <typename... ConstructorArgTypes>
    explicit NoDestructor(ConstructorArgTypes && ... constructor_args) {
        static_assert(sizeof(instance_storage_) >= sizeof(InstanceType),
                "instance_storage_ is not large enough to hold the instance");
        static_assert(
                alignof(decltype(instance_storage_)) >= alignof(InstanceType),
                "instance_storage_ does not meet the instance's alignment requirement");
        new(&instance_storage_) InstanceType(std::forward<ConstructorArgTypes>(constructor_args)...);
    }
 
    ~NoDestructor() = default;
 
    NoDestructor(const NoDestructor&) = delete;
    NoDestructor& operator=(const NoDestructor&) = delete;
 
    InstanceType* get() {
        return reinterpret_cast<InstanceType*>(&instance_storage_);
    }
 
private:
    typename std::aligned_storage<sizeof(InstanceType), alignof(InstanceType)>::type instance_storage_;
};
```
这里针对这段代码中值得注意的知识做一个总结：

1、std::forward完美转发

forward的作用是完美转发参数，若函数传入左值，那么转发到其他函数时仍保留左值属性；若函数传入右值，那么转发到其他函数时仍保留右值属性，forward通用范式如下：
```C++
template <typename T>
void foo(T&& value) {
    bar(std::forward<T>(value));
}
```
编译器通过判断&的组合方式决定变量是左值还是右值，匹配规则如下：
```C++
& & -> &

&& & -> &

& && -> &

&& && -> &&
```
例如如下代码：

std::string str;
foo(str);
编译器推理T是const string&，参数类型是& &&，因此得到一个左值。从上面可以看到编译器在调用std::forward的过程中做了大量工作，匹配规则也很复杂，但是使用起来是很简单的，目前笔者见到的std::forward的使用都是按照标准范式来的，因此推荐按照标准范式使用

2、static_assert 静态期断言

static_assert用于编译期检查，若比较无法通过，那么编译期报错。静态期语义决定了static_assert只能使用编译器可见的常数

3、alignof 返回对齐地址字节数，遵循标准的字节对齐数，比如，plain old 数据类型对齐大小就是本身，结构和联合的本身看最大成员对齐数，使用pragma pack可以指定对齐子节数，例子如下：
```C++
#pragma pack(1)
class Test {
private:
    int i;
};
#pragma pack()
 
void func() {
    cout << alignof(Test) << endl;    // 这一行输出为1
}
```
4、std::aligned_storage 对齐存储，通过提供一个数据类型和该类型对应的对齐字节数，获取一个对齐大小的存储，通过type类型成员声明，例子如下：
```C++
void func() {
    std::aligned_storage<3, 2>::type a;
    cout << sizeof(a) << endl;
}
```
这里原始类型大小为3字节，按照2字节对齐，因此得到一个4字节大小的存储，注意这里a的类型不是原始类型，而是std::aligned_storage<T1, T2>::type类型，使用时需要用原始类型强制转换

5、定位new

定位new在指定位置分配内存，一般用于栈空间分配内存，定位new的一个特点是无法显式delete，因此**通过定位new分配出来的对象没有调用析构函数的机会**，这也是NoDesturctor类不会调用析构函数的核心实现