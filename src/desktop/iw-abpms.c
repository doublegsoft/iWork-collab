#include <cui/cui.h>
#include <cui/cui_app.h>
#include <stdbool.h>
#include <stdio.h>

/* =========================================================================
 * 状态定义
 * ========================================================================= */

typedef struct {
  int active_cmd;         /* 选中的指令 (1 ~ 4) */
  char last_action[128];  /* 状态反馈信息 */
  bool font_loaded;       /* 标记中文字体是否已加载 */
} app_state;

/* =========================================================================
 * 加载 macOS 系统中文字体并注册为 Fallback
 * ========================================================================= */

static void ensure_chinese_font(cui_ctx* ui, app_state* state) {
  if (state->font_loaded) {
    return;
  }
  state->font_loaded = true;

  /* 候选的 macOS 系统 CJK 中文字体路径 */
  const char* candidates[] = {
    "/System/Library/Fonts/PingFang.ttc",
    "/System/Library/Fonts/Hiragino Sans GB.ttc",
    "/System/Library/Fonts/STHeiti Light.ttc",
    "/Library/Fonts/Arial Unicode.ttf"
  };

  cui_font cjk_font = -1;
  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
    FILE* f = fopen(candidates[i], "rb");
    if (f) {
      fclose(f);
      cjk_font = cui_font_load_file(ui, candidates[i]);
      if (cjk_font >= 0) {
        break;
      }
    }
  }

  /* 如果成功载入中文字体，将其绑定为默认字体的 Fallback (CJK 回退) */
  if (cjk_font >= 0) {
    /* 0 代表默认的 CUI_FONT_BODY */
    cui_font_add_fallback(ui, 0, cjk_font);
  }
}

/* =========================================================================
 * 指令按钮组件 (网格中的单个按键)
 * ========================================================================= */

static void render_cmd_button(cui_ctx* ui, app_state* state, int idx, const char* label) {
  bool active = (state->active_cmd == idx);

  /* 激活时显示实体主色按钮，平时显示轮廓质感按钮 */
  cui_button_variant variant = active ? CUI_BUTTON_PRIMARY : CUI_BUTTON_OUTLINE;
  const char* icon_right = active ? CUI_ICON_CHECK : NULL;

  if (cui_button(ui, label,
    .variant = variant,
    .icon = CUI_ICON_CHEVRON_RIGHT,
    .icon_right = icon_right,
    .full_width = true,
    .size = CUI_SIZE_MD,
    .selected = active
  )) {
    state->active_cmd = idx;
    snprintf(state->last_action, sizeof(state->last_action), "已切换至: %s", label);
    cui_toast(ui, state->last_action, .tone = CUI_TONE_SUCCESS);
  }
}

/* =========================================================================
 * 界面主渲染函数
 * ========================================================================= */

static void frame(cui_ctx* ui, void* user) {
  app_state* state = (app_state*)user;

  /* 确保中文字体已就绪 */
  ensure_chinese_font(ui, state);

  cui_scroll(ui, .id = "main-scroll", .width = CUI_FILL, .height = CUI_FILL, .fade_edges = true) {
    cui_row(ui, .justify = CUI_JUSTIFY_CENTER, .align = CUI_ALIGN_START, .pad_y = 16) {
      cui_column(ui, .width = CUI_FILL, .max_width = 880, .pad_x = 16, .gap = 20) {

        /* =================================================================
         * 模块 1：香港堡垒机 (发送按钮靠上、右端对齐)
         * ================================================================= */
        cui_row(ui, .width = CUI_FILL, .align = CUI_ALIGN_START, .pad_top = 12, .pad_bottom = 6) {
          /* 左侧标题与描述 */
          cui_column(ui, .gap = 4) {
            cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
              cui_icon(ui, CUI_ICON_MONITOR, 18, CUI_COLOR_FOREGROUND);
              cui_heading(ui, "香港堡垒机", 3);
            }
            cui_secondary(ui, "执行预设控制指令，或将待解析邮件文件拖入工作区。");
          }

          /* 顶开中间留白，将发送按钮推至最右侧 */
          cui_spacer(ui);

          /* 靠上、右端对齐的“发送”按钮 */
          if (cui_button(ui, "发送", .variant = CUI_BUTTON_PRIMARY, .icon = CUI_ICON_SEND, .size = CUI_SIZE_SM)) {
            snprintf(state->last_action, sizeof(state->last_action), "已触发香港堡垒机发送任务");
            cui_toast(ui, state->last_action, .tone = CUI_TONE_SUCCESS);
          }
        }

        /* 远程机器 1 卡片主体 */
        cui_card(ui, .pad = 20, .gap = 20, .width = CUI_FILL) {
          cui_row(ui, .gap = 20, .align = CUI_ALIGN_STRETCH, .width = CUI_FILL) {

            /* 左侧 2 列 x 2 行 指令网格 */
            cui_column(ui, .width = CUI_GROW(1.1f), .gap = 8) {
              cui_text(ui, "控制指令", .font = 0, .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);

              cui_box(ui, .dir = CUI_GRID, .columns = 2, .gap = 10, .width = CUI_FILL) {
                render_cmd_button(ui, state, 1, "空指令");
                render_cmd_button(ui, state, 2, "执行SQL");
                render_cmd_button(ui, state, 3, "更新代码");
                render_cmd_button(ui, state, 4, "执行命令");
              }
            }

            /* 右侧邮件文件拖拽区 */
            cui_column(ui, .width = CUI_GROW(1.3f), .gap = 8) {
              cui_text(ui, "邮件导入", .font = 0, .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);

              int files = cui_dropzone(ui, "drop-mail",
                .title = "将邮件文件拖到这里",
                .description = "支持 .eml、.msg、.mbox 等格式",
                .icon = CUI_ICON_MAIL,
                .height = 152.0f
              );
              if (files > 0) {
                snprintf(state->last_action, sizeof(state->last_action), "已接收邮件: %s", cui_dropped_file(ui, 0));
                cui_toast(ui, state->last_action, .tone = CUI_TONE_SUCCESS);
              }
            }

          }
        }

        /* =================================================================
         * 模块 2：本地Ubuntu
         * ================================================================= */
        cui_column(ui, .gap = 4, .pad_top = 16, .pad_bottom = 6) {
          cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
            cui_icon(ui, CUI_ICON_SERVER, 18, CUI_COLOR_FOREGROUND);
            cui_heading(ui, "本地Ubuntu", 3);
          }
          cui_secondary(ui, "支持多种通用文件导入与 SQL 脚本批处理任务。");
        }

        cui_card(ui, .pad = 20, .gap = 16, .width = CUI_FILL) {
          cui_row(ui, .gap = 16, .align = CUI_ALIGN_STRETCH, .width = CUI_FILL) {

            /* 左侧：通用文件拖拽 */
            cui_box(ui, .width = CUI_GROW(1)) {
              int files = cui_dropzone(ui, "drop-general",
                .title = "将文件拖到这里",
                .description = "支持多种文件格式",
                .icon = CUI_ICON_FILE_TEXT,
                .height = 152.0f
              );
              if (files > 0) {
                snprintf(state->last_action, sizeof(state->last_action), "收到通用文件: %s", cui_dropped_file(ui, 0));
                cui_toast(ui, state->last_action, .tone = CUI_TONE_INFO);
              }
            }

            /* 右侧：SQL 文件拖拽 */
            cui_box(ui, .width = CUI_GROW(1)) {
              int files = cui_dropzone(ui, "drop-sql",
                .title = "执行 sql 的文件拖到这里",
                .description = "支持 .sql 文件",
                .icon = CUI_ICON_DATABASE,
                .height = 152.0f
              );
              if (files > 0) {
                snprintf(state->last_action, sizeof(state->last_action), "已加载 SQL 脚本: %s", cui_dropped_file(ui, 0));
                cui_toast(ui, state->last_action, .tone = CUI_TONE_SUCCESS);
              }
            }

          }
        }

        /* 状态与操作反馈 */
        if (state->last_action[0]) {
          cui_row(ui, .justify = CUI_JUSTIFY_CENTER) {
            cui_muted(ui, state->last_action);
          }
        }

      }
    }
  }
}

/* =========================================================================
 * 程序入口
 * ========================================================================= */

int main(void) {
  static app_state state = {
    .active_cmd = 1,
    .last_action = "",
    .font_loaded = false
  };

  return cui_app_run(&(cui_app_desc){
    .title = "ABPMS助手",
    .width = 980,
    .height = 720,
    .min_width = 720,
    .min_height = 540
  }, frame, &state);
}