# 快速开始指南

## 5分钟上手

### 1. 编译

```bash
make
```

### 2. 测试 fastq_merger

```bash
# 查看帮助
./fastq_merger --help

# 合并两个文件（如果你有测试数据）
./fastq_merger -i file1.fq.gz -i file2.fq.gz -o merged.fq.gz -v
```

### 3. 测试 seq_replacer

```bash
# 查看帮助
./seq_replacer --help

# 随机替换（最快模式）
./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -R 20 -v
```

## 常用命令

### fastq_merger

```bash
# 基本合并
./fastq_merger -i file1.fq.gz -i file2.fq.gz -o merged.fq.gz

# 自定义参数
./fastq_merger -i file1.fq -i file2.fq -o output.fq \
    -p MYINST -r 100 -f FC001 -l 2 -v
```

### seq_replacer

```bash
# 最快模式：随机reads，指定位置
./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -R 20 -v

# 指定reads模式
./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -1 3 10 -v

# 随机模式
./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -r -v

# 全部替换
./seq_replacer -i input.fa -o output.fa -s NNNNNNNN -p 50
```

## 模式选择指南

### seq_replacer 模式

| 需求 | 推荐模式 | 命令 |
|------|---------|------|
| 最快速度 | `-R` | `-R 20` |
| 精确控制 | `-1` | `-1 3 10` |
| 随机测试 | `-r` | `-r` |
| 批量替换 | `-p` | `-p 50` |

## 查看更多

- 完整文档：`README.md`
- 性能指南：`docs/PERFORMANCE.md`
- API 文档：`docs/API.md`
- 项目结构：`PROJECT_STRUCTURE.md`
- 示例脚本：`examples/`
