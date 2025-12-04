#include "adc.h"

/*!
 * \brief   双ADC同步模式初始化（频响测试模式）
 * \details 配置ADC0+ADC1同步采样待测电路系统两端正弦信号：
 *          - ADC0_CH6 (PA6): 输入参考信号K·sin(ωt) - 来自U1A运放输出
 *          - ADC1_CH9 (PB1): 输出测量信号K₁·sin(ωt+θ) - 来自第三级运放输出
 *          - 规则并行模式，由TIMER3_CH3触发
 *          - DMA自动传输数据到adc_buffer
 *          - 采样时间：13.5周期（适合10Hz-1kHz信号）
 * 
 * \note 硬件连接（根据实际电路图）：
 *       GD32 → SPI0(PA5/PA7/PA4) → DAC5311(VOUT) → U1A → U1B → 第三级运放 → PB1
 *                                                    ↓                      ↓
 *                                                   PA6                    PB1
 *                                              (ADC0_CH6输入K)      (ADC1_CH9输出K₁)
 */
void ADC_Dual_Init(void)
{
    /* 1. 使能时钟 */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_periph_clock_enable(RCU_ADC1);
    
    /* 2. 配置ADC时钟：72MHz/12 = 6MHz（在2MHz-14MHz范围内） */
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV12);
    
    /* 3. 配置GPIO为模拟输入 */
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_6);  /* ADC0_CH6: 输入参考K (U1A输出) */
    gpio_init(GPIOB, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_1);  /* ADC1_CH9: 输出测量K₁ (第三级运放输出) */
    
    /* 4. 配置ADC工作模式：规则并行模式（双ADC同步） */
    adc_mode_config(ADC_DAUL_REGULAL_PARALLEL);  /* ADC0和ADC1同步转换规则通道 */
    
    /* ========== 配置ADC0（主ADC）========== */
    /* 禁用连续转换（由定时器触发） */
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
    
    /* 禁用扫描模式（单通道） */
    adc_special_function_config(ADC0, ADC_SCAN_MODE, DISABLE);
    
    /* 数据右对齐 */
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    
    /* 配置规则通道：1个通道 */
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 1);
    
    /* 配置ADC0通道6 (PA6)，采样时间13.5周期 */
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_6, ADC_SAMPLETIME_13POINT5);
    
    /* 外部触发源：TIMER3_CH3 */
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_EXTTRIG_REGULAR_T3_CH3);
    
    /* 使能外部触发 */
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);
    
    /* ========== 配置ADC1（从ADC）========== */
    /* 禁用连续转换（由ADC0触发同步） */
    adc_special_function_config(ADC1, ADC_CONTINUOUS_MODE, DISABLE);
    
    /* 禁用扫描模式（单通道） */
    adc_special_function_config(ADC1, ADC_SCAN_MODE, DISABLE);
    
    /* 数据右对齐 */
    adc_data_alignment_config(ADC1, ADC_DATAALIGN_RIGHT);
    
    /* 配置规则通道：1个通道 */
    adc_channel_length_config(ADC1, ADC_REGULAR_CHANNEL, 1);
    
    /* 配置ADC1通道9 (PB1)，采样时间13.5周期 */
    adc_regular_channel_config(ADC1, 0, ADC_CHANNEL_9, ADC_SAMPLETIME_13POINT5);
    
    /* ADC1在某些GD32芯片上也需要配置外部触发源（即使在并行模式下）*/
    adc_external_trigger_source_config(ADC1, ADC_REGULAR_CHANNEL, ADC0_1_EXTTRIG_REGULAR_T3_CH3);
    
    /* 使能ADC1的外部触发 */
    adc_external_trigger_config(ADC1, ADC_REGULAR_CHANNEL, ENABLE);
    
    /* ========== 使能DMA ========== */
    adc_dma_mode_enable(ADC0);  /* 使能ADC0的DMA请求 */
    
    /* ========== 使能和校准ADC ========== */
    /* 使能ADC0 */
    adc_enable(ADC0);
    delay_ms(1);  /* 等待ADC稳定 */
    adc_calibration_enable(ADC0);  /* 校准ADC0 */
    
    /* 使能ADC1 */
    adc_enable(ADC1);
    delay_ms(1);  /* 等待ADC稳定 */
    adc_calibration_enable(ADC1);  /* 校准ADC1 */
    
    /* 注意：ADC现在等待TIMER3_CH3的外部触发事件 */
    /* 首次手动触发启动转换（可选，用于测试） */
    // adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
}

/* ========== 旧的单通道ADC初始化（保留，不使用） ========== */
void ADC_Init(void)//ͨ��1    PA1     ����ģʽ    ����ģʽ    �ڲ���������      adcʱ��Ϊ9M
{
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV12);
    gpio_init(GPIOA,GPIO_MODE_AIN,GPIO_OSPEED_50MHZ,GPIO_PIN_6);
    adc_mode_config(ADC_MODE_FREE);//����ģʽ
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE); //����ɨ��ģʽ
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);   //���ݶ���ģʽ���Ҷ���ģʽ
    adc_channel_length_config(ADC0,ADC_REGULAR_CHANNEL,1);  //ͨ����������
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_6, ADC_SAMPLETIME_1POINT5);
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE);
    adc_external_trigger_config(ADC0,ADC_REGULAR_CHANNEL,ENABLE);//�ⲿ��������
//    adc_tempsensor_vrefint_enable();//ʹ���¶ȴ�����
    
    adc_enable(ADC0);//ADCʹ��
    delay_ms(1);
    adc_calibration_enable(ADC0);//У׼ʹ��
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
    
}



double ADC_Read(uint32_t adc_periph)
{
    int data=0;
    int i=0;
    for(i=0;i<50;i++)
    {
        
        while(!adc_flag_get(ADC0, ADC_FLAG_EOC));                       // �ȴ��������
        adc_flag_clear(ADC0, ADC_FLAG_EOC);                             // ���������־  
        data += adc_regular_data_read(adc_periph);
    }
    data = data/50;
//    printf("%x\r\n",data);
    return data;
//     return (double)data/4096*3.3;
    
}


