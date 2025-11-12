# 快速入门指南

## 1. 编译工具

```bash
make
```

编译成功后会生成两个可执行文件：
- `fastq_merger` - FASTQ 文件合并工具
- `seq_replacer` - 序列替换工具

## 2. 运行示例

```bash
./examples.sh
```

这个脚本会演示所有主要功能。

## 3. 常用命令

### 合并 FASTQ 文件

```bash
# 基本用法
./fastq_merger -i file1.fq.gz -i file2.fq.gz -o merged.fq.gz -v

# 自定义 ID 参数
./fastq_merger -i file1.fq -i file2.fq -o output.fq \
    -p INSTRUMENT -r 1 -f FLOWCELL -l 1 -v
```

### 序列替换

```bash
# 随机替换（随机 reads + 随机位置）
./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -r -v

# 随机 reads + 固定位置
./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -R 20 -v

# 指定 reads + 指定位置
./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -1 3 10 -v

# 所有 reads + 相同位置
./seq_replacer -i input.fa -o output.fa -s NNNNNNNN -p 50 -v
```

## 4. 查看帮助

```bash
./fastq_merger --help
./seq_replacer --help
```

## 5. 清理

```bash
# 清理编译产物
make clean

# 删除测试文件
rm -f test_*.fq output_*.fq *.log
```

## 更多信息

详细文档请查看 [README.md](README.md)
