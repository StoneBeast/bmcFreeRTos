# 基于stm32f103平台的bmc程序

* 目标是在vita4.6标准下实现一个极简的bmc的应用程序。实际上几乎完全不遵守，甚至连ipmi1.5遵守的也不多。
* 该程序基于freertos。
* /linkList目录中实现了一个并不完善的通用链表，可以直接应用到其他项目中。
* /uartConsole目录中实现了基于uart外设的命令行程序，支持注册用户任务以及后台任务，抽离出了硬件操作层，完全可移植，可独立于该程序运行；本项目中途判断需要引入rtos，uartConsole也做出了响应的修改。
* /ipmiTools目录下遵守ipmi协议，简略实现了ipmb request/response的封包/解包以及验证等。多数功能待完善。

## 初次提交可用版本 2025/2/18

各个模块间配合正常，简单测试正常，尚未实现具体的ipmi command或vita command，仅测试实现了ipmb上的 request/reponse以及响应超时等情况。

目前仅有初步的框架雏形。注释尚未添加完全

## 实现一个简单命令 2025/2/20

* 目前整体处于开发中，目前仅实现了**GET DEVICE ID** 这一个IPMI命令，同时添加了 `device` 以及 `info`两个console命令，分别用于扫描可用设别以及产看目标设备信息。
* 修复了因总线其他设备复位而导致的bmc的i2cbusy位被错误置位而导致的总线锁死问题
* 目前可能还存在其他可能导致总线锁死的情况
* 尚未充分测试

## 完成第一个相对可用的版本 2025/3/7

    由于实际情况、功能需求以及能力限制，目前与**通用**、**可移植**的初期目标原来越远。。。。目前只是为了完成功能的无需堆砌以及疯狂的耦合。

    当前版本主要实现了以下功能：

* 实现命令 **get device sdr info**、**get device sdr**两个命令，基于这两条命令实现了**sensor <addr>**这一条console命令。
* 从sdr中提取传感器数值的能力
* 对ipmc的程序做了相应适配。更新了sdr的读取能力(存储在at24cx中)，适配了上述两条ipmi命令。

    接下来的计划：
* 为适应项目需求，将上述ipmc的能力，适配到bmc本机上；为减小工作量，目前考虑不走ipmi接口。
* 整合之前项目中的log记录能力，并考虑是否需要使用`fatFS`。
* bmc本机根据项目需要还需要添加监视其他特有传感器的能力。
* 在实现上述两大功能的基础上，判断是否需要添加接收事件的能力。
