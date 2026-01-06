# AGENTS.md

## 项目结构

- 所有的开发集中在modules/gdscript2目录下，完成一定的开发工作后应该在本文件同目录下的PROGRESS.md中记录开发进度和剩余工作

## 项目构建

- 使用scons构建项目，开发版二进制固定使用`scons dev_mode=yes dev_build=yes module_mono_enabled=yes scu_build=yes`构建，以复用scons cache。

- 使用`.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --test`运行单元测试
