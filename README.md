# FASTQ/FASTA 处理工具集

这是一套用 C 语言编写的高性能生物信息学工具，用于处理 FASTQ 和 FASTA 格式的测序数据文件。

## 工具列表

### 1. fastq_merger - FASTQ 文件合并工具

合并多个 FASTQ 文件并重新生成符合 Illumina 格式的序列 ID。

**主要功能：**
- 合并多个 FASTQ 文件到单个输出文件
- 重新生成唯一的序列 ID（Illumina 格式）
- 支持 gzip 压缩文件（.gz）的读取和写入
- 流式处理，内存占用低（<100MB）
- 格式验证和错误检测

**使用示例：**

```bash
# 合并两个压缩的 FASTQ 文件
./fastq_merger -i file1.fq.gz -i file2.fq.gz -o merged.fq.gz -v

# 自定义序列 ID 参数
./fastq_merger -i input1.fq -i input2.fq -o output.fq \
    -p MYINST -r 100 -f FC001 -l 2 -v

# 查看帮助信息
./fastq_merger --help
```

**命令行参数：**

必需参数：
- `-i, --input <file>` - 输入 FASTQ 文件（可多次指定）
- `-o, --output <file>` - 输出文件路径

可选参数：
- `-p, --prefix <string>` - 序列 ID 前缀（默认："INSTRUMENT"）
- `-r, --run-id <string>` - 运行编号（默认："1"）
- `-f, --flowcell <string>` - 流动槽 ID（默认："FLOWCELL"）
- `-l, --lane <int>` - 泳道编号（默认：1）
- `-v, --verbose` - 详细输出模式
- `-h, --help` - 显示帮助信息
- `--version` - 显示版本信息

---

### 2. seq_replacer - 序列替换工具

在 FASTA/FASTQ 文件中替换指定的序列片段，支持多种替换模式。

**主要功能：**
- 支持 FASTA 和 FASTQ 格式
- 支持 gzip 压缩文件
- 四种替换模式：
  - 随机模式：随机选择一条 reads，在随机位置替换
  - 随机固定位置模式：随机选择一条 reads，在指定位置替换
  - 指定 reads 模式：指定某条 reads，在指定位置替换
  - 全部替换模式：在所有 reads 的相同位置替换
- 详细的替换日志
- 可重现的随机替换（通过种子）

**使用示例：**

```bash
# 随机模式：随机选择一条 reads，在随机位置替换
./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -r -v

# 随机固定位置模式：随机选择一条 reads，在位置 20 处替换
./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -R 20 -v

# 指定 reads 模式：只替换第 3 条 reads，在位置 10 处替换
./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -1 3 10 -v

# 全部替换模式：在所有 reads 的位置 50 处替换
./seq_replacer -i input.fa -o output.fa -s NNNNNNNN -p 50 -l changes.log

# 可重现的随机替换（使用固定种子）
./seq_replacer -i input.fq -o output.fq -s GCGCGCGC -r --seed 12345

# 查看帮助信息
./seq_replacer --help
```

**命令行参数：**

必需参数：
- `-i, --input <file>` - 输入 FASTA/FASTQ 文件
- `-o, --output <file>` - 输出文件路径
- `-s, --sequence <seq>` - 替换序列

替换模式（选择一种）：
- `-r, --random` - 随机模式：一条随机 reads 在随机位置
- `-R, --random-pos <pos>` - 随机固定位置模式：一条随机 reads 在指定位置
- `-p, --position <pos>` - 全部替换模式：所有 reads 在相同位置
- `-1, --single <n> <pos>` - 指定 reads 模式：第 n 条 reads 在指定位置

可选参数：
- `-l, --log <file>` - 日志文件（默认：replacements.log）
- `--seed <n>` - 随机种子（用于可重现性）
- `-v, --verbose` - 详细输出模式
- `-h, --help` - 显示帮助信息
- `--version` - 显示版本信息

**替换模式对比：**

| 模式 | 选择 reads | 替换位置 | 替换数量 |
|------|-----------|---------|---------|
| `-r` | 随机 | 随机 | 1 条 |
| `-R <pos>` | 随机 | 指定位置 | 1 条 |
| `-1 <n> <pos>` | 指定第 n 条 | 指定位置 | 1 条 |
| `-p <pos>` | 所有 | 指定位置 | 所有 |

---

## 编译和安装

### 系统要求

- GCC 4.8+ 或 Clang 3.5+
- C99 标准支持
- Linux/Unix 系统
- gzip（用于处理压缩文件）

### 编译

```bash
# 编译所有工具
make

# 只编译 fastq_merger
make fastq_merger

# 只编译 seq_replacer
make seq_replacer

# 清理编译产物
make clean
```

### 安装

```bash
# 安装到 /usr/local/bin（需要 root 权限）
sudo make install

# 卸载
sudo make uninstall
```

---

## 文件格式支持

### FASTQ 格式

标准的 4 行格式：
```
@SEQ_ID
GATTTGGGGTTCAAAGCAGTATCG
+
!''*((((***+))%%%++)(%%%
```

### FASTA 格式

标准的序列格式：
```
>SEQ_ID
GATTTGGGGTTCAAAGCAGTATCG
```

### 压缩文件

两个工具都支持 gzip 压缩文件（.gz 扩展名）：
- 自动检测文件扩展名
- 输入和输出都支持压缩
- 流式处理，无需手动解压

---

## 性能特点

### fastq_merger
- **处理速度**：>10MB/s
- **内存使用**：<100MB（无论文件大小）
- **支持文件大小**：无限制（流式处理）

### seq_replacer
- **处理速度**：取决于文件大小和模式
- **内存使用**：<100MB（流式处理）
- **随机模式**：需要两次文件扫描（计数 + 处理）

---

## 日志和输出

### fastq_merger 输出

```
Processing file 1/2: input1.fq.gz
  Completed: 16689514 sequences from 'input1.fq.gz'
Processing file 2/2: input2.fq.gz
  Completed: 16643392 sequences from 'input2.fq.gz'

Merge completed successfully:
  Files processed: 2
  Total sequences: 33332906
  Output file: merged.fq.gz
```

### seq_replacer 日志文件

```
Sequence ID: READ2
Position: 10
Original: GCGCGCGC
Replaced: NNNNNNNN
---
```

---

## 错误处理

两个工具都包含完善的错误处理：

- 文件不存在或无法打开
- 格式错误检测
- 磁盘空间不足
- 参数验证
- 详细的错误消息和位置信息

---

## 示例工作流

### 工作流 1：合并多个测序文件

```bash
# 1. 合并来自不同泳道的数据
./fastq_merger \
    -i lane1.fq.gz \
    -i lane2.fq.gz \
    -i lane3.fq.gz \
    -o merged_all.fq.gz \
    -p MYSEQ -r 20231112 -f FCXXX -v

# 2. 验证输出
zcat merged_all.fq.gz | head -4
```

### 工作流 2：在测序数据中引入变异

```bash
# 1. 在随机位置引入 SNP
./seq_replacer -i original.fq.gz -o mutated.fq.gz \
    -s NNNNNNNN -r -v --seed 42

# 2. 查看替换日志
cat replacements.log

# 3. 在特定位置引入已知变异
./seq_replacer -i original.fq.gz -o variant.fq.gz \
    -s ATCGATCG -1 1000 50 -v -l variant.log
```

---

## 技术细节

### 内存管理
- 使用动态缓冲区，按需扩展
- 及时释放不再使用的内存
- 流式处理避免加载整个文件

### I/O 优化
- 使用缓冲 I/O（8KB 缓冲区）
- 批量写入提高效率
- 通过 popen 处理压缩文件

### 序列 ID 格式（fastq_merger）

生成的 ID 符合 Illumina 格式：
```
@INSTRUMENT:RUN:FLOWCELL:LANE:TILE:X:Y READ:FILTERED:CONTROL:INDEX
```

示例：
```
@MYINST:100:FC001:2:1001:1001:1000 1:N:0:ATCG
```

---

## 故障排除

### 问题：编译错误 "implicit declaration of function 'popen'"

**解决方案**：确保源文件开头有 `#define _POSIX_C_SOURCE 200809L`

### 问题：无法处理压缩文件

**解决方案**：确保系统安装了 gzip：
```bash
# Ubuntu/Debian
sudo apt-get install gzip

# CentOS/RHEL
sudo yum install gzip
```

### 问题：内存不足

**解决方案**：这些工具设计为低内存占用。如果仍然遇到问题，检查：
- 系统可用内存
- 是否有其他进程占用内存
- 文件是否损坏

---

## 许可证

本项目使用 MIT 许可证。

---

## 作者

开发于 2024 年

---

## 更新日志

### v1.0.0 (2024-11-12)
- 初始版本
- fastq_merger：FASTQ 文件合并功能
- seq_replacer：序列替换功能
- 支持 gzip 压缩文件
- 四种替换模式

---

## 贡献

欢迎提交问题报告和功能请求。

---

## 相关资源

- [FASTQ 格式规范](https://en.wikipedia.org/wiki/FASTQ_format)
- [FASTA 格式规范](https://en.wikipedia.org/wiki/FASTA_format)
- [Illumina 序列 ID 格式](https://help.basespace.illumina.com/)
