#include "dac5311.h"
#include "adc.h"

/*!
 * \brief   DAC5311 SPI引脚定义
 * \details 使用SPI0硬件接口（根据电路图）
 *          PA5 = SPI0_SCK
 *          PA7 = SPI0_MOSI (DIN - 发送数据到DAC)
 *          PA4 = CS片选 (SYNC)
 */
#define DAC5311_SPI             SPI0
#define DAC5311_SPI_CLK         RCU_SPI0

#define DAC5311_GPIO_PORT       GPIOA
#define DAC5311_GPIO_CLK        RCU_GPIOA

#define DAC5311_SCK_PIN         GPIO_PIN_5   /* SPI0_SCK:  PA5 */
#define DAC5311_MOSI_PIN        GPIO_PIN_7   /* SPI0_MOSI: PA7 (DIN) */
#define DAC5311_CS_PIN          GPIO_PIN_4   /* CS片选:    PA4 (SYNC) */

/*!
 * \brief   片选控制宏
 */
#define DAC5311_CS_LOW()        gpio_bit_reset(DAC5311_GPIO_PORT, DAC5311_CS_PIN)
#define DAC5311_CS_HIGH()       gpio_bit_set(DAC5311_GPIO_PORT, DAC5311_CS_PIN)

/*!
 * \brief   初始化SPI0硬件接口和DAC5311
 * \details 配置SPI0硬件模块，用于驱动外部DAC5311芯片
 *          PA5 = SPI0_SCK, PA7 = SPI0_MOSI, PA4 = CS
 */
void DAC5311_Init(void)
{
    spi_parameter_struct spi_init_struct;
    
    /* 1. 使能时钟 */
    rcu_periph_clock_enable(DAC5311_GPIO_CLK);
    rcu_periph_clock_enable(DAC5311_SPI_CLK);
    
    /* 2. 配置GPIO */
    /* SCK(PA5)和MOSI(PA7)配置为复用推挽输出 */
    gpio_init(DAC5311_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, 
              DAC5311_SCK_PIN | DAC5311_MOSI_PIN);
    
    /* CS(PA4)配置为通用推挽输出 */
    gpio_init(DAC5311_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, 
              DAC5311_CS_PIN);
    
    /* 默认CS拉高（未选中）*/
    DAC5311_CS_HIGH();
    
    /* 3. 配置SPI参数 */
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;  /* 全双工 */
    spi_init_struct.device_mode          = SPI_MASTER;                /* 主机模式 */
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;        /* 8位数据帧 */
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;    /* CPOL=0, CPHA=0 */
    spi_init_struct.nss                  = SPI_NSS_SOFT;              /* 软件CS控制 */
    spi_init_struct.prescale             = SPI_PSC_32;                /* 分频32: 72MHz/32=2.25MHz */
    spi_init_struct.endian               = SPI_ENDIAN_MSB;            /* MSB先发送 */
    
    spi_init(DAC5311_SPI, &spi_init_struct);
    
    /* 4. 使能SPI0 */
    spi_enable(DAC5311_SPI);
    
    /* 5. 初始化DAC5311为中点值 */
    DAC5311_Write(128);
}

/*!
 * \brief   通过硬件SPI发送一个字节
 * \param   data 要发送的字节
 * \return  接收到的字节（DAC5311不返回数据，忽略）
 */
static uint8_t DAC5311_SPI_SendByte(uint8_t data)
{
    /* 等待发送缓冲区空 */
    while(RESET == spi_i2s_flag_get(DAC5311_SPI, SPI_FLAG_TBE));
    
    /* 发送数据 */
    spi_i2s_data_transmit(DAC5311_SPI, data);
    
    /* 等待接收缓冲区非空 */
    while(RESET == spi_i2s_flag_get(DAC5311_SPI, SPI_FLAG_RBNE));
    
    /* 读取接收到的数据（清除RBNE标志）*/
    return (uint8_t)spi_i2s_data_receive(DAC5311_SPI);
}

/*!
 * \brief   写入DAC5311数据（通过硬件SPI）
 * \param   data 8位DAC数据（0-255）
 * \details DAC5311是12位DAC，需要将8位数据扩展为12位
 *          16位SPI格式：[D11-D4(高8位)][D3-D0(低4位) + 4个填充位]
 *          将8位数据左移4位，得到12位数据（0-4080，约满量程）
 *          输出电压 = (dac_value/4095) * Vref
 */
void DAC5311_Write(uint8_t data)
{
    /* 将8位数据（0-255）转换为12位（0-4080）：左移4位 */
    uint16_t dac_value = ((uint16_t)data) << 4;
    
    /* 1. 拉低CS，选中DAC5311 */
    DAC5311_CS_LOW();
    
    /* 2. 发送16位数据：高8位 + 低8位 */
    DAC5311_SPI_SendByte((uint8_t)(dac_value >> 8));   /* 高8位：D11-D4 */
    DAC5311_SPI_SendByte((uint8_t)(dac_value & 0xFF)); /* 低8位：D3-D0 + 填充 */
    
    /* 3. 拉高CS，取消选中（数据锁存）*/
    DAC5311_CS_HIGH();
    
    /* 4. 短暂延迟，确保DAC5311完成转换 */
    for(volatile int i = 0; i < 10; i++);
}

/* 保留原来的SPI中断处理函数（暂时注释掉，改用定时器中断） */
#if 0
void SPI0_IRQHandler(void)
 {
	 uint16_t v;
	 static uint16_t data = 0;
 	if(spi_i2s_flag_get(SPI0,SPI_FLAG_TBE)!=RESET)
 	{
 		if(spi_i2s_interrupt_flag_get(SPI0,SPI_I2S_INT_FLAG_TBE)!=RESET)
 		{
			v = ADC_Read(ADC0);
			printf("%d\r\n",v);
			
				
			data++;
			if(data == 256)
			{
				data=0;
			}
			spi_transmit(sin(2*3.1415926*data/256)*128+128);
			delay_ms(1);
 		}
 	}
 }
#endif
