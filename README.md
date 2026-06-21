# FPS — UE5.6 第一人称射击游戏

一个基于 Unreal Engine 5.6 的多人联机第一人称射击游戏项目，支持 LAN 局域网对战、AI 敌人自动填充、完整武器系统和行为树驱动的 AI 系统。

## 🎮 游戏特性

### 核心玩法

- **多人联机**：基于 OnlineSubsystemNull 的 LAN 局域网对战，支持房间创建/搜索/加入
- **AI 自动填充**：根据 Session 最大人数自动补齐 AI 敌人，玩家加入时自动替换 AI
- **限时对战**：可配置的游戏时长（默认 300 秒），倒计时结束后展示计分板
- **滚雪球计分**：击杀得分 = 基础分 + 存活时间加成 + 连杀加成
- **重生系统**：玩家死亡后显示击杀者名称，点击按钮请求重生

### 武器系统

- **双武器槽位**：主武器 + 副武器，支持切换和丢弃
- **射击模式**：半自动 / 全自动，可配置射击间隔
- **后坐力系统**：内置 6 种后坐力图案预设（AK47/M4A1/Pistol/SMG/LMG/Sniper）+ 自定义图案
- **扩散系统**：移动扩散 + 射击累积扩散 + 瞄准减半 + AI 倍率修正
- **弹药管理**：弹匣 / 备弹系统，支持换弹（区分普通/空仓换弹动画）
- **武器检视**：检视动画播放，期间锁定其他操作
- **物理丢弃**：武器丢弃后带物理冲量和旋转，支持重新拾取
- **自动销毁**：未拾取的武器超时自动销毁

### 角色系统

- **第一人称视角**：独立的 FP 手臂网格 + TP 身体网格，OnlyOwnerSee/OwnerNoSee 可见性控制
- **移动状态机**：行走(400) / 冲刺(700) / 瞄准(200) 三种速度，冲刺与瞄准互斥
- **血量系统**：可配置最大血量，伤害浮动，死亡时禁用碰撞/移动并广播事件
- **脚步声**：基于速度的定时脚步声，冲刺/行走不同间隔
- **布娃娃物理**：死亡后 TP 网格切换为物理模拟

### AI 系统

- **行为树驱动**：基于 UE BehaviorTree 的完整 AI 决策系统
- **感知系统**：视觉(20m, 60° FOV) + 听觉(30m)，支持敌我识别
- **战斗行为**：瞄准目标 → 判断距离/视线 → 开火/换弹/巡逻
- **瞄准偏移**：基于命中概率的骨骼瞄准 + 未命中时环形随机偏转
- **AI 名称池**：预设名称列表，死亡归还/重生分配

### UI 系统

- **主菜单**：标题界面 → 大厅（房间列表/创建/加入/刷新） → 创建房间（地图选择）
- **游戏内 HUD**：定时刷新机制、动态准星创建、命中提示、击杀指示器
- **准星系统**：四角/圆形/无三种形状，支持中心点、扩散响应、颜色/参数完全可配置
- **命中提示**：X 形 4 条线对角排列，命中时出现并淡出消失
- **击杀指示器**：动态生成图标，延迟后淡出，连续击杀重置计时器
- **重生界面**：显示击杀者名称 + 重生按钮
- **游戏结束**：RichText 展示前三名计分板 + 返回大厅按钮
- **设置界面**：音量/分辨率/全屏 + 准星配置分页 + 实时预览
- **存档系统**：音量/分辨率/全屏/玩家名/准星配置持久化存储

## 🏗️ 项目结构

```
Source/FPS/
├── FPS.Build.cs              — 模块依赖配置
├── FPS.h / FPS.cpp           — 模块入口 + LogFPS 日志类别
├── Public/
│   ├── Character/            — 角色类
│   │   ├── FPS_CharacterBase — 共享基类：血量/武器库存/移动速度/脚步声/死亡
│   │   ├── FP_Character      — 玩家角色：FP摄像机/EnhancedInput/冲刺瞄准互斥
│   │   └── FPS_AICharacter   — AI角色：行为树接口/瞄准旋转/射击/换弹
│   ├── Weapon/               — 武器系统
│   │   ├── WeaponActor       — 武器Actor：装备/射击/换弹/检视/丢弃/网络复制
│   │   ├── WeaponDataConfig  — 数值配置DataAsset：后坐力/伤害/弹药/射击/扩散
│   │   ├── WeaponAnimationConfig — 动画配置DataAsset：FP/TP AnimBP + 蒙太奇
│   │   ├── WeaponAudioConfig — 音频配置DataAsset：射击/换弹/检视/命中音效
│   │   ├── WeaponRecoilHandler — 后坐力处理器：图案索引/速度衰减/相机驱动
│   │   ├── WeaponSpreadHandler — 扩散处理器：移动扩散/射击累积/衰减
│   │   ├── WeaponMeshManager — 网格管理器：挂载/卸载/弹夹状态/可见性
│   │   ├── WeaponRecoilPresets — 后坐力图案预设数据
│   │   ├── PickUpComponent   — 拾取碰撞组件：SphereComponent + Overlap检测
│   │   └── AIAttackConfig    — AI攻击配置：瞄准骨骼/命中概率/开火距离
│   ├── AI/                   — AI系统
│   │   ├── FPS_AIController  — AI控制器：感知/行为树/Blackboard键值
│   │   ├── BTTask_FireWeapon — 开火任务：瞄准偏移/射击模式/弹药检查
│   │   ├── BTTask_ReloadWeapon — 换弹任务
│   │   ├── BTTask_SetAiming  — 瞄准状态切换任务
│   │   ├── BTTask_SetSprinting — 冲刺状态切换任务
│   │   ├── BTTask_FaceTarget — 面向目标任务：持续旋转+角度判定
│   │   ├── BTTask_FindRandomPatrolPoint — 随机巡逻点任务
│   │   ├── BTTask_ClearBlackboardKey — 清除Blackboard键任务
│   │   ├── BTService_UpdateTarget — 目标更新服务：存活/位置/距离/视线/交战判定
│   │   ├── BTService_UpdateAmmo — 弹药状态更新服务
│   │   └── BTDecorator_HasAmmo — 弹药检查装饰器
│   ├── UI/                   — UI系统
│   │   ├── FPSBaseMenuWidget — 菜单基类：子窗口缓存/父窗口导航/ZOrder
│   │   ├── FPSHUDWidget      — HUD基类：定时刷新/准星/命中提示/击杀指示器
│   │   ├── FPSCrosshairWidget — 准星控件：四角/圆形/无 + 扩散响应
│   │   ├── FPSHitMarkerWidget — 命中提示控件：X形标记 + 淡出
│   │   ├── FPSKillIndicatorWidget — 击杀指示器：动态图标 + 延迟淡出
│   │   ├── FPSRespawnWidget  — 重生界面：击杀者名 + 重生按钮
│   │   ├── FPSGameEndWidget  — 游戏结束：RichText计分板 + 返回大厅
│   │   ├── FPSTitleScreen    — 标题界面：开始/设置/退出 + 玩家名输入
│   │   ├── FPSSettingsScreen — 设置界面：音量/分辨率/全屏 + 准星配置
│   │   ├── FPSCreateRoomScreen — 创建房间：房间名 + 地图选择
│   │   └── FPSSaveGame       — 存档系统：设置/准星配置持久化
│   ├── FPSGameMode.h         — 游戏模式：计时/计分/重生/AI管理
│   ├── FPSPlayerState.h      — 玩家状态：Kills/Deaths/Score/时间同步
│   ├── FPSPlayerController.h — 玩家控制器：Client RPC/武器委托管理
│   ├── FPSGameInstance.h     — 游戏实例：在线Session管理/UI ZOrder
│   ├── FPSMainMenuWidget.h   — 主菜单大厅：房间列表/创建/加入/刷新
│   ├── FPSRoomEntryWidget.h  — 房间条目：数据/点击/选中状态
│   └── FPSMainMenuPlayerController.h — 主菜单控制器：标题界面/显示设置
└── Private/                  — 对应的 .cpp 实现文件
```

## 🔧 技术栈

| 类别  | 技术                                            |
| --- | --------------------------------------------- |
| 引擎  | Unreal Engine 5.6                             |
| 语言  | C++ (UBT)                                     |
| 输入  | Enhanced Input System                         |
| AI  | BehaviorTree + AIPerception (Sight + Hearing) |
| 动画  | Animation Montage + AnimBlueprint             |
| 网络  | Server-Authoritative Replication + RPC        |
| 在线  | OnlineSubsystemNull (LAN)                     |
| UI  | UMG (UserWidget + BindWidget)                 |
| 物理  | Niagara VFX + Physics Simulation              |
| 渲染  | DX12 + Lumen + Virtual Shadow Maps            |

## 📦 模块依赖

```
Core, CoreUObject, Engine, InputCore, EnhancedInput,
AIModule, StateTreeModule, GameplayStateTreeModule,
UMG, Slate, SlateCore, Niagara, NavigationSystem,
OnlineSubsystem, OnlineSubsystemUtils
```

## 🎯 网络架构

- **服务器权威**：所有游戏逻辑在服务器执行，客户端仅处理输入和表现
- **属性复制**：Server → Client 自动复制（DOREPLIFETIME）
- **RPC 通信**：Server RPC（客户端→服务器）、Client RPC（服务器→拥有者客户端）、NetMulticast（服务器→所有客户端）
- **客户端预测**：射击时本地扣弹药 + 射线检测预测 VFX，服务器权威确认

## 🎨 命名规范

| 前缀    | 类型          | 示例                 |
| ----- | ----------- | ------------------ |
| `A`   | AActor 派生类  | `AWeaponActor`     |
| `U`   | UObject 派生类 | `UFPSHUDWidget`    |
| `F`   | 结构体         | `FPlayerScoreData` |
| `E`   | 枚举          | `EWeaponFireMode`  |
| `C_`  | 角色蒙太奇       | `C_FireAnimation`  |
| `W_`  | 武器蒙太奇       | `W_FireAnimation`  |
| `TP_` | 第三人称蒙太奇     | `TP_FireAnimation` |

## 📝 配置文件

| 文件                         | 说明                                                             |
| -------------------------- | -------------------------------------------------------------- |
| `Config/DefaultEngine.ini` | 渲染(DX12/Lumen)、碰撞通道、GameMode、OnlineSubsystem(Null)、LAN Session |
| `Config/DefaultInput.ini`  | Enhanced Input 默认配置、鼠标灵敏度                                      |

## 🚀 构建方式

1. 打开 `FPS.sln`（Visual Studio）
2. 选择 `Development Editor` 配置
3. 构建 FPS 项目
4. 或在 Unreal Editor 中 Ctrl+Alt+F11 热重载

# 
