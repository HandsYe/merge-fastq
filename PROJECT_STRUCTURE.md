# 项目结构

```
merge-fastq/
├── README.md                    # 主文档
├── PROJECT_STRUCTURE.md         # 本文件
├── Makefile                     # 编译配置
├── .gitignore                   # Git 忽略文件
│
├── docs/                        # 文档目录
│   ├── API.md                   # API 文档
│   └── PERFORMANCE.md           # 性能指南
│
├── examples/                    # 示例脚本
│   ├── merge_example.sh         # fastq_merger 示例
│   └── replace_example.sh       # seq_replacer 示例
│
├── 源代码文件
│   ├── main.c                   # fastq_merger 主程序
│   ├── seq_replace_main.c       # seq_replacer 主程序
│   │
│   ├── fastq_parser.c/h         # FASTQ 解析器
│   ├── id_generator.c/h         # ID 生成器
│   ├── file_merger.c/h          # 文件合并模块
│   ├── seq_replacer.c/h         # 序列替换模块
│   └── utils.c/h                # 工具函数
│
└── 编译产物（.gitignore）
    ├── fastq_merger             # 可执行文件
    ├── seq_replacer             # 可执行文件
    └── *.o                      # 目标文件
```

## 模块依赖关系

```
fastq_merger:
    main.c
    ├── fastq_parser.c/h
    ├── id_generator.c/h
    ├── file_merger.c/h
    │   ├── fastq_parser.c/h
    │   ├── id_generator.c/h
    │   └── utils.c/h
    └── utils.c/h

seq_replacer:
    seq_replace_main.c
    ├── seq_replacer.c/h
    │   ├── fastq_parser.c/h
    │   └── utils.c/h
    ├── fastq_parser.c/h
    └── utils.c/h
```

## 核心模块说明

### 1. fastq_parser（FASTQ 解析器）
- **功能**：读取和验证 FASTQ/FASTA 文件
- **支持**：普通文件和 gzip 压缩文件
- **特点**：流式处理，低内存占用

### 2. id_generator（ID 生成器）
- **功能**：生成 Illumina 格式的序列 ID
- **格式**：`@INSTRUMENT:RUN:FLOWCELL:LANE:TILE:X:Y READ:FILTERED:CONTROL:INDEX`
- **特点**：可配置参数，自动递增坐标

### 3. file_merger（文件合并）
- **功能**：合并多个 FASTQ 文件
- **特点**：重新生成 ID，格式验证，进度显示

### 4. seq_replacer（序列替换）
- **功能**：在 FASTA/FASTQ 文件中替换序列
- **模式**：4种替换模式（随机、随机固定、指定、全部）
- **特点**：快速计数，日志记录

### 5. utils（工具函数）
- **功能**：内存管理、字符串处理、错误处理
- **特点**：安全的内存分配，统一的错误处理

## 编译流程

```
源文件 (.c) → 编译 → 目标文件 (.o) → 链接 → 可执行文件
```

### fastq_merger 编译
```bash
gcc -c main.c fastq_parser.c id_generator.c file_merger.c utils.c
gcc -o fastq_merger main.o fastq_parser.o id_generator.o file_merger.o utils.o
```

### seq_replacer 编译
```bash
gcc -c seq_replace_main.c seq_replacer.c fastq_parser.c utils.c
gcc -o seq_replacer seq_replace_main.o seq_replacer.o fastq_parser.o utils.o
```

## 数据流

### fastq_merger 数据流
```
输入文件1.fq.gz ─┐
输入文件2.fq.gz ─┼─→ fastq_parser → 验证 → id_generator → file_merger → 输出.fq.gz
输入文件N.fq.gz ─┘
```

### seq_replacer 数据流
```
输入.fq.gz → 计数（wc/grep）→ 随机选择 → fastq_parser → 替换 → 输出.fq.gz
                                                              ↓
                                                         日志文件
```

## 文件大小

| 文件 | 行数 | 说明 |
|------|------|------|
| main.c | ~200 | fastq_merger 主程序 |
| seq_replace_main.c | ~250 | seq_replacer 主程序 |
| fastq_parser.c | ~250 | FASTQ 解析实现 |
| id_generator.c | ~120 | ID 生成实现 |
| file_merger.c | ~200 | 文件合并实现 |
| seq_replacer.c | ~500 | 序列替换实现 |
| utils.c | ~80 | 工具函数实现 |

**总计**：约 1600 行 C 代码

## 性能特点

| 工具 | 内存使用 | 处理速度 | 文件大小限制 |
|------|---------|---------|-------------|
| fastq_merger | <100MB | ~17MB/s | 无限制 |
| seq_replacer | <100MB | ~30秒/2GB | 无限制 |

## 开发指南

### 添加新功能

1. **新的文件格式支持**
   - 修改 `fastq_parser.c`
   - 添加格式检测和读取函数

2. **新的替换模式**
   - 修改 `seq_replacer.h`（添加枚举）
   - 修改 `seq_replacer.c`（实现逻辑）
   - 修改 `seq_replace_main.c`（添加命令行选项）

3. **性能优化**
   - 调整缓冲区大小（`WRITE_BUFFER_SIZE`）
   - 实现多线程处理
   - 使用内存映射（mmap）

### 测试

```bash
# 编译
make clean && make

# 测试 fastq_merger
./fastq_merger --help
./fastq_merger -i test1.fq -i test2.fq -o output.fq -v

# 测试 seq_replacer
./seq_replacer --help
./seq_replacer -i test.fq -o output.fq -s NNNNNNNN -R 10 -v
```

### 调试

```bash
# 使用 gdb
gcc -g -o fastq_merger main.c fastq_parser.c id_generator.c file_merger.c utils.c
gdb ./fastq_merger

# 内存检查
valgrind --leak-check=full ./fastq_merger -i test.fq -o output.fq
```

## 许可证

MIT License

## 版本历史

- v1.0.0 (2025-11-12): 初始版本
  - fastq_merger: FASTQ 文件合并
  - seq_replacer: 序列替换（4种模式）
  - 支持 gzip 压缩文件
  - 完整文档和示例
