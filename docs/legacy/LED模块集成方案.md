# LED模块集成方案

## 📋 任务概述

将**作业4的LED控制模块**集成到**Project 2硬件版（伯德图分析仪）**中。

---

## 🔍 现状分析

### Project 2 (伯德图分析仪)
- **功能**: 10Hz-1kHz频率响应测试
- **现有LED**: 简单的GPIO控制（仅开关功能）
- **定时器**: TIMER2用于DDS信号生成（50kHz）
- **串口**: USART0，115200波特率
- **目录结构**:
  ```
  firmware/
  ├── BSP/
  │   ├── LED/          # 现有简单LED驱动
  │   ├── LED_CTRL/     # 空目录
  │   ├── TIMER_LED/    # 空目录
  │   └── ...
  └── USER/
      └── main.c        # 主程序
  ```

### 作业4 (LED控制系统)
- **功能**: 完整的LED控制系统
- **LED模式**: 6种（OFF、Flow、Breath、Blink、Specific、Rotate、Alarm）
- **核心功能**:
  - ✅ 软件PWM亮度控制（10KHz，无闪烁）
  - ✅ 灵活参数设置（间隔1-30000ms、亮度0-100%）
  - ✅ 报警和错误指示功能
  - ✅ 外部中断按键控制
  - ✅ 串口命令控制
- **定时器**: TIMER1，20KHz中断
- **目录结构**:
  ```
  作业4/
  ├── BSP/
  │   ├── LED/          # 完整LED驱动（533行）
  │   ├── TIMER/        # TIMER1配置
  │   └── USART/        # 串口控制
  └── USER/
      └── main.c        # 命令解析
  ```

---

## 🎯 集成目标

### 功能集成
1. ✅ **保留伯德图分析仪的核心功能**（不受影响）
2. ✅ **添加LED状态指示功能**
   - 测量进行中：呼吸灯
   - 测量完成：绿色LED常亮
   - 错误发生：报警模式（1-2Hz快闪）
3. ✅ **通过串口控制LED**
   - 新增LED控制命令（不影响现有命令）
4. ✅ **可选：按键控制LED模式**（外部中断）

### 技术要求
- ⚠️ **定时器冲突处理**：TIMER1 vs TIMER2
- ⚠️ **LED引脚定义**：确认硬件兼容性
- ⚠️ **串口命令扩展**：避免命令冲突

---

## 📐 集成方案（推荐）

### 方案一：完整集成（推荐）⭐

**优点**: 功能最完整，LED状态指示丰富  
**缺点**: 需要修改定时器配置，工作量中等

#### 步骤1: 复制LED驱动模块
```bash
# 替换现有的LED模块
cp -r 作业4/BSP/LED/* firmware/BSP/LED/
```

#### 步骤2: 复制TIMER模块（用于LED）
```bash
# 复制到LED_CTRL目录（避免与现有TIMER冲突）
cp -r 作业4/BSP/TIMER/* firmware/BSP/TIMER_LED/
```

#### 步骤3: 修改定时器配置
- **现状**: Project 2使用TIMER2（50kHz）用于DDS
- **需求**: LED需要TIMER1（20KHz）用于PWM
- **方案**: 两个定时器共存，各司其职
  - TIMER2 → DDS信号生成（50kHz）
  - TIMER1 → LED PWM控制（20kHz）

#### 步骤4: 修改main.c
添加以下初始化代码：
```c
/* 在main()函数中添加 */

// LED控制系统初始化
LED_Init();
printf("[OK] LED system initialized.\r\n");

// TIMER1初始化（用于LED PWM，20kHz）
TIM1_Init(71, 49);  // 72MHz / (71+1) / (49+1) = 20kHz
printf("[OK] TIMER1 initialized for LED PWM (20kHz).\r\n");

// 可选：外部中断按键控制
// EXTI_Config();
// printf("[OK] Button interrupt configured (PA0).\r\n");

// 设置默认LED模式为OFF
LED_Set_Mode(LED_MODE_OFF);
```

#### 步骤5: 扩展串口命令
在`main.c`的命令解析部分添加：
```c
/* LED控制命令（与作业4保持一致）*/
else if (strncmp(cmd, "LED:", 4) == 0) {
    uint8_t mode = atoi(cmd + 4);
    if (mode <= 6) {
        LED_Set_Mode((led_mode_t)mode);
        printf("LED mode set to: %d\r\n", mode);
    }
}
else if (strncmp(cmd, "LED_BRIGHT:", 11) == 0) {
    uint8_t brightness = atoi(cmd + 11);
    if (brightness <= 100) {
        LED_Set_Brightness(brightness);
        printf("LED brightness: %d%%\r\n", brightness);
    }
}
else if (strncmp(cmd, "LED_FREQ:", 9) == 0) {
    uint16_t freq = atoi(cmd + 9);
    if (freq >= 1 && freq <= 30000) {
        LED_Set_Interval(freq);
        printf("LED interval: %d ms\r\n", freq);
    }
}
```

#### 步骤6: 添加状态指示逻辑
在测量函数中添加LED状态指示：
```c
/* 在AutoSweep()函数中添加 */

// 开始扫频时：设置呼吸灯模式
LED_Set_Mode(LED_MODE_BREATH);
LED_Set_Brightness(50);

// 扫频完成时：LED常亮
LED_Set_Mode(LED_MODE_OFF);
LED3_H;  // 绿色LED常亮表示完成

// 发生错误时：报警模式
if (error_occurred) {
    LED_Set_Alarm_Mode();  // 1-2Hz快闪
}
```

---

### 方案二：简化集成（快速）

**优点**: 集成快速，改动最小  
**缺点**: 功能简化，只保留基本LED控制

#### 步骤1: 只复制LED驱动（不含TIMER）
```bash
cp -r 作业4/BSP/LED/* firmware/BSP/LED/
```

#### 步骤2: 使用现有TIMER2
修改`led.c`中的时间基准：
- 将20KHz改为50KHz（复用TIMER2）
- 调整PWM频率和分频系数

#### 步骤3: 简化功能
- 去掉PWM亮度控制（固定亮度100%）
- 只保留基本模式切换
- 不使用外部中断

---

## 🔧 具体实施步骤（方案一）

### 第1步: 备份当前代码
```bash
cd "/Users/wengcan/Desktop/Project 2_硬件版"
cp -r firmware firmware_backup
```

### 第2步: 复制LED模块
```bash
# LED驱动
cp 作业4/BSP/LED/led.c firmware/BSP/LED/
cp 作业4/BSP/LED/led.h firmware/BSP/LED/

# TIMER驱动（用于LED）
cp 作业4/BSP/TIMER/timer.c firmware/BSP/TIMER_LED/
cp 作业4/BSP/TIMER/timer.h firmware/BSP/TIMER_LED/
```

### 第3步: 修改Keil工程
在Keil工程中：
1. 确认`BSP/LED/led.c`已添加
2. 添加`BSP/TIMER_LED/timer.c`到工程
3. 确认头文件路径包含`BSP/TIMER_LED`

### 第4步: 修改main.c
需要修改的关键部分：
1. 添加`#include "timer.h"`（LED的TIMER）
2. 在`main()`中添加LED和TIMER1初始化
3. 在主循环中调用`LED_Process()`（如果需要）
4. 添加串口命令解析

### 第5步: 修改中断服务程序
在`timer.c`（TIMER_LED版本）中：
```c
void TIMER1_UP_IRQHandler(void) {
    if(timer_interrupt_flag_get(TIMER1, TIMER_INT_FLAG_UP) != RESET) {
        timer_interrupt_flag_clear(TIMER1, TIMER_INT_FLAG_UP);
        
        // 调用LED处理函数
        LED_Process();
    }
}
```

### 第6步: 编译测试
1. 编译工程（F7）
2. 检查是否有错误
3. 下载到开发板（F8）
4. 测试LED功能

---

## ⚠️ 注意事项

### 硬件兼容性
确认LED引脚定义是否一致：
- **作业4**: LED在GPIOB (PB11-PB15)
- **Project 2**: 需要检查硬件原理图

如果引脚不同，修改`led.h`中的引脚定义。

### 定时器优先级
```c
// TIMER1（LED）: 优先级设为1（较高）
nvic_irq_enable(TIMER1_UP_IRQn, 1, 0);

// TIMER2（DDS）: 优先级设为2（较低）
nvic_irq_enable(TIMER2_IRQn, 2, 0);
```

### 串口命令避免冲突
- 现有命令: `HELP`, `DEBUG`, `FREQ:xxx`, `MEASURE`, `SWEEP`, `STATUS`
- 新增命令: `LED:x`, `LED_BRIGHT:x`, `LED_FREQ:x`
- ✅ 无冲突

---

## 📊 功能对比

| 功能 | 方案一（完整） | 方案二（简化） | Project 2原版 |
|------|---------------|---------------|--------------|
| 6种LED模式 | ✅ | ❌（仅3种） | ❌ |
| PWM亮度控制 | ✅ | ❌ | ❌ |
| 报警模式 | ✅ | ✅ | ❌ |
| 串口控制 | ✅ | ✅ | ❌ |
| 按键控制 | ✅ | ❌ | ❌ |
| 工作量 | 中等 | 低 | - |
| 代码改动 | 较多 | 少 | - |

---

## 🎯 推荐实施路径

### 阶段1: 基础集成（1小时）
- [ ] 复制LED和TIMER模块
- [ ] 修改Keil工程配置
- [ ] 基础编译测试

### 阶段2: 功能集成（1小时）
- [ ] 修改main.c（初始化、命令解析）
- [ ] 添加中断服务程序
- [ ] 功能测试

### 阶段3: 状态指示（30分钟）
- [ ] 添加测量状态LED指示
- [ ] 添加错误报警功能
- [ ] 完整测试

---

## ❓ 你的选择

请告诉我你想采用哪种方案：

1. **方案一：完整集成**（推荐）
   - 功能最全，LED状态指示丰富
   - 工作量：约2.5小时

2. **方案二：简化集成**
   - 快速集成，基本功能
   - 工作量：约1小时

3. **自定义方案**
   - 告诉我你想要哪些功能

---

## 📞 下一步

确定方案后，我会帮你：
1. ✅ 自动执行文件复制
2. ✅ 修改所有必要的代码
3. ✅ 配置Keil工程
4. ✅ 提供测试命令

**请告诉我你的选择！** 🚀
