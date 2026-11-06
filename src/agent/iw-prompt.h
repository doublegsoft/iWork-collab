#ifndef IW_PROMPT_H
#define IW_PROMPT_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef enum {
  IW_LANG_C = 0,
  IW_LANG_CPP,
  IW_LANG_RUST,
  IW_LANG_GO,
  IW_LANG_PYTHON,
  IW_LANG_TYPESCRIPT,
  IW_LANG_COUNT
} iw_lang_type;

typedef struct {
  iw_lang_type type;
  const char* id;
  const char* name;
  const char* ext;
  const char* build_system;
  const char* std_version;
  const char* system_prompt;
  const char* default_task;
  const char* preset_code;
} iw_lang_profile;

/* --------------------------------------------------------------------------
 * 1. C99 / C11 语言基线与预置代码
 * -------------------------------------------------------------------------- */

static const char* IW_PROMPT_PROJ_DESC = 
  "文件全路径后，接``` ```代码块。\n"
  "代码缩进都为2个空格。\n"
  "<文件全路径>\n"
  "```\n"
  "文件内容\n"
  "```\n"
  "每个文件块都要求是完整代码，不能有任何缺失。";

static const char* IW_PROMPT_SYS_C =
  "You are a Senior Systems C Programmer.\n"
  "Follow these baseline requirements for all generated C code:\n"
  "1. Standard: Strict ISO C99 / C11, must compile with -std=c99 -Wall -Wextra -Werror.\n"
  "2. Memory Safety: All malloc/calloc calls must have corresponding free and NULL checks.\n"
  "3. String Safety: Never use gets(), strcpy(), sprintf(). Always use snprintf() and strncpy() with explicit bounds.\n"
  "4. Error Handling: Functions must return standard int error codes (0 for success) or pointer with NULL on failure.\n"
  "5. Formatting: Indent by 2 spaces. Pointers must be left-aligned (e.g., `char* buf`, `void* ctx`).\n"
  "6. Portability: Use <stdint.h>, <stdbool.h>, and POSIX/Windows cross-compatible guards.";

static const char* IW_PRESET_CODE_C =
  "#include <stdio.h>\n"
  "#include <stdlib.h>\n"
  "#include <stdbool.h>\n"
  "#include <stdint.h>\n"
  "#include <string.h>\n\n"
  "typedef struct {\n"
  "  uint8_t* buffer;\n"
  "  size_t capacity;\n"
  "  size_t size;\n"
  "} byte_stream_t;\n\n"
  "byte_stream_t* byte_stream_create(size_t initial_cap) {\n"
  "  byte_stream_t* stream = (byte_stream_t*)malloc(sizeof(byte_stream_t));\n"
  "  if (!stream) return NULL;\n"
  "  stream->buffer = (uint8_t*)malloc(initial_cap);\n"
  "  if (!stream->buffer) {\n"
  "    free(stream);\n"
  "    return NULL;\n"
  "  }\n"
  "  stream->capacity = initial_cap;\n"
  "  stream->size = 0;\n"
  "  return stream;\n"
  "}\n\n"
  "bool byte_stream_write(byte_stream_t* stream, const uint8_t* data, size_t len) {\n"
  "  if (!stream || !data) return false;\n"
  "  if (stream->size + len > stream->capacity) {\n"
  "    size_t new_cap = (stream->capacity * 2 >= stream->size + len) ? stream->capacity * 2 : stream->size + len;\n"
  "    uint8_t* new_buf = (uint8_t*)realloc(stream->buffer, new_cap);\n"
  "    if (!new_buf) return false;\n"
  "    stream->buffer = new_buf;\n"
  "    stream->capacity = new_cap;\n"
  "  }\n"
  "  memcpy(stream->buffer + stream->size, data, len);\n"
  "  stream->size += len;\n"
  "  return true;\n"
  "}\n\n"
  "void byte_stream_destroy(byte_stream_t* stream) {\n"
  "  if (!stream) return;\n"
  "  free(stream->buffer);\n"
  "  free(stream);\n"
  "}\n";

/* --------------------------------------------------------------------------
 * 2. C++17 / C++20 语言基线与预置代码
 * -------------------------------------------------------------------------- */
static const char* IW_PROMPT_SYS_CPP =
  "You are a Modern C++ Architect.\n"
  "Follow these baseline requirements for generated C++ code:\n"
  "1. Standard: Strict Modern C++20 (or C++17 minimum).\n"
  "2. Memory Safety: Strict RAII. Forbid raw new/delete. Use std::unique_ptr and std::shared_ptr.\n"
  "3. Modern Constructs: Prefer std::string_view, std::span, auto, and constexpr.\n"
  "4. Error Model: Use std::optional or std::expected (or noexcept with error codes) where appropriate.\n"
  "5. Formatting: 2 spaces indentation, left-aligned pointers/references (`T* ptr`, `const T& ref`).";

static const char* IW_PRESET_CODE_CPP =
  "#include <iostream>\n"
  "#include <memory>\n"
  "#include <string>\n"
  "#include <string_view>\n"
  "#include <vector>\n"
  "#include <optional>\n\n"
  "class PipelineTask {\n"
  "public:\n"
  "  explicit PipelineTask(std::string name) : task_name_(std::move(name)) {}\n"
  "  virtual ~PipelineTask() = default;\n\n"
  "  virtual std::optional<std::string> execute(std::string_view input) = 0;\n"
  "  [[nodiscard]] const std::string& name() const noexcept { return task_name_; }\n\n"
  "private:\n"
  "  std::string task_name_;\n"
  "};\n\n"
  "class EchoFilterTask final : public PipelineTask {\n"
  "public:\n"
  "  EchoFilterTask() : PipelineTask(\"EchoFilter\") {}\n"
  "  std::optional<std::string> execute(std::string_view input) override {\n"
  "    if (input.empty()) return std::nullopt;\n"
  "    return std::string(\"[PROCESSED]: \") + std::string(input);\n"
  "  }\n"
  "};\n";

/* --------------------------------------------------------------------------
 * 3. Rust (Edition 2021) 语言基线与预置代码
 * -------------------------------------------------------------------------- */
static const char* IW_PROMPT_SYS_RUST =
  "You are a Rust Core Engineer.\n"
  "Follow these baseline requirements for all generated Rust code:\n"
  "1. Edition: Rust 2021 edition.\n"
  "2. Safety: 100% safe Rust. No unsafe blocks unless strictly required with SAFETY comments.\n"
  "3. Robustness: Never use .unwrap() or .expect() in library code. Use thiserror / Result propagation.\n"
  "4. Concurrency: Use Tokio / async-std where asynchronous execution is required.\n"
  "5. Cleanliness: Satisfy cargo clippy -- -D warnings with zero warnings.";

static const char* IW_PRESET_CODE_RUST =
  "use std::collections::HashMap;\n"
  "use std::fmt;\n\n"
  "#[derive(Debug, Clone)]\n"
  "pub enum PipelineError {\n"
  "  EmptyPayload,\n"
  "  ExecutionFailure(String),\n"
  "}\n\n"
  "impl fmt::Display for PipelineError {\n"
  "  fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {\n"
  "    match self {\n"
  "      Self::EmptyPayload => write!(f, \"Payload cannot be empty\"),\n"
  "      Self::ExecutionFailure(msg) => write!(f, \"Task failed: {}\", msg),\n"
  "    }\n"
  "  }\n"
  "}\n\n"
  "pub struct TaskContext {\n"
  "  metadata: HashMap<String, String>,\n"
  "}\n\n"
  "impl TaskContext {\n"
  "  pub fn new() -> Self {\n"
  "    Self { metadata: HashMap::new() }\n"
  "  }\n\n"
  "  pub fn insert(&mut self, key: impl Into<String>, val: impl Into<String>) {\n"
  "    self.metadata.insert(key.into(), val.into());\n"
  "  }\n\n"
  "  pub fn run_pipeline(&self, input: &str) -> Result<String, PipelineError> {\n"
  "    if input.trim().is_empty() {\n"
  "      return Err(PipelineError::EmptyPayload);\n"
  "    }\n"
  "    Ok(format!(\"Pipeline processed: {}\", input))\n"
  "  }\n"
  "}\n";

/* --------------------------------------------------------------------------
 * 4. Go (1.20+) 语言基线与预置代码
 * -------------------------------------------------------------------------- */
static const char* IW_PROMPT_SYS_GO =
  "You are a Senior Go Specialist.\n"
  "Follow these baseline requirements for all generated Go code:\n"
  "1. Standard: Go 1.20+ with explicit module support (go.mod).\n"
  "2. Error Handling: Always return explicit error. No panic/recover in standard workflows.\n"
  "3. Context: All I/O and pipeline steps must take context.Context as their first parameter.\n"
  "4. Concurrency: Prevent goroutine leaks using sync.WaitGroup or errgroup.Group.\n"
  "5. Formatting: Follow gofmt strict conventions, single tab or 2 spaces.";

static const char* IW_PRESET_CODE_GO =
  "package pipeline\n\n"
  "import (\n"
  "  \"context\"\n"
  "  \"errors\"\n"
  "  \"fmt\"\n"
  "  \"sync\"\n"
  ")\n\n"
  "var ErrInvalidInput = errors.New(\"input cannot be empty\")\n\n"
  "type StepHandler func(ctx context.Context, payload string) (string, error)\n\n"
  "type Engine struct {\n"
  "  mu    sync.RWMutex\n"
  "  steps []StepHandler\n"
  "}\n\n"
  "func NewEngine() *Engine {\n"
  "  return &Engine{steps: make([]StepHandler, 0)}\n"
  "}\n\n"
  "func (e *Engine) Register(step StepHandler) {\n"
  "  e.mu.Lock()\n"
  "  defer e.mu.Unlock()\n"
  "  e.steps = append(e.steps, step)\n"
  "}\n\n"
  "func (e *Engine) Dispatch(ctx context.Context, data string) (string, error) {\n"
  "  if len(data) == 0 {\n"
  "    return \"\", ErrInvalidInput\n"
  "  }\n"
  "  curr := data\n"
  "  for _, step := range e.steps {\n"
  "    select {\n"
  "    case <-ctx.Done():\n"
  "      return \"\", ctx.Err()\n"
  "    default:\n"
  "    }\n"
  "    var err error\n"
  "    if curr, err = step(ctx, curr); err != nil {\n"
  "      return \"\", fmt.Errorf(\"step execution failed: %w\", err)\n"
  "    }\n"
  "  }\n"
  "  return curr, nil\n"
  "}\n";

/* --------------------------------------------------------------------------
 * 5. Python (3.10+) 语言基线与预置代码
 * -------------------------------------------------------------------------- */
static const char* IW_PROMPT_SYS_PYTHON =
  "You are a Senior Python Backend Engineer.\n"
  "Follow these baseline requirements for generated Python code:\n"
  "1. Version: Python 3.10+.\n"
  "2. Type Safety: 100% strict type hints everywhere using typing (Optional, List, Dict, Protocol).\n"
  "3. Data Modelling: Prefer dataclasses(slots=True) or Pydantic V2 models.\n"
  "4. Async Support: Provide async/await methods for any network/IO bound pipeline jobs.\n"
  "5. Formatting: PEP 8 compliant, 2 or 4 spaces indentation.";

static const char* IW_PRESET_CODE_PYTHON =
  "from dataclasses import dataclass\n"
  "from typing import Protocol, Optional, List\n"
  "import asyncio\n\n"
  "@dataclass(slots=True)\n"
  "class ExecutionResult:\n"
  "  task_id: str\n"
  "  success: bool\n"
  "  data: Optional[str] = None\n"
  "  error: Optional[str] = None\n\n"
  "class StepWorker(Protocol):\n"
  "  async def execute(self, payload: str) -> ExecutionResult:\n"
  "    ...\n\n"
  "class ParallelDispatcher:\n"
  "  def __init__(self) -> None:\n"
  "    self._workers: List[StepWorker] = []\n\n"
  "  def register(self, worker: StepWorker) -> None:\n"
  "    self._workers.append(worker)\n\n"
  "  async def run_all(self, payload: str) -> List[ExecutionResult]:\n"
  "    tasks = [w.execute(payload) for w in self._workers]\n"
  "    return await asyncio.gather(*tasks)\n";

/* --------------------------------------------------------------------------
 * 6. TypeScript (ES2022) 语言基线与预置代码
 * -------------------------------------------------------------------------- */
static const char* IW_PROMPT_SYS_TYPESCRIPT =
  "You are a Principal TypeScript Architect.\n"
  "Follow these baseline requirements for generated TypeScript code:\n"
  "1. Configuration: strict mode true, noUncheckedIndexedAccess enabled, target ES2022.\n"
  "2. Anti-any: Strictly forbid 'any'. Use 'unknown', generics, or discriminated unions.\n"
  "3. Immutability: Use readonly for public properties and ReadonlyArray where possible.\n"
  "4. Async/Await: Standard Promise-based pipeline methods with proper error boundaries.\n"
  "5. Formatting: 2 spaces indent, semicolon enabled.";

static const char* IW_PRESET_CODE_TYPESCRIPT =
  "export type TaskState = 'IDLE' | 'RUNNING' | 'SUCCESS' | 'FAILED';\n\n"
  "export interface TaskEvent<T = unknown> {\n"
  "  readonly id: string;\n"
  "  readonly timestamp: number;\n"
  "  readonly payload: T;\n"
  "}\n\n"
  "export interface PipelineNode<TIn, TOut> {\n"
  "  readonly name: string;\n"
  "  process(input: TIn): Promise<TOut>;\n"
  "}\n\n"
  "export class LinearPipeline<T> {\n"
  "  private readonly steps: PipelineNode<T, T>[] = [];\n\n"
  "  public use(node: PipelineNode<T, T>): this {\n"
  "    this.steps.push(node);\n"
  "    return this;\n"
  "  }\n\n"
  "  public async execute(initialInput: T): Promise<T> {\n"
  "    let current = initialInput;\n"
  "    for (const step of this.steps) {\n"
  "      current = await step.process(current);\n"
  "    }\n"
  "    return current;\n"
  "  }\n"
  "}\n";

/* --------------------------------------------------------------------------
 * 语言配置注册表
 * -------------------------------------------------------------------------- */
static const iw_lang_profile IW_LANG_REGISTRY[IW_LANG_COUNT] = {
  {
    .type = IW_LANG_C,
    .id = "c",
    .name = "C (C99/C11)",
    .ext = ".c",
    .build_system = "CMake / Makefile",
    .std_version = "c99",
    .system_prompt = IW_PROMPT_SYS_C,
    .default_task = "Implement high-performance byte stream processor with bounds checking",
    .preset_code = IW_PRESET_CODE_C
  },
  {
    .type = IW_LANG_CPP,
    .id = "cpp",
    .name = "C++ (Modern C++20)",
    .ext = ".cpp",
    .build_system = "CMake (Ninja)",
    .std_version = "c++20",
    .system_prompt = IW_PROMPT_SYS_CPP,
    .default_task = "Create an abstract RAII task pipeline with std::optional results",
    .preset_code = IW_PRESET_CODE_CPP
  },
  {
    .type = IW_LANG_RUST,
    .id = "rust",
    .name = "Rust (Edition 2021)",
    .ext = ".rs",
    .build_system = "Cargo",
    .std_version = "2021",
    .system_prompt = IW_PROMPT_SYS_RUST,
    .default_task = "Build a safe pipeline executor using custom enum error handling",
    .preset_code = IW_PRESET_CODE_RUST
  },
  {
    .type = IW_LANG_GO,
    .id = "go",
    .name = "Go (Golang 1.20+)",
    .ext = ".go",
    .build_system = "go build / go test",
    .std_version = "1.20+",
    .system_prompt = IW_PROMPT_SYS_GO,
    .default_task = "Implement context-cancellable concurrency pipeline engine",
    .preset_code = IW_PRESET_CODE_GO
  },
  {
    .type = IW_LANG_PYTHON,
    .id = "python",
    .name = "Python (3.10+)",
    .ext = ".py",
    .build_system = "pytest / poetry / uv",
    .std_version = "3.10+",
    .system_prompt = IW_PROMPT_SYS_PYTHON,
    .default_task = "Construct an async step parallel execution dispatcher with dataclasses",
    .preset_code = IW_PRESET_CODE_PYTHON
  },
  {
    .type = IW_LANG_TYPESCRIPT,
    .id = "typescript",
    .name = "TypeScript (ES2022)",
    .ext = ".ts",
    .build_system = "tsc / vite / esbuild",
    .std_version = "ES2022",
    .system_prompt = IW_PROMPT_SYS_TYPESCRIPT,
    .default_task = "Create a typed linear pipeline engine with strict immutable event types",
    .preset_code = IW_PRESET_CODE_TYPESCRIPT
  }
};

/* --------------------------------------------------------------------------
 * 辅助查询与组装函数
 * -------------------------------------------------------------------------- */
static inline const iw_lang_profile* iw_prompt_get_profile(iw_lang_type lang) {
  if ((int)lang >= 0 && (int)lang < IW_LANG_COUNT) {
    return &IW_LANG_REGISTRY[lang];
  }
  return &IW_LANG_REGISTRY[IW_LANG_C];
}

static inline const iw_lang_profile* iw_prompt_find_by_id(const char* id) {
  if (!id) return NULL;
  for (int i = 0; i < IW_LANG_COUNT; i++) {
    if (strcmp(IW_LANG_REGISTRY[i].id, id) == 0) {
      return &IW_LANG_REGISTRY[i];
    }
  }
  return NULL;
}

static inline int iw_prompt_build_full_instruction(const iw_lang_profile* prof, const char* user_req, char* out_buf, size_t max_len) {
  if (!prof || !out_buf || max_len == 0) return -1;
  const char* task = (user_req && strlen(user_req) > 0) ? user_req : prof->default_task;

  return snprintf(out_buf, max_len,
    "=== SYSTEM PROMPT [%s] ===\n%s\n\n"
    "=== TARGET ENVIRONMENT ===\n"
    "- Build Tool: %s\n"
    "- Standard  : %s\n\n"
    "=== USER TASK ===\n%s\n",
    prof->name, prof->system_prompt, prof->build_system, prof->std_version, task);
}

#endif /* IW_PROMPT_H */