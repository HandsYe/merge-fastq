# 性能指南

## fastq_merger 性能

### 基准测试

测试环境：
- CPU: Intel Xeon
- 内存: 16GB
- 磁盘: SSD

| 文件大小 | 文件数 | 处理时间 | 内存使用 | 吞吐量 |
|---------|--------|---------|---------|--------|
| 2.1GB × 2 | 2 | ~4分钟 | <100MB | ~17MB/s |
| 1GB × 4 | 4 | ~4分钟 | <100MB | ~16MB/s |
| 500MB × 8 | 8 | ~4分钟 | <100MB | ~16MB/s |

### 优化建议

1. **使用压缩文件**
   - 输入和输出都使用 .gz 格式
   - 减少磁盘 I/O

2. **SSD 存储**
   - 使用 SSD 而不是 HDD
   - 显著提升 I/O 性能

3. **避免网络存储**
   - 在本地磁盘处理
   - 完成后再传输到网络存储

---

## seq_replacer 性能

### 模式性能对比

测试文件：2.1GB FASTQ 文件（约1670万条reads）

| 模式 | 处理时间 | 内存使用 | 说明 |
|------|---------|---------|------|
| `-R` (random-fixed) | ~30秒 | <100MB | 最快！只需一次扫描 |
| `-1` (single) | ~30秒 | <100MB | 快速，直接定位 |
| `-r` (random) | ~35秒 | <100MB | 需要计算随机位置 |
| `-p` (position) | ~2分钟 | <100MB | 需要替换所有reads |

### 优化策略

#### 1. 选择合适的模式

**最快模式：`-R` (random-fixed)**
```bash
# 推荐用于大文件
./seq_replacer -i large.fq.gz -o output.fq.gz -s ATCGATCG -R 20 -v
```

**原因：**
- 使用 `wc -l` 快速计数（不读取内容）
- 只读取文件一遍
- 不需要计算随机位置

#### 2. 使用压缩文件

```bash
# 输入输出都使用压缩
./seq_replacer -i input.fq.gz -o output.fq.gz -s NNNNNNNN -R 10
```

#### 3. 并行处理多个文件

```bash
# 使用 GNU parallel 并行处理
parallel -j 4 './seq_replacer -i {} -o {.}_replaced.fq.gz -s ATCGATCG -R 20' ::: *.fq.gz
```

---

## 内存使用

### fastq_merger

- **固定内存**：约 50MB
- **缓冲区**：8KB 写入缓冲
- **动态分配**：每条 reads 约 1KB（处理后立即释放）

**总计**：<100MB（无论文件大小）

### seq_replacer

- **固定内存**：约 50MB
- **缓冲区**：8KB 写入缓冲
- **动态分配**：每条 reads 约 1KB（处理后立即释放）

**总计**：<100MB（无论文件大小）

---

## 磁盘 I/O

### 读取优化

- 使用缓冲 I/O（setvbuf）
- 流式处理，避免随机访问
- 支持 gzip 压缩（通过 popen）

### 写入优化

- 8KB 写入缓冲区
- 批量写入减少系统调用
- 支持直接写入压缩文件

---

## 大文件处理建议

### 文件大小 > 10GB

1. **使用 `-R` 模式**（seq_replacer）
   ```bash
   ./seq_replacer -i huge.fq.gz -o output.fq.gz -s ATCGATCG -R 20
   ```

2. **监控进度**
   ```bash
   # 使用 pv 监控进度
   pv input.fq.gz | gzip -dc | ./seq_replacer -i - -o output.fq.gz -s ATCGATCG -R 20
   ```

3. **分批处理**
   ```bash
   # 将大文件分割成小文件
   zcat huge.fq.gz | split -l 40000000 - chunk_
   
   # 并行处理
   for f in chunk_*; do
       ./seq_replacer -i $f -o ${f}_out.fq -s ATCGATCG -R 20 &
   done
   wait
   
   # 合并结果
   ./fastq_merger -i chunk_*_out.fq -o final.fq.gz
   ```

---

## 性能监控

### 使用 time 命令

```bash
time ./fastq_merger -i file1.fq.gz -i file2.fq.gz -o merged.fq.gz
```

### 使用 /usr/bin/time 获取详细信息

```bash
/usr/bin/time -v ./seq_replacer -i input.fq.gz -o output.fq.gz -s ATCGATCG -R 20
```

输出包括：
- 实际运行时间
- 用户 CPU 时间
- 系统 CPU 时间
- 最大内存使用
- I/O 统计

---

## 故障排除

### 问题：处理速度慢

**可能原因：**
1. 使用 HDD 而不是 SSD
2. 网络存储延迟
3. 系统负载高

**解决方案：**
1. 迁移到 SSD
2. 使用本地存储
3. 减少并发进程

### 问题：内存不足

**可能原因：**
- 系统内存 < 512MB

**解决方案：**
- 这些工具设计为低内存占用
- 检查其他进程的内存使用
- 考虑增加系统内存

---

## 最佳实践

1. **使用压缩文件**
   - 减少磁盘空间
   - 减少 I/O 时间

2. **选择合适的模式**
   - 大文件使用 `-R` 模式
   - 需要精确控制使用 `-1` 模式

3. **批量处理**
   - 使用脚本自动化
   - 使用 parallel 并行处理

4. **监控资源**
   - 使用 `htop` 监控 CPU/内存
   - 使用 `iotop` 监控磁盘 I/O

5. **验证输出**
   - 检查输出文件大小
   - 验证格式正确性
   - 查看日志文件
