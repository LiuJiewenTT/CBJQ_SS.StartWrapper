---
LANG: zh_CN.UTF-8
date: 2024-06-07 15:51:00 +0800
permalink: /
redirect_from:
  - /README/
---

# CBJQ_SS StartWrapper

《尘白禁区切服器》辅助启动程序

尘白禁区切服器的辅助启动程序项目。

原本，这个程序最重要的目标，就是让没有提权的情况，可以自动发起提权申请，这样就可以保持调用发起程序不用使用管理员权限运行了。
不记得从什么时候起，为了兼容国际服，这个辅助程序成了一种必要。在2024年末的今天，需要使用这个辅助程序来启动才能使得`preference.json`文件不冲突且不出现在链接的目标目录中。

开源许可协议：*MIT License*

**快速跳转:** (暂无)

## 使用

```
CBJQ_SS.StartWrapper.exe ProgramToStart.exe
```

## 配置

若存在同名+".test"文件，则视为测试状态。测试状态下，不对提权情况做要求，直接正常结束。
