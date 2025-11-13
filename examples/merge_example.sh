#!/bin/bash
# FASTQ Merger 使用示例

echo "=== FASTQ Merger 示例 ==="
echo

# 示例 1: 基本合并
echo "1. 基本合并两个文件"
echo "命令: ./fastq_merger -i file1.fq.gz -i file2.fq.gz -o merged.fq.gz -v"
echo

# 示例 2: 自定义参数
echo "2. 自定义序列 ID 参数"
echo "命令: ./fastq_merger -i input1.fq -i input2.fq -o output.fq \\"
echo "         -p MYINST -r 100 -f FC001 -l 2 -v"
echo

# 示例 3: 合并多个文件
echo "3. 合并多个文件"
echo "命令: ./fastq_merger \\"
echo "         -i lane1.fq.gz \\"
echo "         -i lane2.fq.gz \\"
echo "         -i lane3.fq.gz \\"
echo "         -i lane4.fq.gz \\"
echo "         -o all_lanes.fq.gz -v"
echo

echo "提示: 使用 --help 查看所有选项"
echo "./fastq_merger --help"
