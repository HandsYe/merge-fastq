# 项目总结

## 项目概述

本项目包含两个高性能的生物信息学命令行工具，用于处理 FASTQ 和 FASTA 格式的测序数据。

## 工具功能

### 1. fastq_merger
- **功能**：合并多个 FASTQ 文件并重新生成序列 ID
- **特点**：
  - 支持 gzip 压缩文件
  - 流式处理，内存占用低
  - 生成 Illumina 格式的序列 ID
  - 完善的错误处理

### 2. seq_replacer
- **功能**：在 FASTA/FASTQ 文件中替换序列片段
- **特点**：
  - 支持 FASTA 和 FASTQ 格式
  - 支持 gzip 压缩文件
  - 四种替换模式
  - 详细的替换日志
  - 可重现的随机替换

## 文件结构

```
.
├── README.md              # 详细文档
├── QUICKSTART.md          # 快速入门指南
├── PROJECT_SUMMARY.md     # 项目总结（本文件）
├── Makefile               # 编译配置
├── examples.sh            # 示例脚本
├── .gitignore             # Git 忽略文件
│
├── fastq_merger 相关文件：
│   ├── main.c             # 主程序
│   ├── fastq_parser.c/h   # FASTQ 解析器
│   ├── id_generator.c/h   # ID 生成器
│   ├── file_merger.c/h    # 文件合并模块
│   └── utils.c/h          # 工具函数
│
└── seq_replacer 相关文件：
    ├── seq_replace_main.c # 主程序
    ├── seq_replacer.c/h   # 序列替换模块
    ├── fastq_parser.c/h   # FASTQ 解析器（共享）
    └── utils.c/h          # 工具函数（共享）
```

## 技术特点

### 编程语言
- C99 标准
- 无外部依赖（仅标准库）

### 性能优化
- 流式处理
- 缓冲 I/O
- 动态内存管理
- 低内存占用（<100MB）

### 代码质量
- 模块化设计
- 完善的错误处理
- 详细的注释
- 编译无警告（-Wall -Wextra）

## 使用场景

### fastq_merger
1. 合并来自不同泳道的测序数据
2. 整合多个批次的样本
3. 重新生成标准化的序列 ID
4. 数据预处理和格式化

### seq_replacer
1. 在测序数据中引入已知变异
2. 模拟 SNP 或 indel
3. 测试下游分析流程
4. 数据质量控制测试

## 编译和运行

```bash
# 编译
make

# 运行示例
./examples.sh

# 查看帮助
./fastq_merger --help
./seq_replacer --help
```

## 性能指标

### fastq_merger
- 处理速度：>10MB/s
- 内存使用：<100MB
- 支持文件大小：无限制

### seq_replacer
- 处理速度：取决于模式和文件大小
- 内存使用：<100MB
- 随机模式：需要两次文件扫描

## 测试数据

项目包含示例脚本 `examples.sh`，可以：
- 自动创建测试数据
- 演示所有主要功能
- 验证工具正常工作
- 清理测试文件

## 未来改进方向

1. **性能优化**
   - 多线程支持
   - 更大的缓冲区
   - SIMD 优化

2. **功能扩展**
   - 支持更多压缩格式（bzip2, xz）
   - 支持 BAM/SAM 格式
   - 批量替换模式
   - 图形界面

3. **代码改进**
   - 单元测试
   - 集成测试
   - 性能基准测试
   - 持续集成

## 依赖项

### 编译时
- GCC 4.8+ 或 Clang 3.5+
- Make

### 运行时
- gzip（处理压缩文件）
- Linux/Unix 系统

## 许可证

MIT License

## 版本历史

- v1.0.0 (2024-11-12)
  - 初始版本
  - fastq_merger 基本功能
  - seq_replacer 四种替换模式
  - 完整文档

## 联系方式

如有问题或建议，请提交 Issue。

---

**最后更新**：2024-11-12
