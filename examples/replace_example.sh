#!/bin/bash
# Sequence Replacer 使用示例

echo "=== Sequence Replacer 示例 ==="
echo

# 示例 1: 随机模式
echo "1. 随机模式 - 随机选择一条reads，在随机位置替换"
echo "命令: ./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -r -v"
echo

# 示例 2: 随机固定位置模式（最快）
echo "2. 随机固定位置模式 - 随机选择一条reads，在位置20处替换（最快！）"
echo "命令: ./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -R 20 -v"
echo

# 示例 3: 指定reads模式
echo "3. 指定reads模式 - 只替换第3条reads，在位置10处替换"
echo "命令: ./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -1 3 10 -v"
echo

# 示例 4: 全部替换模式
echo "4. 全部替换模式 - 在所有reads的位置50处替换"
echo "命令: ./seq_replacer -i input.fa -o output.fa -s NNNNNNNN -p 50 -l changes.log"
echo

# 示例 5: 可重现的随机替换
echo "5. 可重现的随机替换 - 使用固定种子"
echo "命令: ./seq_replacer -i input.fq -o output.fq -s GCGCGCGC -r --seed 12345"
echo

echo "性能对比（2GB文件）:"
echo "  -R (random-fixed): 最快 ⚡⚡⚡"
echo "  -1 (single):       快   ⚡⚡"
echo "  -r (random):       较快 ⚡"
echo "  -p (position):     慢   （需要替换所有reads）"
echo

echo "提示: 使用 --help 查看所有选项"
echo "./seq_replacer --help"
