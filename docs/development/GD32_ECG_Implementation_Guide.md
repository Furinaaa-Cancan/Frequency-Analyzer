# GD32 硬件端 ECG 信号模式实现指南

## 1. 串口命令协议

网页端会发送以下命令到GD32：

```
TYPE:SINE    // 切换到正弦波模式
TYPE:ECG     // 切换到心电ECG模式
```

## 2. 硬件实现方案

### 方案A：外部ECG输入模式（推荐）

**适用场景：** 将真实的心电信号接入PA6，系统处理后从PB1输出

#### 主要修改代码：

```c
// ============== 在 main.c 开头添加 ==============
typedef enum {
    SIGNAL_TYPE_SINE = 0,
    SIGNAL_TYPE_ECG = 1
} SignalType_t;

SignalType_t g_signal_type = SIGNAL_TYPE_SINE;

// ============== 在串口接收中断或命令处理函数中添加 ==============
void process_uart_command(char* cmd_buffer) {
    // 信号类型切换命令
    if (strncmp(cmd_buffer, "TYPE:SINE", 9) == 0) {
        g_signal_type = SIGNAL_TYPE_SINE;
        printf("OK: Signal type changed to SINE\r\n");
        return;
    }
    else if (strncmp(cmd_buffer, "TYPE:ECG", 8) == 0) {
        g_signal_type = SIGNAL_TYPE_ECG;
        printf("OK: Signal type changed to ECG\r\n");
        return;
    }
    
    // ... 其他命令处理（SWEEP, FREQ, LED等）
}

// ============== 修改DAC输出部分 ==============
void timer_update_handler(void) {
    if (g_signal_type == SIGNAL_TYPE_SINE) {
        // 正弦波模式：使用DDS生成正弦波
        phase_accumulator += phase_increment;
        if (phase_accumulator >= SINE_TABLE_SIZE) {
            phase_accumulator -= SINE_TABLE_SIZE;
        }
        uint16_t dac_value = sine_table[phase_accumulator];
        dac_data_set(DAC0, DAC_ALIGN_12B_R, dac_value);
    }
    else if (g_signal_type == SIGNAL_TYPE_ECG) {
        // ECG模式：DAC输出中间电平（或不输出）
        // 因为ECG是外部输入信号，不需要DAC生成
        dac_data_set(DAC0, DAC_ALIGN_12B_R, 2048);  // 输出1.5V中间电平
    }
    
    // ADC采集继续执行（不管什么模式都采集PA6和PB1）
    // 数据发送格式保持不变
}

// ============== ADC采集和数据发送保持不变 ==============
// 不管是正弦波还是ECG，都继续采集PA6和PB1
// 数据格式保持：WAVE:freq,input,output
```

### 方案B：DAC生成模拟ECG波形（用于测试）

**适用场景：** 没有真实ECG信号源，用DAC生成一个模拟心电波形用于测试

#### ECG波形表：

```c
// ============== 在 main.c 开头添加ECG波形查找表 ==============
// 简化的心电波形（120个采样点，约1秒心跳，心率60 BPM）
const uint16_t ecg_waveform[120] = {
    // 基线 (0-15)
    2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
    2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
    
    // P波 - 心房除极 (16-23)
    2048, 2050, 2055, 2065, 2075, 2070, 2060, 2050,
    
    // PR段 (24-31)
    2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
    
    // Q波 - 小幅下降 (32-35)
    2045, 2040, 2035, 2038,
    
    // R波 - 快速上升尖峰 (36-43)
    2048, 2100, 2250, 2500, 2800, 3100, 3300, 3400,
    
    // S波 - 快速下降 (44-47)
    3200, 2900, 2400, 2100,
    
    // ST段 (48-55)
    2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
    
    // T波 - 心室复极 (56-67)
    2050, 2060, 2075, 2090, 2105, 2115, 2120, 2115,
    2105, 2090, 2070, 2055,
    
    // 基线 - 等待下一个心跳 (68-119)
    2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
    2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
    2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
    2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
    2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
    2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048,
    2048, 2048, 2048, 2048
};

uint16_t ecg_index = 0;

// ============== 修改定时器中断处理 ==============
void timer_update_handler(void) {
    if (g_signal_type == SIGNAL_TYPE_SINE) {
        // 正弦波模式
        phase_accumulator += phase_increment;
        if (phase_accumulator >= SINE_TABLE_SIZE) {
            phase_accumulator -= SINE_TABLE_SIZE;
        }
        uint16_t dac_value = sine_table[phase_accumulator];
        dac_data_set(DAC0, DAC_ALIGN_12B_R, dac_value);
    }
    else if (g_signal_type == SIGNAL_TYPE_ECG) {
        // ECG模式：播放ECG波形
        dac_data_set(DAC0, DAC_ALIGN_12B_R, ecg_waveform[ecg_index]);
        ecg_index++;
        if (ecg_index >= 120) {
            ecg_index = 0;  // 循环播放，模拟持续心跳
        }
    }
}
```

## 3. 完整实现步骤

### 步骤1: 添加全局变量
```c
// 在main.c开头添加
typedef enum {
    SIGNAL_TYPE_SINE = 0,
    SIGNAL_TYPE_ECG = 1
} SignalType_t;

SignalType_t g_signal_type = SIGNAL_TYPE_SINE;
```

### 步骤2: 修改串口命令处理
在你现有的串口接收处理函数中添加TYPE命令的识别：
```c
void uart_receive_handler(void) {
    // ... 接收数据到cmd_buffer
    
    if (strncmp(cmd_buffer, "TYPE:", 5) == 0) {
        if (strcmp(&cmd_buffer[5], "SINE") == 0) {
            g_signal_type = SIGNAL_TYPE_SINE;
            printf("OK: SINE mode\r\n");
        }
        else if (strcmp(&cmd_buffer[5], "ECG") == 0) {
            g_signal_type = SIGNAL_TYPE_ECG;
            printf("OK: ECG mode\r\n");
        }
    }
    // ... 其他命令
}
```

### 步骤3: 修改DAC输出逻辑
在定时器中断或DAC更新函数中：
```c
void update_dac_output(void) {
    if (g_signal_type == SIGNAL_TYPE_SINE) {
        // 你原有的正弦波生成代码
        dac_data_set(DAC0, DAC_ALIGN_12B_R, sine_value);
    }
    else {
        // ECG模式
        // 方案A: 不输出（外部输入）
        dac_data_set(DAC0, DAC_ALIGN_12B_R, 2048);
        
        // 方案B: 输出模拟ECG波形
        // dac_data_set(DAC0, DAC_ALIGN_12B_R, ecg_waveform[ecg_index++]);
    }
}
```

### 步骤4: ADC采集保持不变
```c
// 不管什么模式，都继续采集PA6和PB1
// 数据发送格式保持：WAVE:freq,input_adc,output_adc
```

## 4. 硬件连接

### 正弦波模式（原有方式）
```
GD32 DAC (PA4/PB0) --> 系统输入
系统输入 (PA6) --> ADC采集
系统输出 (PB1) --> ADC采集
```

### ECG模式
```
外部ECG信号源 --> PA6 (ADC输入通道)
系统处理后 --> PB1 (ADC输入通道)
DAC --> 不使用（或输出中间电平）
```

## 5. 测试方法

### 正弦波模式测试
1. 网页选择"正弦波信号"
2. 硬件接收到 `TYPE:SINE`
3. DAC输出正弦波
4. 正常扫频测试

### ECG模式测试
1. 网页选择"心电ECG信号"
2. 硬件接收到 `TYPE:ECG`
3. DAC停止输出正弦波
4. PA6接入心电信号（或使用方案B的模拟波形）
5. 网页实时显示心电波形

## 6. 常见问题

**Q: ECG模式下还需要发送频率信息吗？**
A: 可以发送固定值（如100Hz）或0，网页会自动隐藏频率显示。

**Q: ADC采样率需要调整吗？**
A: 心电信号建议使用250Hz以上采样率，如果你已经在用更高采样率就不需要调整。

**Q: 数据格式需要改变吗？**
A: 不需要，保持现有的 `WAVE:freq,input,output` 格式即可。

## 7. 示例代码位置

假设你的项目结构是：
```
main.c          // 主要修改这个文件
dac.c/dac.h     // DAC相关函数
adc.c/adc.h     // ADC相关函数（不需要修改）
usart.c/usart.h // 串口相关函数
```

重点修改：
- `main.c`: 添加信号类型变量和命令处理
- 定时器中断处理函数: 添加信号类型判断
- 串口接收中断: 添加TYPE命令识别

## 8. 调试建议

1. 先实现命令接收，用printf确认收到TYPE命令
2. 再实现DAC切换，用示波器确认DAC输出变化
3. 最后测试完整流程，确认网页能正确显示波形

---

**作者：** 翁灿
**日期：** 2025-11-26
**版本：** 1.0
