#!/bin/bash
# FASTQ/FASTA 处理工具示例脚本

echo "=== FASTQ/FASTA 处理工具示例 ==="
echo ""

# 检查工具是否存在
if [ ! -f "./fastq_merger" ] || [ ! -f "./seq_replacer" ]; then
    echo "错误：工具未编译。请先运行 'make' 编译工具。"
    exit 1
fi

echo "1. 创建测试数据..."
cat > test_example.fq << 'EOF'
@READ1
ATCGATCGATCGATCGATCGATCGATCGATCGATCGATCGATCG
+
IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
@READ2
GCGCGCGCGCGCGCGCGCGCGCGCGCGCGCGCGCGCGCGCGCGC
+
JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ
@READ3
TTTTAAAACCCCGGGGTTTTAAAACCCCGGGGTTTTAAAACCCC
+
KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK
EOF

cat > test_example2.fq << 'EOF'
@READ4
AAAATTTTCCCCGGGGAAAATTTTCCCCGGGGAAAATTTTCCCC
+
LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@READ5
GGGGCCCCAAAATTTTGGGGCCCCAAAATTTTGGGGCCCCAAAA
+
MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
EOF

echo "✓ 测试数据已创建"
echo ""

echo "2. 示例：合并 FASTQ 文件"
echo "命令：./fastq_merger -i test_example.fq -i test_example2.fq -o merged_example.fq -v"
./fastq_merger -i test_example.fq -i test_example2.fq -o merged_example.fq -v
echo ""

echo "3. 查看合并结果（前 8 行）"
head -8 merged_example.fq
echo ""

echo "4. 示例：随机替换一条 reads"
echo "命令：./seq_replacer -i merged_example.fq -o random_replaced.fq -s NNNNNNNN -r -v --seed 42"
./seq_replacer -i merged_example.fq -o random_replaced.fq -s NNNNNNNN -r -v --seed 42
echo ""

echo "5. 示例：在指定位置替换指定 reads"
echo "命令：./seq_replacer -i merged_example.fq -o single_replaced.fq -s XXXXXXXX -1 2 10 -v"
./seq_replacer -i merged_example.fq -o single_replaced.fq -s XXXXXXXX -1 2 10 -v
echo ""

echo "6. 示例：随机选择一条 reads，在固定位置替换"
echo "命令：./seq_replacer -i merged_example.fq -o random_fixed.fq -s TTTTTTTT -R 5 -v --seed 999"
./seq_replacer -i merged_example.fq -o random_fixed.fq -s TTTTTTTT -R 5 -v --seed 999
echo ""

echo "7. 查看替换日志"
cat replacements.log
echo ""

echo "8. 清理测试文件"
read -p "是否删除测试文件？(y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm -f test_example.fq test_example2.fq merged_example.fq
    rm -f random_replaced.fq single_replaced.fq random_fixed.fq
    rm -f replacements.log
    echo "✓ 测试文件已删除"
else
    echo "测试文件保留在当前目录"
fi

echo ""
echo "=== 示例完成 ==="
echo "更多信息请查看 README.md"
