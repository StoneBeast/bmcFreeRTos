# 基于stm32f103平台的bmc程序

* 目标是在vita4.6标准下实现一个极简的bmc的应用程序。实际上几乎完全不遵守，甚至连ipmi1.5遵守的也不多。
* 该程序基于freertos。
* /linkList目录中实现了一个并不完善的通用链表，可以直接应用到其他项目中。
* /uartConsole目录中实现了基于uart外设的命令行程序，支持注册用户任务以及后台任务，抽离出了硬件操作层，完全可移植，可独立于该程序运行；本项目中途判断需要引入rtos，uartConsole也做出了响应的修改。
* /ipmiTools目录下遵守ipmi协议，简略实现了ipmb request/response的封包/解包以及验证等。多数功能待完善。

## 初次提交可用版本 2025/2/18

各个模块间配合正常，简单测试正常，尚未实现具体的ipmi command或vita command，仅测试实现了ipmb上的 request/reponse以及响应超时等情况。

目前仅有初步的框架雏形。注释尚未添加完全
