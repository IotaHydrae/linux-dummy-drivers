
# static_call

`static_call` 是 Linux 内核近几年（大约从 5.10 开始成熟）引入的一种运行时函数替换（Runtime Function Patching）机制，其目的是消除函数指针调用的开销。

它可以理解成：

函数指针的灵活性 + 普通函数调用的性能。

对于现代 Linux（x86、arm64 等）来说，它已经大量用于调度器、KVM、Trace、Security、BPF 等子系统。

**为什么需要 static_call？**

假设有这样一段代码：

```c
struct ops {
    void (*foo)(void);
};

extern struct ops my_ops;

void test(void)
{
    my_ops.foo();
}
```

编译后的汇编大概是：

```
ldr x0, [my_ops]
ldr x1, [x0]
blr x1
```

这里有几个问题：

每次都需要读取函数指针
blr 是间接跳转（Indirect Branch）
CPU 很难预测
Spectre 后还有 retpoline 等额外开销

一次函数指针调用可能要比普通 call 慢很多。

**static_call 的原理**

启动时先编译成普通 call，运行时再修改 call 的目标地址。

**static_call 与函数指针对比**

| 特性           | 函数指针    | static_call            |
| ------------ | ------- | ---------------------- |
| 可运行时修改       | ✅       | ✅                      |
| 调用成本         | 高（间接调用） | 接近普通函数调用               |
| CPU 分支预测     | 差       | 好                      |
| Retpoline 开销 | 有（部分架构） | 无（直接调用）                |
| 修改目标         | 修改指针值   | 修改机器码中的 `call`/`BL` 目标 |
| 适用场景         | 频繁变化的回调 | 初始化后很少变化的热点路径          |


**Linux 中的典型应用**

static_call 特别适合调用频繁但实现很少变化的场景，因此被广泛用于：

- 调度器（Scheduler）：根据不同调度策略切换实现。
- KVM：根据 CPU 是否支持某些虚拟化特性，选择不同的 VM Exit 处理函数。
- x86 CPU 特性：根据硬件能力（如 IBRS、Retbleed 缓解等）选择最优实现。
- 安全框架：某些 LSM（Linux Security Modules）钩子。
- Tracing / Perf：启用或禁用特定追踪逻辑时，避免热点路径上的函数指针开销。

这些场景都有一个共同特点：**运行期间调用次数极多，而目标函数几乎不会改变**。 在这种情况下，`static_call` 能将间接调用优化为直接调用，在保持可配置性的同时获得接近普通函数调用的性能。

## 用法

```c
#include <linux/static_call.h>

static int func_a(int x)
{
	pr_info("static_call -> %s, x : %d\n", __func__, x);

	return x;
}

static int func_b(int x)
{
	x += 10;

	pr_info("static_call -> %s, x + 10 : %d\n", __func__, x);

	return x;
}

DEFINE_STATIC_CALL(dummy_static_call, func_a);


static_call(dummy_static_call)(10);

static_call_update(dummy_static_call, func_b);

ret = static_call(dummy_static_call)(10);
pr_info("%s, static_call -> func_b ret = %d\n", __func__, ret);
```

函数可以带参数，可以有返回值，但对于同一个  DEFINE_STATIC_CALL `dummy_static_call`，func_a 与
需要更新的 func_b 必须具有完全一致的函数原型（参数个数、参数类型、返回值都一致）。

下面是 static_call 的两种实现方式：

### trampoline（默认）

调用流程：

```
call __SCT__dummy_static_call
           │
           ▼
    jmp func_a
           │
           ▼
       func_a()
           │
           ▼
          ret
```

- 直接跳转（Direct Jump）
- 目标固定，CPU 分支预测几乎 100% 命中
- 没有函数指针带来的 indirect branch
- 不需要 retpoline

调用 `static_call_update(dummy_static_call, func_b);` 后，只是将内存中代码的text段__SCT__dummy_static_call函数的 `jmp func_a` 修改为 `jmp func_b`

相比inline方式多一条jmp指令，但优点是update时只修改jmp指令，而inline方式由于直接call目标函数，需要对每一处调用都进行修改。

__SCT__dummy_static_call 是个预留值，模块加载时（apply_relocate_add() → static_call_mod_init()）根据 .static_call_sites 将这些调用点重写为真正的 call func_a（如果 CONFIG_HAVE_STATIC_CALL_INLINE=y）。 后续 static_call_update() 再利用这些 site 信息，把所有 call func_a 改成 call func_b。

-----------------------------------

编译后，每一个 static_call() 都会额外生成一条元数据，放进一个特殊的 ELF Section `static_call_sites`：
```bash
# readelf -a dummy-static-call.ko

Relocation section '.rela.static_call_sites'

Offset          Info      Type             Sym.Value    Sym.Name + Addend

0000            ...       R_X86_64_PC32    .init.text + 8d
0004            ...       R_X86_64_PC32    __SCK__dummy_static_call

0008            ...       R_X86_64_PC32    .init.text + ac
000c            ...       R_X86_64_PC32    __SCK__dummy_static_call
```

```bash
# objdump -Sdr dummy-static-call.o

0000000000000000 <__SCT__dummy_static_call>:
   0:   e9 00 00 00 00          jmp    5 <__SCT__dummy_static_call+0x5>
                        1: R_X86_64_PC32        .text+0xc
   5:   0f b9 cc                ud1    %esp,%ecx

Disassembly of section .init.text:

0000000000000000 <__pfx_init_module>:
   0:   90                      nop
   1:   90                      nop
   2:   90                      nop
   3:   90                      nop
   4:   90                      nop
   5:   90                      nop
   6:   90                      nop
   7:   90                      nop
   8:   90                      nop
   9:   90                      nop
   a:   90                      nop
   b:   90                      nop
   c:   90                      nop
   d:   90                      nop
   e:   90                      nop
   f:   90                      nop

0000000000000010 <init_module>:
  10:   f3 0f 1e fa             endbr64
  14:   e8 00 00 00 00          call   19 <init_module+0x9>
                        15: R_X86_64_PLT32      __fentry__-0x4
  19:   55                      push   %rbp
        printk("%s\n", __func__);
  1a:   48 c7 c6 00 00 00 00    mov    $0x0,%rsi
                        1d: R_X86_64_32S        .rodata+0x30
  21:   48 c7 c7 00 00 00 00    mov    $0x0,%rdi
                        24: R_X86_64_32S        .rodata.str1.1+0x38
module_driver(dummy_static_call, dummy_static_call_drv_register,
  28:   48 89 e5                mov    %rsp,%rbp
        printk("%s\n", __func__);
  2b:   e8 00 00 00 00          call   30 <init_module+0x20>
                        2c: R_X86_64_PLT32      _printk-0x4
        ret = alloc_chrdev_region(&dummy_static_call->dev_num, 0, 1,
  30:   48 c7 c1 00 00 00 00    mov    $0x0,%rcx
                        33: R_X86_64_32S        .rodata.str1.1+0x3c
  37:   31 f6                   xor    %esi,%esi
  39:   ba 01 00 00 00          mov    $0x1,%edx
  3e:   48 c7 c7 00 00 00 00    mov    $0x0,%rdi
                        41: R_X86_64_32S        .bss
  45:   e8 00 00 00 00          call   4a <init_module+0x3a>
                        46: R_X86_64_PLT32      alloc_chrdev_region-0x4
        dummy_static_call->class = class_create("dummy-static_call-class");
  4a:   48 c7 c7 00 00 00 00    mov    $0x0,%rdi
                        4d: R_X86_64_32S        .rodata.str1.1+0x4e
  51:   e8 00 00 00 00          call   56 <init_module+0x46>
                        52: R_X86_64_PLT32      class_create-0x4
  56:   48 89 05 00 00 00 00    mov    %rax,0x0(%rip)        # 5d <init_module+0x4d>
                        59: R_X86_64_PC32       .bss+0x4
        if (IS_ERR(dummy_static_call->class)) {
  5d:   48 3d 00 f0 ff ff       cmp    $0xfffffffffffff000,%rax
  63:   77 67                   ja     cc <init_module+0xbc>
        dummy_static_call->dev = device_create(dummy_static_call->class, NULL,
  65:   8b 15 00 00 00 00       mov    0x0(%rip),%edx        # 6b <init_module+0x5b>
                        67: R_X86_64_PC32       .bss-0x4
  6b:   48 89 c7                mov    %rax,%rdi
  6e:   31 c9                   xor    %ecx,%ecx
  70:   31 f6                   xor    %esi,%esi
  72:   49 c7 c0 00 00 00 00    mov    $0x0,%r8
                        75: R_X86_64_32S        .rodata.str1.1+0x84
  79:   e8 00 00 00 00          call   7e <init_module+0x6e>
                        7a: R_X86_64_PLT32      device_create-0x4
  7e:   48 89 05 00 00 00 00    mov    %rax,0x0(%rip)        # 85 <init_module+0x75>
                        81: R_X86_64_PC32       .bss+0xc
        if (IS_ERR(dummy_static_call->dev)) {
  85:   48 3d 00 f0 ff ff       cmp    $0xfffffffffffff000,%rax
  8b:   77 6d                   ja     fa <init_module+0xea>
        static_call(dummy_static_call)();
  8d:   e8 00 00 00 00          call   92 <init_module+0x82>
                        8e: R_X86_64_PLT32      __SCT__dummy_static_call-0x4
        static_call_update(dummy_static_call, func_b);
  92:   48 c7 c2 00 00 00 00    mov    $0x0,%rdx
                        95: R_X86_64_32S        .text+0x90
  99:   48 c7 c6 00 00 00 00    mov    $0x0,%rsi
                        9c: R_X86_64_32S        __SCT__dummy_static_call
  a0:   48 c7 c7 00 00 00 00    mov    $0x0,%rdi
                        a3: R_X86_64_32S        __SCK__dummy_static_call
  a7:   e8 00 00 00 00          call   ac <init_module+0x9c>
                        a8: R_X86_64_PLT32      __static_call_update-0x4
        static_call(dummy_static_call)();
  ac:   e8 00 00 00 00          call   b1 <init_module+0xa1>
                        ad: R_X86_64_PLT32      __SCT__dummy_static_call-0x4
```

`.init.text + 8d` 和 `.init.text + ac`
```bash
  8d:   e8 00 00 00 00          call   92 <init_module+0x82>
                        8e: R_X86_64_PLT32      __SCT__dummy_static_call-0x4

  ac:   e8 00 00 00 00          call   b1 <init_module+0xa1>
                        ad: R_X86_64_PLT32      __SCT__dummy_static_call-0x4
```

因为 x86 PC-relative relocation，call/jmp rel32 计算方式为
```
target - next_ip
```

所以 __SCT__dummy_static_call 中的 `jmp .text + 0xc` 就等价于 `jmp .text + 0x10` 也就是 func_a 被重定位时的地址。

反汇编中 static_call_update(dummy_static_call, func_b); 的调用

```
92: mov $func_b, %rdx
99: mov $__SCT__dummy_static_call, %rsi
a0: mov $__SCK__dummy_static_call, %rdi
a7: call __static_call_update
```

```
__static_call_update(
    &__SCK__dummy_static_call,
    __SCT__dummy_static_call,
    func_b);
```

update之后，内存中代码jmp func_a(重定位之后的地址) 被替换为 func_b

### inline

...
