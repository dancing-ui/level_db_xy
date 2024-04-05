关于FALLTHROUGH_INTENDED
- 　若是具体的case后面没有实际执行代码，这样子看起来也挺舒服；一旦有了执行代码，没有break的话，则不会立即退出switch，而是继续执行后面的case(这就叫switch结构的fall through现象，额，今天才知道还有这么个高大上的叫法)，看着会不会有点别扭？
- 若是很久以后或是陌生人看到这样的代码，可能先入为主地认为这可能存在bug！
- 但是，有时候是真的需要不break，继续执行后面的case，比如上面LevelDB获取hash值，需要用到键值的每个字节，所以如果剩余字节数是3，取到3之后还得继续取2、1，需要这种fall through现象（当然，没有break就会自然的fall through），于是在这里显示的加上FALLTHROUGH_INTENDED，从名字上也好理解，故意地fall through，相当于加上注释了。

