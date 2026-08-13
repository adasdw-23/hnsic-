<div align="center">

# HNSIC — 个人 HNS 比赛系统

**CS 1.6 HNS (Hide & Seek) 比赛系统 · 基于 AMX Mod X + ReGameDLL**

[![AMX Mod X](https://img.shields.io/badge/AMX_Mod_X-1.10+-blue)]()
[![ReGameDLL](https://img.shields.io/badge/ReGameDLL-5.x-orange)]()
[![Version](https://img.shields.io/badge/Version-5.6.1-green)]()
[![Status](https://img.shields.io/badge/Status-BETA-yellow)]()
[![License](https://img.shields.io/badge/License-GPLv3-success)]()

> 一套**个人维护**的 HNS 比赛系统，从 AI 分组 → 模式投票 → 地图选择 → 开赛，全流程覆盖。
> 内置 9 种比赛模式、权限系统、皮肤系统、点位积分、内测账号系统等完整功能。

**维护者 / 作者：LINNA (GTRHNS)**

</div>

---

## 目录

- [这是什么](#这是什么)
- [版本状态](#版本状态)
- [项目结构](#项目结构)
- [比赛主流程](#比赛主流程)
- [对外接口（Native）](#对外接口native)
- [9 种比赛模式](#9-种比赛模式)
- [修复记录](#修复记录)
  - [v5.6.1 — AI 系统全面修复](#v561--ai-系统全面修复)
  - [v5.6.0 — Switch Break 全面修复](#v560--switch-break-全面修复)
  - [2.2 — 菜单与入口整理](#22--菜单与入口整理)
  - [2.1 — AI 分组保存 + 回合制纳入正式链路](#21--ai-分组保存--回合制纳入正式链路)
  - [beta2.0 — 核心功能集中修复](#beta20--核心功能集中修复)
- [配置文件说明](#配置文件说明)
- [启动方式](#启动方式)
- [如何二次开发](#如何二次开发)

---

## 这是什么

HNSIC 是一套**个人维护**的 HNS 比赛系统，核心调度器为 `HnsMatchSystem.sma`（v5.0.0+）。它把比赛从报名到开赛的整条链路串起来：

```
AI 分组 → 模式投票 → 地图选择 → 换图 → 开始比赛
```

系统由 20+ 个插件组成，各插件通过 `hns-match/natives.inc` 注册的 **30+ Native 接口** 互相通信，也对外暴露接口供第三方插件调用。

---

## 版本状态

| 版本 | 说明 | 文档 |
|------|------|------|
| **5.6.1** | AI 系统全面修复 + 训练工具模式限制 | [CHANGELOG.md](CHANGELOG.md) |
| **5.6.0** | Switch Break 全面修复（权限/皮肤/计分） | [CHANGELOG.md](CHANGELOG.md) |
| **2.2** | 菜单与入口整理（点位入口收口） | [UPDATE_2.2.md](UPDATE_2.2.md) |
| **2.1** | AI 分组保存 + 回合制纳入正式链路 | [UPDATE_2.1.md](UPDATE_2.1.md) |
| **beta2.0** | 权限/前缀/点位/皮肤/账号集中修复 | 本文档下方 |

---

## 项目结构

```
hnsic-/
├── cstrike/addons/amxmodx/
│   ├── configs/                    # 全部配置文件
│   │   ├── mixsystem/              # 比赛系统配置
│   │   │   ├── mode/               # 各模式 cfg（training/knife/pub/dm/zombie/match/ascension/vampire）
│   │   │   ├── pointscap/          # 点位积分配置
│   │   │   ├── admin_models.ini    # 管理员模型
│   │   │   ├── hns-arenas.ini      # 竞技场配置
│   │   │   ├── hns-maps.ini        # 地图池
│   │   │   ├── hns-pointscap-zones.ini  # 点位区域
│   │   │   ├── hnsmatch-sql.cfg    # MySQL 配置
│   │   │   ├── jumpstats.cfg       # 跳跃统计
│   │   │   ├── matchsystem.cfg     # 比赛系统主配置
│   │   │   └── player_models.ini   # 皮肤模型
│   │   ├── modules.ini             # 模块启用
│   │   ├── openhns-prefixes.ini    # 聊天前缀
│   │   └── plugins.ini             # 插件加载顺序
│   ├── data/lang/                  # 语言文件（中文）
│   └── scripting/
│       ├── include/                # ★ 接口层
│       │   ├── hns-match/          # 比赛系统核心头文件
│       │   │   ├── natives.inc     # ★ 30+ Native 接口注册
│       │   │   ├── modes/          # 各模式实现（.inl）
│       │   │   ├── gameplay/       # 玩法实现
│       │   │   └── addition/       # 附加功能（菜单/命令/AFK/投降/队长）
│       │   ├── hnsic/              # HNSIC 附加头文件
│       │   ├── HnsicCore.inc       # 核心
│       │   ├── PersistentDataStorage.inc  # PDS 持久化
│       │   ├── hns_matchsystem_api.inc    # ★ 对外 API（统计）
│       │   └── ...                 # AMXX 标准头文件
│       └── HnsMatch*.sma           # 20+ 插件源码
├── CHANGELOG.md                    # 更新日志
├── UPDATE_2.1.md                   # 2.1 更新公告
├── UPDATE_2.2.md                   # 2.2 更新公告
└── README.md
```

---

## 比赛主流程

```
1. AI 分组完成（/join 报名 → AI 自动公平分组）
2. 投票选择比赛模式（MR / Time / 单挑 / 回合制 / 点位 / 吸血）
3. 选择随机地图或指定地图
4. 切换到目标地图
5. 开始比赛
```

**AI 分组结果正式保存**：分组完成后写入 `playerslist`，换图 / 继续开赛时主系统能识别谁在 T、谁在 CT；管理员 `swap` 后队伍归属重新保存。

---

## 对外接口（Native）

### 核心接口（`hns-match/natives.inc`）

| Native | 参数 | 用途 |
|--------|------|------|
| `hns_get_prefix` | szPrefix[], iLen | 获取聊天前缀 `[HNS]` |
| `hns_get_flag_watcher` | 无 | 观察员权限 flag |
| `hns_get_flag_fullwatcher` | 无 | 高级观察员 flag |
| `hns_get_flag_admin` | 无 | 管理员 flag |
| `hns_get_mode` | 无 | 当前模式 |
| `hns_set_mode` | iMode | 切换模式 |
| `hns_get_status` | 无 | 比赛状态 |
| `hns_get_state` | 无 | 模式状态 |
| `hns_get_rules` | 无 | 比赛规则 |
| `hns_isboost` | 无 | 是否 boost 图 |
| `hns_get_score_a/b` | 无 | A/B 队分数 |
| `hns_get_is_team_tt` | 无 | 当前 TT 方 |
| `hns_get_point_distance` | 无 | 点位距离档位 |
| `hns_get_players_distance` | 无 | 决斗玩家距离 |
| `hns_deserter_is_banned` | id | 是否被逃跑禁赛 |
| `hns_deserter_get_remaining` | id | 剩余禁赛秒数 |
| `hns_deserter_get_count` | id | 逃跑次数 |

### 旧版 HD 菜单兼容 Native（转发到模式函数）

| Native | 用途 |
|--------|------|
| `mix_start` / `mix_stop` | 开始 / 停止比赛 |
| `mix_pause` / `mix_unpause` | 暂停 / 恢复 |
| `mix_restartround` | 重开回合 |
| `mix_roundstart` / `mix_roundend` | 回合开始 / 结束 |
| `mix_freezeend` | 冻结结束 |
| `mix_swap` | 交换队伍 |
| `mix_killed` | 击杀回调 |
| `mix_falldamage` | 坠落伤害回调 |

### 对外统计 API（`hns_matchsystem_api.inc`）

| Native | 参数 | 用途 |
|--------|------|------|
| `hns_api_stats_init` | 无 | API 统计模块是否可用 |
| `hns_api_stats_rating` | id | 玩家评分 |
| `hns_api_stats_rank` | id, szLen[], ilen | 玩家段位/称号 |

---

## 9 种比赛模式

| # | 模式 | 说明 | 实现文件 |
|---|------|------|---------|
| 1 | **训练模式** | 无敌 + USP + 钩爪，地图探索 | `modes/mode_training.inl` |
| 2 | **拼刀模式** | 纯刀战，队长拼刀 / 队伍拼刀 / 杯赛拼刀 | `modes/mode_knife.inl` |
| 3 | **公共模式** | 休闲捉迷藏，闪光弹 + 烟雾弹，自动穿透 | `modes/mode_pub.inl` |
| 4 | **死亡竞赛** | 击杀后互换角色，回满血重生 | `modes/mode_dm.inl` |
| 5 | **僵尸模式** | 随机选僵尸，阵营对抗 | `modes/mode_zombie.inl` |
| 6 | **混合赛** | 核心比赛 — MR 制 / 计时制 / 决斗 / 点位积分，支持半场换边 | `modes/mode_mix.inl` |
| 7 | **飞升 / 点位积分** | T 占点位得分，彩色光束可视化 | `modes/mode_ascension.inl` |
| 8 | **吸血鬼** | T 占点位扣 CT 分，扣到零获胜 | `modes/mode_vampire.inl` |
| 9 | **回合制** | 先赢 N 局获胜，可选换边，动态回合数 | `modes/mode_rounds.inl` |

---

## 修复记录

### v5.6.1 — AI 系统全面修复

**🔴 P0 严重修复**

- **补全 AI 状态机**：新增 `aisTaskStateMachine` 轮询任务，自动推进状态转换
  - `AIS_KNIFE_PENDING → AIS_KNIFE_ACTIVE`（拼刀开始）
  - `AIS_KNIFE_ACTIVE → AIS_VOTE_MODE`（拼刀结束，进入投票）
  - `AIS_VOTE_MODE → AIS_LOCKED`（比赛开始，锁定）
  - `AIS_LOCKED → AIS_IDLE`（比赛结束，自动重置）
- **比赛结束自动重置 AI 系统**：检测到 `MATCH_NONE` 时自动调用 `aisCancel()`，确保下次报名正常启动
- **禁用旧 AI 插件 `HnsMatchAITeams.amxx`**：`/join` `/unjoin` 命令已内置到 `HnsMatchSystem.amxx`，避免两套系统命令冲突

**🟠 P1 高优修复**

- `aisAutoStartKnife` 加 **10 次重试上限**（约 30 秒），防止无限循环等待玩家
- `g_iAISSignedCount` 3 处递减加 **负数保护**
- `aisTaskTeamCheck` 中 `static iWaitCount` 改为 **全局变量** `g_iAISWaitCount`，避免任务重启后残留

**🟡 P2 中优修复**

- 报名人数奇偶截断后 `< 2` 人时自动取消报名
- 断线玩家数据清理：报名阶段清除 `AIS_SIGN_TIME`，队伍阶段递减 `g_iAISTotalPlayers`
- 阵营 HUD 任务只在 `AIS_KNIFE_PENDING/ACTIVE` 状态运行，防止泄漏

**🛡️ 训练工具模式限制**

- **14 个训练命令**（`/tr` `/cp` `/tp` `/gc` `/st` `/rp` `/clip` `/weap` `/sc` `/usp` `/awp` `/m4` `/flash` `/showdmg` `/ang`）**仅在训练模式下可用**
- 主菜单训练入口加模式检查，非训练模式显示灰色并提示 `[HNS] 训练工具仅在训练模式下可用！`
- 去掉暂停状态例外（比赛暂停时训练工具同样不可用）

### v5.6.0 — Switch Break 全面修复

**🔴 P0 严重修复**（核心问题：Pawn 的 switch 语句漏写 break 导致 fall-through）

| 文件 | 修复 | 影响 |
|------|------|------|
| `HnsMatchPermSystem.sma` | 12 个 switch 加 **60 处 break** | 修复权限系统完全失效（所有用户权限为 0） |
| `HnsMatchSkinSystem.sma` | 约 30 个 switch 加 break | 修复皮肤/队伍菜单混乱 |
| `mode_ascension.inl` / `mode_vampire.inl` | 计分 switch 加 break | 修复所有点位按 3 人点计分的问题 |
| `HnsMatchAITeams.sma` | 模式名 switch 加 break | 修复无论选什么模式都变成 duel 的问题 |
| `pointscap_editor.inl` | 分数 switch 加 break | 修复所有 zone 分数为 0.5 的问题 |
| `HnsMatchPlayerInfo.sma` | 状态字符串 switch 加 break | 修复所有状态显示为 "Wait players" 的问题 |

**🔧 其他**

- `HnsMatchAITeams.sma` 任务 ID 重构：使用固定常量替代动态任务 ID
- `HnsMatchSystem.sma` 主菜单训练入口加模式检查

### 2.2 — 菜单与入口整理

- **清理点位积分重复入口**：点位编辑器不再放在普通比赛快捷入口；点位分数配置只在进入点位模式时显示
- **比赛设置菜单收口**：非点位模式下不再常驻显示点位目标分数和点位分数配置
- **快捷操作菜单整理**：去掉把点位编辑器直接摆在快捷操作里的做法，避免普通比赛中途误点
- **延续 2.1 的 AI 分组保存逻辑**：换图后主系统仍能识别谁在 T、谁在 CT；`swap` 后重新保存

### 2.1 — AI 分组保存 + 回合制纳入正式链路

- **AI 分组结果正式保存**：分组完成后保存每个玩家所属队伍，同步写入 `playerslist`，管理员 `swap` 后重新保存
- **回合制重新纳入正式比赛模式链路**
- **人数限制修正**：2 人时显示全部模式；大于 2 人时隐藏 `单挑积分`，保留 `回合制`
- **地图选择逻辑收口**：`MR / Time` 随机地图进入正式图池；`单挑积分` 仅在 2 人时参与

### beta2.0 — 核心功能集中修复

| 模块 | 修复内容 |
|------|---------|
| **权限与前缀** | 修复管理员/辅助聊天前缀不显示；修复 `/fuzhu` 错误触发服主认证；统一权限显示名称（服主/管理员/VIP/辅助） |
| **官方管理员菜单** | 修复服主/管理员无法打开官方 AMXX 菜单；补上 `ADMIN_MENU` 权限位；`/menu` 管理员优先进官方菜单；服务器语言改中文 |
| **点位积分** | 缩小默认点位判定框；`3/4/5人点` 用更合理范围；占点改包围盒判定；正式接入 `stay_time` 停留判定；修复 Vampire 点位任务清理 |
| **皮肤系统** | 维持默认皮肤自动发放；额外皮肤按单个发放；保留未解锁皮肤显示；修复管理菜单与皮肤菜单入口冲突 |
| **内测账号系统** | 新增 `/reg` `/login` `/logout` `/account`；已登录时皮肤优先绑定账号；减少盗版环境改名/冒名导致的皮肤数据混乱 |
| **其他** | 修复 AI 分组状态恢复与断线重连；修复 Watcher 与权限 flags 互相覆盖；修复 HUD 冲突；修复 TestBots 对真人误判 |

---

## 配置文件说明

| 文件 | 用途 |
|------|------|
| `mixsystem/matchsystem.cfg` | 比赛系统主配置（CVAR） |
| `mixsystem/mode/*.cfg` | 各模式独立配置 |
| `mixsystem/hns-maps.ini` | 地图池（随机地图 / 指定地图） |
| `mixsystem/hns-pointscap-zones.ini` | 点位积分区域配置 |
| `mixsystem/pointscap/*.ini` | 各图点位配置 |
| `mixsystem/player_models.ini` | 皮肤模型配置 |
| `mixsystem/admin_models.ini` | 管理员模型 |
| `mixsystem/hnsmatch-sql.cfg` | MySQL 连接配置（统计用） |
| `mixsystem/jumpstats.cfg` | 跳跃统计配置 |
| `openhns-prefixes.ini` | 聊天前缀 |
| `plugins.ini` | 插件加载顺序 |

---

## 启动方式

```
CS 1.6 服务器
  → Metamod 加载 AMX Mod X
    → plugins.ini 按顺序加载：
      1. HnsMatchSystem.amxx    ← 核心调度器
      2. HnsMatchSkinSystem.amxx
      3. HnsMatchTraining.amxx  ← 已禁用（AI 系统已内置）
      4. HnsMatchStats.amxx
      5. 其他插件...
```

---

## 如何二次开发

**开发环境：**

1. 安装 AMX Mod X SDK（含 `amxxpc` 编译器与 `include/` 头文件）。
2. 确保能 include 到 `reapi.inc`、`PersistentDataStorage.inc`、`json.inc`。
3. 改完 `.sma` 后编译：`amxxpc HnsMatchSystem.sma`。
4. 在测试服务器 `amxx plugins` 确认加载无报错。

**维护约定：**

- 所有模式实现放在 `hns-match/modes/*.inl`，不要堆进主文件。
- 新增对外接口在 `hns-match/natives.inc` 的 `register_hns_natives()` 中注册。
- 所有配置走 `configs/mixsystem/` 下的 ini / cfg 文件，不要硬编码。
- **Pawn 的 switch 语句务必写 break**（v5.6.0 曾因漏写 break 导致权限系统全面失效）。

**遇到问题：**

- 插件没加载 → 看 `addons/amxmodx/logs/` 下的错误日志，确认模块是否齐全。
- AI 报名异常 → 确认 `HnsMatchAITeams.amxx` 已禁用（命令已内置）。
- 点位判定不准 → 检查 `hns-pointscap-zones.ini` 与 `pointscap/*.ini` 配置。

---

## 开源协议

本项目基于 **GPLv3** 协议开源，自由使用、修改和分发。

---

**HNSIC — Personal HNS Match System** — Built with passion for the CS 1.6 HNS community.
作者：**LINNA (GTRHNS)**

---

<div align="center">

### <span style="color:red">⚠️ 严令禁止倒卖插件 ⚠️</span>

<span style="color:red">**本项目为原创独立开发作品，严禁任何形式的倒卖、转售或商业牟利行为！**</span>

<span style="color:red">源码已开源仅供学习交流与个人使用，未经授权不得将其打包、改头换面后用于收费出售、捆绑销售或二次分发获利。</span>

<span style="color:red">**一经发现，将直接追究相关法律责任，并停止后续更新与技术支持。**</span>

</div>
