# API 文档

本文档描述了两个工具的内部 API 和数据结构，供开发者参考。

## fastq_parser 模块

### 数据结构

#### FastqRecord
```c
typedef struct {
    char *seq_id;        /* 序列 ID（不含 @ 符号）*/
    char *sequence;      /* 碱基序列 */
    char *plus_line;     /* 分隔符行（通常是 +）*/
    char *quality;       /* 质量分数 */
} FastqRecord;
```

#### FastqReader
```c
typedef struct {
    FILE *fp;            /* 文件指针 */
    char *filename;      /* 文件名 */
    size_t line_number;  /* 当前行号 */
    int is_valid;        /* 是否有效 */
    int is_pipe;         /* 是否为管道（gzip）*/
} FastqReader;
```

### 函数

#### fastq_reader_open
```c
FastqReader* fastq_reader_open(const char *filename);
```
打开 FASTQ 文件进行读取。

**参数：**
- `filename`: 文件路径（支持 .gz）

**返回：**
- 成功：FastqReader 指针
- 失败：NULL

#### fastq_reader_next
```c
int fastq_reader_next(FastqReader *reader, FastqRecord *record);
```
读取下一条 FASTQ 记录。

**参数：**
- `reader`: FastqReader 指针
- `record`: 用于存储记录的结构体

**返回：**
- 1: 成功读取
- 0: 文件结束
- -1: 错误

#### fastq_record_validate
```c
int fastq_record_validate(const FastqRecord *record, char *error_msg, size_t error_msg_size);
```
验证 FASTQ 记录格式。

**参数：**
- `record`: 要验证的记录
- `error_msg`: 错误消息缓冲区
- `error_msg_size`: 缓冲区大小

**返回：**
- 1: 有效
- 0: 无效

#### fastq_record_free
```c
void fastq_record_free(FastqRecord *record);
```
释放 FASTQ 记录内存。

#### fastq_reader_close
```c
void fastq_reader_close(FastqReader *reader);
```
关闭 FASTQ 读取器。

---

## id_generator 模块

### 数据结构

#### IdGeneratorConfig
```c
typedef struct {
    char *instrument_name;  /* 仪器名称 */
    char *run_id;           /* 运行编号 */
    char *flowcell_id;      /* 流动槽 ID */
    int lane;               /* 泳道编号 */
    int tile;               /* tile 编号 */
    int x_pos;              /* x 坐标 */
    int y_pos;              /* y 坐标 */
    int read_num;           /* 读段编号（1 或 2）*/
    char is_filtered;       /* 是否过滤（Y/N）*/
    int control_bits;       /* 控制位 */
    char *index_seq;        /* 索引序列 */
} IdGeneratorConfig;
```

#### IdGenerator
```c
typedef struct {
    IdGeneratorConfig config;
    size_t sequence_counter;  /* 序列计数器 */
} IdGenerator;
```

### 函数

#### id_generator_init
```c
IdGenerator* id_generator_init(const IdGeneratorConfig *config);
```
初始化 ID 生成器。

**参数：**
- `config`: 配置结构体（可为 NULL 使用默认值）

**返回：**
- IdGenerator 指针

#### id_generator_next
```c
char* id_generator_next(IdGenerator *gen);
```
生成下一个序列 ID。

**参数：**
- `gen`: IdGenerator 指针

**返回：**
- 新分配的 ID 字符串（需要调用者释放）

**ID 格式：**
```
INSTRUMENT:RUN:FLOWCELL:LANE:TILE:X:Y READ:FILTERED:CONTROL:INDEX
```

#### id_generator_free
```c
void id_generator_free(IdGenerator *gen);
```
释放 ID 生成器。

---

## file_merger 模块

### 数据结构

#### MergerConfig
```c
typedef struct {
    char **input_files;      /* 输入文件路径数组 */
    int num_input_files;     /* 输入文件数量 */
    char *output_file;       /* 输出文件路径 */
    IdGenerator *id_gen;     /* ID 生成器 */
    int verbose;             /* 详细输出标志 */
} MergerConfig;
```

#### MergerStats
```c
typedef struct {
    size_t total_sequences;  /* 处理的总序列数 */
    size_t total_files;      /* 处理的文件数 */
    int success;             /* 成功标志 */
} MergerStats;
```

### 函数

#### merge_fastq_files
```c
int merge_fastq_files(const MergerConfig *config, MergerStats *stats);
```
执行文件合并。

**参数：**
- `config`: 合并配置
- `stats`: 统计信息（输出）

**返回：**
- SUCCESS (0): 成功
- 错误码: 失败

#### write_fastq_record
```c
int write_fastq_record(FILE *out_fp, const char *new_id, const FastqRecord *record);
```
写入 FASTQ 记录到输出文件。

---

## seq_replacer 模块

### 数据结构

#### ReplacementMode
```c
typedef enum {
    MODE_RANDOM,         /* 随机模式 */
    MODE_RANDOM_FIXED,   /* 随机固定位置模式 */
    MODE_POSITION,       /* 位置模式 */
    MODE_SINGLE          /* 单个模式 */
} ReplacementMode;
```

#### ReplacerConfig
```c
typedef struct {
    char *input_file;
    char *output_file;
    char *replacement_seq;
    char *log_file;
    ReplacementMode mode;
    size_t position;
    size_t target_read_index;
    size_t total_reads;
    int verbose;
    unsigned int seed;
} ReplacerConfig;
```

#### ReplacementRecord
```c
typedef struct {
    char *seq_id;
    size_t position;
    char *original_seq;
    char *new_seq;
} ReplacementRecord;
```

### 函数

#### replace_sequences
```c
int replace_sequences(const ReplacerConfig *config);
```
执行序列替换。

**参数：**
- `config`: 替换配置

**返回：**
- SUCCESS (0): 成功
- 错误码: 失败

#### is_fasta_file / is_fastq_file
```c
int is_fasta_file(const char *filename);
int is_fastq_file(const char *filename);
```
检测文件类型。

---

## utils 模块

### 错误码

```c
#define SUCCESS 0
#define ERR_FILE_OPEN 1
#define ERR_FILE_READ 2
#define ERR_FILE_WRITE 3
#define ERR_INVALID_FORMAT 4
#define ERR_MEMORY_ALLOC 5
#define ERR_INVALID_PARAM 6
```

### 函数

#### 内存管理
```c
void* safe_malloc(size_t size);
void* safe_realloc(void *ptr, size_t size);
```

#### 字符串处理
```c
char* safe_strdup(const char *str);
void trim_newline(char *str);
```

#### 错误处理
```c
void error_exit(const char *format, ...);
void warning_msg(const char *format, ...);
```

#### 文件操作
```c
int file_exists(const char *filename);
long get_file_size(const char *filename);
```

---

## 使用示例

### 示例 1：读取 FASTQ 文件

```c
#include "fastq_parser.h"

FastqReader *reader = fastq_reader_open("input.fq.gz");
if (reader == NULL) {
    fprintf(stderr, "Failed to open file\n");
    return 1;
}

FastqRecord record;
while (fastq_reader_next(reader, &record) > 0) {
    printf("ID: %s\n", record.seq_id);
    printf("Seq: %s\n", record.sequence);
    
    fastq_record_free(&record);
}

fastq_reader_close(reader);
```

### 示例 2：生成序列 ID

```c
#include "id_generator.h"

IdGeneratorConfig config = {0};
config.instrument_name = "MYSEQ";
config.run_id = "100";
config.flowcell_id = "FC001";
config.lane = 1;

IdGenerator *gen = id_generator_init(&config);

for (int i = 0; i < 10; i++) {
    char *id = id_generator_next(gen);
    printf("%s\n", id);
    free(id);
}

id_generator_free(gen);
```

### 示例 3：合并文件

```c
#include "file_merger.h"

char *inputs[] = {"file1.fq", "file2.fq"};
IdGenerator *gen = id_generator_init(NULL);

MergerConfig config = {
    .input_files = inputs,
    .num_input_files = 2,
    .output_file = "merged.fq",
    .id_gen = gen,
    .verbose = 1
};

MergerStats stats;
int result = merge_fastq_files(&config, &stats);

printf("Processed %zu sequences\n", stats.total_sequences);

id_generator_free(gen);
```

---

## 编译选项

### 必需的编译标志

```makefile
CFLAGS = -std=c99 -O2 -Wall -Wextra
```

### 调试模式

```makefile
CFLAGS = -std=c99 -g -Wall -Wextra -DDEBUG
```

### 性能优化

```makefile
CFLAGS = -std=c99 -O3 -march=native -Wall -Wextra
```

---

## 线程安全

**注意：** 当前实现不是线程安全的。

如果需要并行处理：
- 为每个线程创建独立的 Reader/Generator 实例
- 不要在线程间共享 FastqReader 或 IdGenerator
- 使用互斥锁保护共享资源

---

## 扩展开发

### 添加新的文件格式

1. 在 `fastq_parser.c` 中添加格式检测
2. 实现新的读取函数
3. 更新 `is_*_file()` 函数

### 添加新的替换模式

1. 在 `seq_replacer.h` 中添加新的 `ReplacementMode`
2. 在 `seq_replacer.c` 中实现逻辑
3. 在 `seq_replace_main.c` 中添加命令行选项

### 性能优化建议

1. 使用更大的缓冲区（当前 8KB）
2. 实现多线程处理
3. 使用内存映射文件（mmap）
4. 优化字符串操作
