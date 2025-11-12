# FASTQ合并程序设计文档

## 概述

本程序是一个用C语言编写的命令行工具，用于合并多个FASTQ格式的测序数据文件，并重新生成符合测序仪输出格式的序列ID。程序采用流式处理架构，确保在处理大文件时保持低内存占用。

## 架构

程序采用模块化设计，主要包含以下模块：

```
fastq-merger/
├── main.c              # 主程序入口，命令行参数解析
├── fastq_parser.c/h    # FASTQ文件解析和验证
├── id_generator.c/h    # 序列ID生成器
├── file_merger.c/h     # 文件合并逻辑
└── utils.c/h           # 工具函数（错误处理、内存管理等）
```

### 数据流

```
输入FASTQ文件1 ─┐
输入FASTQ文件2 ─┼─> 文件读取 -> 格式验证 -> ID重命名 -> 写入输出 -> 输出FASTQ文件
输入FASTQ文件N ─┘
```

## 组件和接口

### 1. 主程序模块 (main.c)

**职责**: 程序入口，解析命令行参数，协调各模块工作

**接口**:
```c
int main(int argc, char *argv[]);
void print_usage(const char *program_name);
void print_version();
```

**命令行参数设计**:
```
fastq_merger -i input1.fq -i input2.fq -o output.fq [选项]

必需参数:
  -i, --input <file>     输入FASTQ文件（可多次指定）
  -o, --output <file>    输出FASTQ文件路径

可选参数:
  -p, --prefix <string>  序列ID前缀（默认: "INSTRUMENT"）
  -r, --run-id <string>  运行编号（默认: "1"）
  -f, --flowcell <string> 流动槽ID（默认: "FLOWCELL"）
  -l, --lane <int>       泳道编号（默认: 1）
  -v, --verbose          详细输出模式
  -h, --help             显示帮助信息
  --version              显示版本信息
```

### 2. FASTQ解析器模块 (fastq_parser.c/h)

**职责**: 读取和验证FASTQ文件格式

**数据结构**:
```c
typedef struct {
    char *seq_id;        // 序列ID（不含@符号）
    char *sequence;      // 碱基序列
    char *plus_line;     // 分隔符行（通常是+）
    char *quality;       // 质量分数
} FastqRecord;

typedef struct {
    FILE *fp;
    char *filename;
    size_t line_number;
    int is_valid;
} FastqReader;
```

**接口**:
```c
// 打开FASTQ文件进行读取
FastqReader* fastq_reader_open(const char *filename);

// 读取下一条FASTQ记录
int fastq_reader_next(FastqReader *reader, FastqRecord *record);

// 验证FASTQ记录格式
int fastq_record_validate(const FastqRecord *record, char *error_msg, size_t error_msg_size);

// 释放FASTQ记录内存
void fastq_record_free(FastqRecord *record);

// 关闭FASTQ读取器
void fastq_reader_close(FastqReader *reader);
```

### 3. ID生成器模块 (id_generator.c/h)

**职责**: 生成符合Illumina测序仪格式的序列ID

**数据结构**:
```c
typedef struct {
    char *instrument_name;  // 仪器名称
    char *run_id;           // 运行编号
    char *flowcell_id;      // 流动槽ID
    int lane;               // 泳道编号
    int tile;               // tile编号
    int x_pos;              // x坐标
    int y_pos;              // y坐标
    int read_num;           // 读段编号（1或2）
    char is_filtered;       // 是否过滤（Y/N）
    int control_bits;       // 控制位
    char *index_seq;        // 索引序列
} IdGeneratorConfig;

typedef struct {
    IdGeneratorConfig config;
    size_t sequence_counter;  // 序列计数器
} IdGenerator;
```

**接口**:
```c
// 初始化ID生成器
IdGenerator* id_generator_init(const IdGeneratorConfig *config);

// 生成下一个序列ID
char* id_generator_next(IdGenerator *gen);

// 释放ID生成器
void id_generator_free(IdGenerator *gen);
```

**ID格式示例**:
```
@INSTRUMENT:1:FLOWCELL:1:1001:1000:1000 1:N:0:ATCG
```

### 4. 文件合并模块 (file_merger.c/h)

**职责**: 协调文件读取、ID重命名和输出写入

**数据结构**:
```c
typedef struct {
    char **input_files;      // 输入文件路径数组
    int num_input_files;     // 输入文件数量
    char *output_file;       // 输出文件路径
    IdGenerator *id_gen;     // ID生成器
    int verbose;             // 是否详细输出
} MergerConfig;

typedef struct {
    size_t total_sequences;  // 处理的总序列数
    size_t total_files;      // 处理的文件数
    int success;             // 是否成功
} MergerStats;
```

**接口**:
```c
// 执行文件合并
int merge_fastq_files(const MergerConfig *config, MergerStats *stats);

// 写入FASTQ记录到输出文件
int write_fastq_record(FILE *out_fp, const char *new_id, const FastqRecord *record);
```

### 5. 工具模块 (utils.c/h)

**职责**: 提供通用工具函数

**接口**:
```c
// 安全的内存分配
void* safe_malloc(size_t size);
void* safe_realloc(void *ptr, size_t size);

// 字符串处理
char* safe_strdup(const char *str);
void trim_newline(char *str);

// 错误处理
void error_exit(const char *format, ...);
void warning_msg(const char *format, ...);

// 文件操作
int file_exists(const char *filename);
long get_file_size(const char *filename);
```

## 数据模型

### FASTQ文件格式

每条FASTQ记录包含4行：
```
@SEQ_ID                    # 第1行：序列标识符（以@开头）
GATTTGGGGTTCAAAGCAGTATCG   # 第2行：碱基序列
+                          # 第3行：分隔符（+号，可选地跟原ID）
!''*((((***+))%%%++)(%%%  # 第4行：质量分数（ASCII编码）
```

### 内存管理策略

- 使用动态缓冲区读取每一行，初始大小256字节，按需扩展
- 每次只在内存中保持一条FASTQ记录
- 处理完一条记录后立即释放内存
- 使用固定大小的写入缓冲区（8KB）提高I/O效率

## 错误处理

### 错误类型

1. **文件错误**
   - 文件不存在
   - 文件无法打开
   - 磁盘空间不足
   - 写入失败

2. **格式错误**
   - FASTQ格式不正确（记录不是4行）
   - 序列和质量分数长度不匹配
   - 包含非法字符

3. **参数错误**
   - 缺少必需参数
   - 参数值无效
   - 输入输出文件相同

### 错误处理策略

- 所有函数返回状态码（0表示成功，非0表示错误）
- 使用errno和自定义错误消息提供详细错误信息
- 在main函数中集中处理错误，确保资源正确释放
- 提供详细的错误位置信息（文件名、行号）

### 错误码定义

```c
#define SUCCESS 0
#define ERR_FILE_OPEN 1
#define ERR_FILE_READ 2
#define ERR_FILE_WRITE 3
#define ERR_INVALID_FORMAT 4
#define ERR_MEMORY_ALLOC 5
#define ERR_INVALID_PARAM 6
```

## 测试策略

### 单元测试

使用简单的测试框架测试各个模块：

1. **FASTQ解析器测试**
   - 测试正确格式的FASTQ文件解析
   - 测试格式错误检测
   - 测试边界情况（空文件、单条记录等）

2. **ID生成器测试**
   - 测试ID格式正确性
   - 测试ID唯一性
   - 测试自定义参数

3. **文件合并测试**
   - 测试单文件处理
   - 测试多文件合并
   - 测试大文件处理

### 集成测试

1. **端到端测试**
   - 准备多个测试FASTQ文件
   - 执行合并操作
   - 验证输出文件格式和内容

2. **性能测试**
   - 测试处理100MB文件的时间和内存使用
   - 测试处理1GB文件的时间和内存使用

3. **错误场景测试**
   - 测试各种错误输入的处理
   - 测试资源清理

### 测试数据

创建以下测试文件：
- `test_small.fq`: 10条记录的小文件
- `test_invalid.fq`: 包含格式错误的文件
- `test_large.fq`: 100万条记录的大文件

## 性能考虑

### 优化策略

1. **I/O优化**
   - 使用缓冲I/O（setvbuf）
   - 批量写入而非逐行写入
   - 使用合适的缓冲区大小（8KB）

2. **内存优化**
   - 流式处理，避免加载整个文件
   - 及时释放不再使用的内存
   - 使用内存池减少malloc/free调用

3. **算法优化**
   - 使用简单的计数器生成坐标，避免复杂计算
   - 预分配ID字符串缓冲区，避免重复分配

### 预期性能指标

- 处理速度: >10MB/s
- 内存使用: <100MB（无论文件大小）
- CPU使用: 单核<50%

## 编译和部署

### 编译要求

- C编译器: GCC 4.8+ 或 Clang 3.5+
- 标准: C99或更高
- 依赖: 仅标准C库，无外部依赖

### 编译命令

```bash
gcc -std=c99 -O2 -Wall -Wextra -o fastq_merger \
    main.c fastq_parser.c id_generator.c file_merger.c utils.c
```

### Makefile示例

```makefile
CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra
TARGET = fastq_merger
SOURCES = main.c fastq_parser.c id_generator.c file_merger.c utils.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	./run_tests.sh
```

## 扩展性考虑

### 未来可能的扩展

1. **支持压缩文件**
   - 支持gzip压缩的FASTQ文件（.fq.gz）
   - 使用zlib库进行解压缩

2. **并行处理**
   - 使用多线程并行处理多个输入文件
   - 使用OpenMP简化并行实现

3. **更多ID格式**
   - 支持其他测序平台的ID格式
   - 可配置的ID模板

4. **统计信息**
   - 输出序列长度分布
   - 输出质量分数统计
   - 生成处理报告

### 设计决策

1. **为什么使用C语言？**
   - 高性能，适合处理大文件
   - 低内存占用
   - 无运行时依赖，易于部署

2. **为什么使用流式处理？**
   - 支持处理超大文件（>内存大小）
   - 内存使用可预测且稳定
   - 实时处理，无需等待全部读取完成

3. **为什么模拟Illumina ID格式？**
   - Illumina是最常用的测序平台
   - 格式标准化，易于被下游工具识别
   - 包含足够的元数据信息
