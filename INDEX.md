# 文档索引

## 快速导航

### 🚀 新手入门
- [快速开始](QUICKSTART.md) - 5分钟上手指南
- [README](README.md) - 完整功能介绍

### 📚 详细文档
- [项目结构](PROJECT_STRUCTURE.md) - 代码组织和模块说明
- [性能指南](docs/PERFORMANCE.md) - 性能优化和基准测试
- [API 文档](docs/API.md) - 开发者参考

### 💡 示例代码
- [fastq_merger 示例](examples/merge_example.sh)
- [seq_replacer 示例](examples/replace_example.sh)

## 工具概览

### fastq_merger
**功能**：合并多个 FASTQ 文件并重新生成序列 ID

**特点**：
- ✅ 支持 gzip 压缩文件
- ✅ 流式处理，低内存占用（<100MB）
- ✅ 自动生成 Illumina 格式 ID
- ✅ 格式验证和错误检测

**快速使用**：
```bash
./fastq_merger -i file1.fq.gz -i file2.fq.gz -o merged.fq.gz -v
```

### seq_replacer
**功能**：在 FASTA/FASTQ 文件中替换序列片段

**特点**：
- ✅ 4种替换模式（随机、随机固定、指定、全部）
- ✅ 支持 FASTA 和 FASTQ 格式
- ✅ 支持 gzip 压缩文件
- ✅ 详细的替换日志
- ✅ 可重现的随机替换

**快速使用**：
```bash
# 最快模式
./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -R 20 -v
```

## 文档结构

```
├── INDEX.md                    # 本文件
├── QUICKSTART.md               # 快速开始
├── README.md                   # 主文档
├── PROJECT_STRUCTURE.md        # 项目结构
│
├── docs/
│   ├── API.md                  # API 参考
│   └── PERFORMANCE.md          # 性能指南
│
└── examples/
    ├── merge_example.sh        # 合并示例
    └── replace_example.sh      # 替换示例
```

## 常见任务

### 编译和安装
```bash
# 编译
make

# 安装（需要 root）
sudo make install

# 卸载
sudo make uninstall
```

### 合并 FASTQ 文件
```bash
./fastq_merger -i file1.fq.gz -i file2.fq.gz -o merged.fq.gz -v
```

### 替换序列
```bash
# 随机选择一条reads，在位置20处替换（最快）
./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -R 20 -v
```

### 查看帮助
```bash
./fastq_merger --help
./seq_replacer --help
```

## 性能参考

| 工具 | 文件大小 | 处理时间 | 内存使用 |
|------|---------|---------|---------|
| fastq_merger | 2.1GB × 2 | ~4分钟 | <100MB |
| seq_replacer (-R) | 2.1GB | ~30秒 | <100MB |

## 获取帮助

1. **查看文档**：从 [README.md](README.md) 开始
2. **运行示例**：查看 `examples/` 目录
3. **查看帮助**：使用 `--help` 选项
4. **性能问题**：参考 [性能指南](docs/PERFORMANCE.md)
5. **开发问题**：参考 [API 文档](docs/API.md)

## 版本信息

- **当前版本**：v1.0.0
- **发布日期**：2025-11-12
- **许可证**：MIT

## 更新日志

### v1.0.0 (2025-11-12)
- ✨ 初始版本发布
- ✨ fastq_merger：FASTQ 文件合并功能
- ✨ seq_replacer：序列替换功能（4种模式）
- ✨ 支持 gzip 压缩文件
- ✨ 完整文档和示例
- ⚡ 优化性能：使用快速计数算法
