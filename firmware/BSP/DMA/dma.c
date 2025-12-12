#include "dma.h"

/* USART DMA缓冲区 */
uint8_t buf_recv[5] = {NULL};
uint8_t buf_send[5] = {0x01,0x02,0x03,0x04,0x05};

/* ADC DMA缓冲区 - 双ADC同步模式 */
uint32_t adc_buffer[ADC_BUFFER_SIZE] = {0};  /* 32位数据：[ADC1_data][ADC0_data] */

void USART0_DMA_Init(void)
{
    dma_parameter_struct dma_struct;
    
    rcu_periph_clock_enable(RCU_DMA0);
    
    dma_deinit(DMA0, DMA_CH4);
    
    dma_struct_para_init(&dma_struct);
    
    dma_struct.direction = DMA_PERIPHERAL_TO_MEMORY;        //�����赽�洢��
    
    dma_struct.memory_addr = (uint32_t)&buf_recv[0];
    
    dma_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;     //�洢�������Լ�
    
    dma_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;        //�洢�����ȣ�8bit
    
    dma_struct.number = 5;
    
    dma_struct.periph_addr = (uint32_t)(&USART_DATA(USART0));
    
    dma_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;    //�����ַ���Լ�
    
    dma_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;    //������ȣ�8bit
    
    dma_struct.priority = DMA_PRIORITY_ULTRA_HIGH;    //�����ȼ�
    
    dma_init(DMA0, DMA_CH4, &dma_struct);
    
    dma_circulation_enable(DMA0, DMA_CH4);     //DMA ѭ��ʹ��
    
    dma_memory_to_memory_disable(DMA0, DMA_CH4);
    
    nvic_irq_enable(DMA0_Channel4_IRQn, 0, 0);
    
    dma_interrupt_enable(DMA0,DMA_CH4,DMA_INT_FTF|DMA_INT_ERR);
    
    dma_channel_enable(DMA0, DMA_CH4);         //DMA ͨ��ʹ��
    
}

void DMA0_Channel4_IRQHandler(void)
{
    if(dma_flag_get(DMA0,DMA_CH4,DMA_FLAG_FTF) != RESET)
    {
        dma_flag_clear(DMA0,DMA_CH4,DMA_FLAG_FTF);
        dma_interrupt_flag_clear(DMA0,DMA_CH4,DMA_INTC_ERRIFC|DMA_INTC_FTFIFC);
        printf("%s\n",buf_recv);
    }
}

/*!
 * \brief   初始化ADC的DMA传输（双ADC同步模式）
 * \details 配置DMA0_CH0，将双ADC同步采样的数据传输到adc_buffer
 *          - ADC0+ADC1同步模式，数据合并在ADC0数据寄存器
 *          - 32位传输：高16位=ADC1数据，低16位=ADC0数据
 *          - 循环模式：缓冲区满后自动覆盖
 */
void ADC_DMA_Init(void)
{
    dma_parameter_struct dma_struct;
    
    /* 使能DMA0时钟 */
    rcu_periph_clock_enable(RCU_DMA0);
    
    /* 复位DMA0通道0 */
    dma_deinit(DMA0, DMA_CH0);
    
    /* 初始化DMA参数结构 */
    dma_struct_para_init(&dma_struct);
    
    /* 配置DMA传输方向：外设到内存 */
    dma_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    
    /* 配置内存地址：adc_buffer数组首地址 */
    dma_struct.memory_addr = (uint32_t)adc_buffer;
    
    /* 内存地址自增使能 */
    dma_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    
    /* 内存数据宽度：32位（双ADC数据合并） */
    dma_struct.memory_width = DMA_MEMORY_WIDTH_32BIT;
    
    /* 传输数据数量 */
    dma_struct.number = ADC_BUFFER_SIZE;
    
    /* 外设地址：ADC0数据寄存器（双ADC模式下包含ADC0+ADC1数据） */
    dma_struct.periph_addr = (uint32_t)(&ADC_RDATA(ADC0));
    
    /* 外设地址不自增 */
    dma_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    
    /* 外设数据宽度：32位 */
    dma_struct.periph_width = DMA_PERIPHERAL_WIDTH_32BIT;
    
    /* DMA优先级：高 */
    dma_struct.priority = DMA_PRIORITY_HIGH;
    
    /* 初始化DMA */
    dma_init(DMA0, DMA_CH0, &dma_struct);
    
    /* 使能循环模式（缓冲区满后自动从头开始） */
    dma_circulation_enable(DMA0, DMA_CH0);
    
    /* 禁用内存到内存模式 */
    dma_memory_to_memory_disable(DMA0, DMA_CH0);
    
    /* 使能DMA通道 */
    dma_channel_enable(DMA0, DMA_CH0);
}

/*!
 * \brief   重启DMA采集（用于欠采样波形采集）
 * \param   sample_count - 要采集的样本数量
 */
void ADC_DMA_Restart(uint32_t sample_count)
{
    /* 禁用DMA通道 */
    dma_channel_disable(DMA0, DMA_CH0);
    
    /* 清除所有DMA标志 */
    dma_flag_clear(DMA0, DMA_CH0, DMA_FLAG_G);
    dma_flag_clear(DMA0, DMA_CH0, DMA_FLAG_FTF);
    dma_flag_clear(DMA0, DMA_CH0, DMA_FLAG_HTF);
    dma_flag_clear(DMA0, DMA_CH0, DMA_FLAG_ERR);
    
    /* 重新设置传输数量 */
    if(sample_count > ADC_BUFFER_SIZE) {
        sample_count = ADC_BUFFER_SIZE;
    }
    dma_transfer_number_config(DMA0, DMA_CH0, sample_count);
    
    /* ⭐ 重置内存地址到缓冲区起始位置 */
    dma_memory_address_config(DMA0, DMA_CH0, (uint32_t)adc_buffer);
    
    /* 重新使能DMA通道 */
    dma_channel_enable(DMA0, DMA_CH0);
}
