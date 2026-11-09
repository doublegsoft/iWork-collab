#include <cui/cui.h>
#include <cui/cui_app.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * 1. 真实金融流水数据结构定义 (对应 fin_tran 表 30 个字段)
 * ========================================================================= */

typedef struct {
  char fin_tran_id[32];          /* 流水主键 ID */
  char fin_tran_type_id[32];     /* 流水类型 ID */
  char svc_id[32];               /* 服务 ID */
  char related_id[32];           /* 关联标识 (如 'WO SYNC') */
  char fin_tran_type_ind[8];     /* 交易类型标识 (如 'AD') */
  double cur_amt;                /* 本次发生金额 */
  double tot_amt;                /* 累计金额 */
  char created_date[32];         /* 创建时间 */
  char freeze_sw[4];             /* 冻结状态 ('Y' / 'N') */
  char freeze_userid[16];        /* 冻结操作员 */
  char freeze_date[32];          /* 冻结时间 */
  char arrears_date[32];         /* 欠费起算日 */
  char correction_sw[4];         /* 冲正开关 */
  char old_fin_tran_sw[4];       /* 历史流水开关 */
  char debit_in_next_bill_sw[4]; /* 下期账单借记 */
  char show_on_bill_sw[4];       /* 账单展示开关 */
  char offset_sw[4];             /* 抵扣冲销标识 */
  char bill_id[32];              /* 关联账单 ID */
  char accounting_date[32];      /* 会计记账日期 */
  char txfr_to_gl_date[32];      /* 总账过账日期 */
  char gl_dist_sts[8];           /* 总账分录状态 (如 'X') */
  char sched_dist_date[32];      /* 计划分录日期 */
  int bal_ctrl_grp_id;           /* 余额控制组 ID */
  char created_by[16];           /* 创建人 */
  char modified_by[16];          /* 修改人 */
  char modified_date[32];        /* 修改时间 */
  char timestamp[32];            /* 时间戳 */
  char gl_batch_code[16];        /* 总账批次代码 */
  int gl_batch_no;               /* 总账批次号 */
  char rowid_pk[32];             /* 行物理标识 */

  /* UI 辅助比对状态 */
  bool has_diff;                 /* 整条记录是否存在差异 */
  bool amt_diff;                 /* 金额是否不一致 */
  bool gl_diff;                  /* 总账状态是否不一致 */
} fin_tran;

typedef struct {
  char type[32];
  char tx_id[32];
  char field[32];
  char sys_a[64];
  char sys_b[64];
  char desc[128];
  cui_tone tone;
} diff_item;

#define MAX_RECORDS 64
#define MAX_DIFFS   32

typedef struct {
  bool font_loaded;
  int sys_a_index;
  int db_a_index;
  int sys_b_index;
  int db_b_index;
  int date_range_index;

  /* 真实对账数据缓存 */
  fin_tran records_a[MAX_RECORDS];
  fin_tran records_b[MAX_RECORDS];
  int record_count;

  /* 比对结果 */
  diff_item diffs[MAX_DIFFS];
  int diff_count;
  float match_rate; /* 一致率 (0.0 ~ 1.0) */
} app_state;

/* =========================================================================
 * 2. 模拟数据加载与自动对账引擎
 * ========================================================================= */

static void init_mock_fin_tran_data(app_state* state) {
  state->record_count = 6;

  /* 基础流水母版 */
  fin_tran base_template = {
    .fin_tran_id = "437006718029",
    .fin_tran_type_id = "778529673826",
    .svc_id = "7495739984",
    .related_id = "WO SYNC",
    .fin_tran_type_ind = "AD",
    .cur_amt = -400.00,
    .tot_amt = 0.00,
    .created_date = "2022-06-17 22:17:20",
    .freeze_sw = "Y",
    .freeze_userid = "SYS",
    .freeze_date = "2022-06-17 22:17:20",
    .arrears_date = "2022-06-17 00:00:00",
    .correction_sw = "N",
    .old_fin_tran_sw = "N",
    .debit_in_next_bill_sw = "N",
    .show_on_bill_sw = "N",
    .offset_sw = "N",
    .bill_id = "502114406116",
    .accounting_date = "2022-06-17 00:00:00",
    .txfr_to_gl_date = "2022-06-17 00:00:00",
    .gl_dist_sts = "X",
    .sched_dist_date = "2022-06-17 00:00:00",
    .bal_ctrl_grp_id = 6349,
    .created_by = "SYS",
    .modified_by = "SYS",
    .modified_date = "2022-06-20 02:05:00",
    .timestamp = "2022-06-18 03:36:44",
    .gl_batch_code = "CMGLDL",
    .gl_batch_no = 6354,
    .rowid_pk = "AAAm3SAALAAAAjrAAF",
    .has_diff = false,
    .amt_diff = false,
    .gl_diff = false
  };

  /* 为系统 A 和系统 B 初始化 6 条一一对应的流水 */
  for (int i = 0; i < state->record_count; ++i) {
    state->records_a[i] = base_template;
    state->records_b[i] = base_template;

    snprintf(state->records_a[i].fin_tran_id, 32, "%llu", 437006718029ULL + i);
    snprintf(state->records_b[i].fin_tran_id, 32, "%llu", 437006718029ULL + i);

    snprintf(state->records_a[i].bill_id, 32, "%llu", 502114406116ULL + i);
    snprintf(state->records_b[i].bill_id, 32, "%llu", 502114406116ULL + i);

    state->records_a[i].cur_amt = -400.00 - (i * 125.50);
    state->records_b[i].cur_amt = state->records_a[i].cur_amt;
  }

  /* 制造真实业务差异测试用例 */
  /* 差异用例 1: 第 2 条流水 (TXN...030) 金额不一致 */
  state->records_b[1].cur_amt = -600.00;

  /* 差异用例 2: 第 4 条流水 (TXN...032) 总账状态不一致 */
  strcpy(state->records_b[3].gl_dist_sts, "E"); /* E 代表 Error */
}

/* 执行真实双系统流水比对 */
static void run_fin_tran_compare(app_state* state) {
  state->diff_count = 0;
  int identical_count = 0;

  for (int i = 0; i < state->record_count; ++i) {
    fin_tran* a = &state->records_a[i];
    fin_tran* b = &state->records_b[i];

    a->has_diff = false;
    a->amt_diff = false;
    a->gl_diff = false;
    b->has_diff = false;
    b->amt_diff = false;
    b->gl_diff = false;

    bool diff_found = false;

    /* 比对发生金额 cur_amt */
    if (fabs(a->cur_amt - b->cur_amt) > 0.001) {
      diff_found = true;
      a->amt_diff = true;
      b->amt_diff = true;

      if (state->diff_count < MAX_DIFFS) {
        diff_item* d = &state->diffs[state->diff_count++];
        strcpy(d->type, "金额不符");
        strcpy(d->tx_id, a->fin_tran_id);
        strcpy(d->field, "cur_amt");
        snprintf(d->sys_a, sizeof(d->sys_a), "%.2f", a->cur_amt);
        snprintf(d->sys_b, sizeof(d->sys_b), "%.2f", b->cur_amt);
        strcpy(d->desc, "交易发生额存在差异");
        d->tone = CUI_TONE_DANGER;
      }
    }

    /* 比对总账状态 gl_dist_sts */
    if (strcmp(a->gl_dist_sts, b->gl_dist_sts) != 0) {
      diff_found = true;
      a->gl_diff = true;
      b->gl_diff = true;

      if (state->diff_count < MAX_DIFFS) {
        diff_item* d = &state->diffs[state->diff_count++];
        strcpy(d->type, "总账状态");
        strcpy(d->tx_id, a->fin_tran_id);
        strcpy(d->field, "gl_dist_sts");
        snprintf(d->sys_a, sizeof(d->sys_a), "%s", a->gl_dist_sts);
        snprintf(d->sys_b, sizeof(d->sys_b), "%s", b->gl_dist_sts);
        strcpy(d->desc, "总账过账分录状态不一致");
        d->tone = CUI_TONE_WARNING;
      }
    }

    if (diff_found) {
      a->has_diff = true;
      b->has_diff = true;
    } else {
      identical_count++;
    }
  }

  /* 计算一致率 */
  state->match_rate = (state->record_count > 0) ? ((float)identical_count / (float)state->record_count) : 1.0f;
}

/* =========================================================================
 * 3. 中文字体 Fallback 加载
 * ========================================================================= */

static void ensure_cjk_font(cui_ctx* ui, app_state* state) {
  if (state->font_loaded) {
    return;
  }
  state->font_loaded = true;

  const char* cjk_paths[] = {
    "/System/Library/Fonts/PingFang.ttc",
    "/System/Library/Fonts/Hiragino Sans GB.ttc",
    "/System/Library/Fonts/STHeiti Light.ttc",
    "C:/Windows/Fonts/msyh.ttc",
    "/Library/Fonts/Arial Unicode.ttf"
  };

  for (size_t i = 0; i < sizeof(cjk_paths) / sizeof(cjk_paths[0]); ++i) {
    FILE* f = fopen(cjk_paths[i], "rb");
    if (f) {
      fclose(f);
      cui_font font_id = cui_font_load_file(ui, cjk_paths[i]);
      if (font_id >= 0) {
        cui_font_add_fallback(ui, 0, font_id);
        break;
      }
    }
  }
}

/* =========================================================================
 * 4. 专属金融流水 Tile 卡片渲染 (单张卡片容纳密集财务字段)
 * ========================================================================= */

static void render_fin_tran_card(cui_ctx* ui, const fin_tran* r) {
  /* 左侧状态色条判断 */
  cui_color stripe_color;
  if (r->amt_diff) {
    stripe_color = CUI_RGB(0xef4444); /* 警示红 */
  } else if (r->gl_diff) {
    stripe_color = CUI_RGB(0xf59e0b); /* 警示黄 */
  } else if (r->has_diff) {
    stripe_color = CUI_RGB(0x1a56db); /* 提示蓝 */
  } else {
    stripe_color = CUI_RGB(0x10b981); /* 正常绿 */
  }

  cui_row(ui,
    .width = CUI_FILL,
    .bg = r->has_diff ? CUI_RGB(0xf8faff) : CUI_COLOR_SURFACE,
    .radius = 8,
    .pad = 0,
    .gap = 0
  ) {
    /* 1. 左侧指示条 */
    cui_box(ui, .width = CUI_PX(4), .height = CUI_FILL, .bg = stripe_color) {}

    /* 2. 主体凭单信息 */
    cui_column(ui, .width = CUI_FILL, .pad = 10, .gap = 6) {

      /* 头部行：流水主键 ID + 类型 + 冻结标识 */
      cui_row(ui, .width = CUI_FILL, .align = CUI_ALIGN_CENTER) {
        cui_box(ui, .bg = CUI_COLOR_SURFACE_MUTED, .radius = 4, .pad_x = 5, .pad_y = 2) {
          cui_text(ui, r->fin_tran_id, .font = CUI_FONT_MEDIUM, .size = 11);
        }
        cui_badge(ui, r->fin_tran_type_ind, .tone = CUI_TONE_INFO, .size = CUI_SIZE_SM);
        cui_text(ui, r->created_date, .size = 11, .color = CUI_COLOR_TEXT_SECONDARY);

        cui_spacer(ui);

        /* 冻结开关指示 */
        if (strcmp(r->freeze_sw, "Y") == 0) {
          cui_badge(ui, "已冻结", .tone = CUI_TONE_NEUTRAL, .size = CUI_SIZE_SM);
        }
      }

      /* 中间 2x2 业务字段紧凑排布 */
      cui_row(ui, .width = CUI_FILL, .gap = 12) {
        /* 服务 ID */
        cui_column(ui, .width = CUI_GROW(1), .gap = 1) {
          cui_text(ui, "服务号 (svc_id)", .size = 10, .color = CUI_COLOR_TEXT_SECONDARY);
          cui_text(ui, r->svc_id, .size = 11, .font = CUI_FONT_MEDIUM);
        }
        /* 账单 ID */
        cui_column(ui, .width = CUI_GROW(1), .gap = 1) {
          cui_text(ui, "账单 (bill_id)", .size = 10, .color = CUI_COLOR_TEXT_SECONDARY);
          cui_text(ui, r->bill_id, .size = 11, .font = CUI_FONT_MEDIUM);
        }
        /* 总账分录状态 */
        cui_column(ui, .width = CUI_PX(64), .gap = 1) {
          cui_text(ui, "总账状态", .size = 10, .color = CUI_COLOR_TEXT_SECONDARY);
          if (r->gl_diff) {
            cui_badge(ui, r->gl_dist_sts, .tone = CUI_TONE_DANGER, .size = CUI_SIZE_SM);
          } else {
            cui_badge(ui, r->gl_dist_sts, .tone = CUI_TONE_SUCCESS, .size = CUI_SIZE_SM);
          }
        }
      }

      cui_separator(ui);

      /* 底部财务行：发生额 cur_amt 与 结余 tot_amt */
      cui_row(ui, .width = CUI_FILL, .align = CUI_ALIGN_END) {
        cui_row(ui, .gap = 4, .align = CUI_ALIGN_CENTER) {
          cui_text(ui, "结余:", .size = 11, .color = CUI_COLOR_TEXT_SECONDARY);
          char tot_buf[32];
          snprintf(tot_buf, sizeof(tot_buf), "¥ %.2f", r->tot_amt);
          cui_text(ui, tot_buf, .size = 11, .tabular = true);
        }

        cui_spacer(ui);

        /* 发生额 (差异时红色胶囊警示) */
        char cur_buf[32];
        snprintf(cur_buf, sizeof(cur_buf), "%.2f", r->cur_amt);
        if (r->amt_diff) {
          cui_box(ui, .bg = CUI_RGB(0xfef2f2), .radius = 4, .pad_x = 6, .pad_y = 2) {
            cui_row(ui, .gap = 2, .align = CUI_ALIGN_END) {
              cui_text(ui, "发生额 ¥", .size = 11, .color = CUI_RGB(0xdc2626));
              cui_text(ui, cur_buf, .tabular = true, .color = CUI_RGB(0xdc2626), .font = CUI_FONT_DISPLAY, .size = 14);
            }
          }
        } else {
          cui_row(ui, .gap = 2, .align = CUI_ALIGN_END) {
            cui_text(ui, "发生额 ¥", .size = 11, .color = CUI_COLOR_TEXT_SECONDARY);
            cui_text(ui, cur_buf, .tabular = true, .font = CUI_FONT_DISPLAY, .size = 14);
          }
        }
      }

    }
  }
}

/* 渲染整列流水流容器 */
static void render_fin_stream(cui_ctx* ui, const fin_tran* rows, int count, const char* stream_id) {
  cui_scroll(ui, .id = stream_id, .height = CUI_PX(430), .width = CUI_FILL, .gap = 8, .fade_edges = true) {
    for (int i = 0; i < count; ++i) {
      render_fin_tran_card(ui, &rows[i]);
    }
  }
}

/* =========================================================================
 * 5. 主渲染帧循环
 * ========================================================================= */

static void frame(cui_ctx* ui, void* user) {
  app_state* state = (app_state*)user;
  ensure_cjk_font(ui, state);

  cui_scroll(ui, .id = "main_scroll", .width = CUI_FILL, .height = CUI_FILL, .fade_edges = true) {
    cui_row(ui, .justify = CUI_JUSTIFY_CENTER, .align = CUI_ALIGN_START, .pad = 20) {
      cui_column(ui, .width = CUI_FILL, .max_width = 1180, .gap = 16) {

        /* ---------------- Header 栏 ---------------- */
        cui_card(ui, .pad_x = 18, .pad_y = 12, .width = CUI_FILL) {
          cui_row(ui, .width = CUI_FILL, .align = CUI_ALIGN_CENTER, .gap = 12) {
            cui_row(ui, .gap = 10, .align = CUI_ALIGN_CENTER) {
              cui_icon(ui, CUI_ICON_MONITOR, 26, CUI_COLOR_FOREGROUND);
              cui_text(ui, "核心流水账 (fin_tran) 双系统自动核对", .font = CUI_FONT_DISPLAY, .size = 18);
            }

            cui_spacer(ui);

            static const char* ranges[] = { "记账日期: 2022-06-17 ~ 2022-06-20" };
            cui_box(ui, .width = CUI_PX(270)) {
              cui_select(ui, "date_range", ranges, 1, &state->date_range_index);
            }

            if (cui_button(ui, "重新对账", .variant = CUI_BUTTON_PRIMARY, .icon = CUI_ICON_PLAY)) {
              run_fin_tran_compare(state);
              cui_toast(ui, "已完成最新一期 fin_tran 流水核对", .tone = CUI_TONE_SUCCESS);
            }
          }
        }

        /* ---------------- 三栏对比区域 ---------------- */
        cui_row(ui, .width = CUI_FILL, .gap = 16, .align = CUI_ALIGN_STRETCH) {

          /* 左栏：系统 A (生产库 prod_a) */
          cui_card(ui, .width = CUI_GROW(1.35f), .pad = 16, .gap = 12) {
            cui_row(ui, .gap = 12, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
              static const char* sys_a_opts[] = { "系统 A (Oracle)" };
              cui_box(ui, .width = CUI_PX(140)) {
                cui_select(ui, "sys_a_select", sys_a_opts, 1, &state->sys_a_index);
              }
              static const char* db_a_opts[] = { "库名: prod_a.fin_tran" };
              cui_box(ui, .width = CUI_PX(180)) {
                cui_select(ui, "db_a_select", db_a_opts, 1, &state->db_a_index);
              }
            }
            render_fin_stream(ui, state->records_a, state->record_count, "stream_sys_a");
          }

          /* 中栏：对账结果仪表盘 */
          cui_card(ui, .width = CUI_PX(180), .pad = 16, .gap = 14, .align = CUI_ALIGN_CENTER) {
            cui_text(ui, "对账结果", .font = CUI_FONT_MEDIUM, .size = 15);

            /* 一致率 */
            cui_column(ui, .align = CUI_ALIGN_CENTER, .gap = 2) {
              char rate_buf[16];
              snprintf(rate_buf, sizeof(rate_buf), "%.0f%%", (double)(state->match_rate * 100.0f));
              cui_text(ui, rate_buf, .font = CUI_FONT_DISPLAY, .size = 36, .color = CUI_RGB(0x1a56db));
              cui_text(ui, "一致率", .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
            }

            /* 进度条 */
            cui_progress(ui, state->match_rate, .width = CUI_PX(120), .height = 6);

            cui_separator(ui);

            /* 差异笔数 */
            cui_column(ui, .align = CUI_ALIGN_CENTER, .gap = 2) {
              cui_row(ui, .gap = 4, .align = CUI_ALIGN_END) {
                char diff_buf[16];
                snprintf(diff_buf, sizeof(diff_buf), "%d", state->diff_count);
                cui_text(ui, diff_buf, .font = CUI_FONT_DISPLAY, .size = 28);
                cui_text(ui, "笔", .font = CUI_FONT_MEDIUM, .size = 14);
              }
              cui_text(ui, "异常差异", .size = 13, .color = CUI_COLOR_TEXT_SECONDARY);
            }

            cui_separator(ui);

            /* 图例 */
            cui_column(ui, .gap = 8, .align = CUI_ALIGN_START, .pad_left = 6) {
              cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
                cui_icon(ui, CUI_ICON_CIRCLE, 10, CUI_RGB(0x10b981));
                cui_text(ui, "全字段一致", .size = 12);
              }
              cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
                cui_icon(ui, CUI_ICON_CIRCLE, 10, CUI_RGB(0xef4444));
                cui_text(ui, "金额 cur_amt 异常", .size = 12);
              }
              cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
                cui_icon(ui, CUI_ICON_CIRCLE, 10, CUI_RGB(0xf59e0b));
                cui_text(ui, "总账 gl_dist_sts 异常", .size = 12);
              }
            }
          }

          /* 右栏：系统 B (信创库 prod_b) */
          cui_card(ui, .width = CUI_GROW(1.35f), .pad = 16, .gap = 12) {
            cui_row(ui, .gap = 12, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
              static const char* sys_b_opts[] = { "系统 B (GaussDB)" };
              cui_box(ui, .width = CUI_PX(140)) {
                cui_select(ui, "sys_b_select", sys_b_opts, 1, &state->sys_b_index);
              }
              static const char* db_b_opts[] = { "库名: prod_b.fin_tran" };
              cui_box(ui, .width = CUI_PX(180)) {
                cui_select(ui, "db_b_select", db_b_opts, 1, &state->db_b_index);
              }
            }
            render_fin_stream(ui, state->records_b, state->record_count, "stream_sys_b");
          }

        }

        /* ---------------- 底部差异详情表 ---------------- */
        cui_card(ui, .pad = 16, .gap = 12, .width = CUI_FILL) {
          cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
            cui_icon(ui, CUI_ICON_CIRCLE_ALERT, 16, CUI_RGB(0x1a56db));
            cui_text(ui, "差异详情明细 (自动定位异常字段)", .font = CUI_FONT_MEDIUM, .size = 15);
          }

          static const cui_table_column diff_cols[6] = {
            { "差异类型",   CUI_PX(110), CUI_TEXT_LEFT,  false },
            { "流水号 ID",  CUI_PX(140), CUI_TEXT_LEFT,  false },
            { "异常字段",   CUI_PX(120), CUI_TEXT_LEFT,  false },
            { "系统 A 原值", CUI_PX(140), CUI_TEXT_LEFT,  false },
            { "系统 B 目标值",CUI_PX(140), CUI_TEXT_LEFT,  false },
            { "业务诊断",   CUI_FILL,    CUI_TEXT_LEFT,  false },
          };

          cui_table(ui, "tbl_fin_diffs", diff_cols, 6) {
            for (int i = 0; i < state->diff_count; ++i) {
              const diff_item* d = &state->diffs[i];
              cui_table_row(ui, false);

              cui_table_cell(ui);
              cui_badge(ui, d->type, .tone = d->tone, .size = CUI_SIZE_SM);

              cui_table_cell(ui);
              cui_text(ui, d->tx_id, .font = CUI_FONT_MEDIUM);

              cui_table_cell_text(ui, d->field);
              cui_table_cell_text(ui, d->sys_a);
              cui_table_cell_text(ui, d->sys_b);

              cui_table_cell(ui);
              cui_text(ui, d->desc, .color = CUI_COLOR_TEXT_SECONDARY);
            }
          }
        }

      }
    }
  }
}

/* =========================================================================
 * 6. 程序启动入口
 * ========================================================================= */

int main(void) {
  static app_state state = {
    .font_loaded = false,
    .sys_a_index = 0,
    .db_a_index = 0,
    .sys_b_index = 0,
    .db_b_index = 0,
    .date_range_index = 0
  };

  /* 初始化真实业务数据与自动对账 */
  init_mock_fin_tran_data(&state);
  run_fin_tran_compare(&state);

  return cui_app_run(&(cui_app_desc){
    .title = "金融核心流水双系统自动对账系统",
    .width = 1200,
    .height = 800,
    .min_width = 980,
    .min_height = 640
  }, frame, &state);
}