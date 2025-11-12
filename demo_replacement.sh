#!/bin/bash

# 演示序列替换功能

echo "=== 序列替换演示 ==="
echo ""

# 创建测试文件
cat > demo.fq << 'EOF'
@SEQ1
AAAAAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEE
+
IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
@SEQ2
GGGGGGGGGGHHHHHHHHHHIIIIIIIIIIJJJJJJJJJJKKKKKKKKKK
+
JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ
@SEQ3
LLLLLLLLLLMMMMMMMMMMNNNNNNNNNNOOOOOOOOOOpppppppppp
+
KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK
EOF

echo "原始序列："
cat demo.fq
echo ""
echo "================================================"
echo ""

# 示例1：在位置10替换8个碱基
echo "示例1：在所有序列的位置10处替换为 'XXXXXXXX'"
./seq_replacer -i demo.fq -o demo_out1.fq -s XXXXXXXX -p 10 -v
echo ""
echo "结果："
grep -A 1 "^@" demo_out1.fq | grep -v "^--$" | grep -v "^+$"
echo ""
echo "================================================"
echo ""

# 示例2：随机替换2个序列
echo "示例2：在2个随机序列的随机位置替换为 'NNNNNNNN'"
./seq_replacer -i demo.fq -o demo_out2.fq -s NNNNNNNN -r 2 -v --seed 42
echo ""
echo "结果："
grep -A 1 "^@" demo_out2.fq | grep -v "^--$" | grep -v "^+$"
echo ""
echo "================================================"
echo ""

echo "查看替换日志："
cat replacements.log

# 清理
rm -f demo.fq demo_out1.fq demo_out2.fq replacements.log
