/**
  ******************************************************************************
  * @file    bsp_LINKEC20_USART.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   重现c库printf函数到usart端口
  ******************************************************************************

  ******************************************************************************
  */

#include "./Bsp/usart/bsp_linkec20_usart.h"
uint8_t ec20_Rxbuffer[100];//debug串口接收缓冲区
uint8_t ec20_Rxcounter;//debug串口接收字节计数器
//volatile uint8_t ec20_Rxbuffer[200];//debug串口接收缓冲区
//volatile uint8_t ec20_Rxcounter;//debug串口接收字节计数器
//volatile uint8_tec20_ReceiveState=0;//debug串口接收状态

uint8_t i =0;

/**
 * @brief  配置嵌套向量中断控制器NVIC
 * @param  无
 * @retval 无
 */
static void NVIC_Configuration_EC20(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 嵌套向量中断控制器组选择 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* 配置USART为中断源 */
    NVIC_InitStructure.NVIC_IRQChannel = LINKEC20_USART_IRQ;
    /* 抢断优先级为1 */
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    /* 子优先级为1 */
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    /* 使能中断 */
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    /* 初始化配置NVIC */
    NVIC_Init(&NVIC_InitStructure);
}


/**
	* @brief  USART1 GPIO 配置,工作模式配置。115200 8-N-1
	* @param  无
	* @retval 无
	*/
void LINKEC20_USART_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    /* Enable GPIO clock */
    RCC_AHB1PeriphClockCmd( LINKEC20_USART_RX_GPIO_CLK|LINKEC20_USART_TX_GPIO_CLK, ENABLE);

    /* Enable UART clock */
    RCC_APB2PeriphClockCmd(LINKEC20_USART_CLK, ENABLE);

    /* Connect PXx to USARTx_Tx*/
    GPIO_PinAFConfig(LINKEC20_USART_RX_GPIO_PORT,LINKEC20_USART_RX_SOURCE, LINKEC20_USART_RX_AF);

    /* Connect PXx to USARTx_Rx*/
    GPIO_PinAFConfig(LINKEC20_USART_TX_GPIO_PORT,LINKEC20_USART_TX_SOURCE,LINKEC20_USART_TX_AF);

    /* Configure USART Tx as alternate function  */
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;

    GPIO_InitStructure.GPIO_Pin = LINKEC20_USART_TX_PIN  ;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(LINKEC20_USART_TX_GPIO_PORT, &GPIO_InitStructure);

    /* Configure USART Rx as alternate function  */
    GPIO_InitStructure.GPIO_Pin = LINKEC20_USART_RX_PIN;
    GPIO_Init(LINKEC20_USART_RX_GPIO_PORT, &GPIO_InitStructure);

    /* 配置串_USART 模式 */
    USART_InitStructure.USART_BaudRate = LINKEC20_USART_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(LINKEC20_USART, &USART_InitStructure);

    /* 嵌套向量中断控制器NVIC配置 */
    NVIC_Configuration_EC20();


    /* 使能串口空闲中断 */
    //USART_ITConfig(LINKEC20_USART, USART_IT_IDLE, ENABLE);

    /* 使能串口接收中断 */
    //USART_ITConfig(LINKEC20_USART, USART_IT_RXNE, ENABLE);

    /* 使能串口 */
    USART_Cmd(LINKEC20_USART, ENABLE);

    
}

    /* 复位串口状态，慎用，
		PE：奇偶校验错误 (Parity error)
		FE：帧错误 (Framing error)
		ORE：上溢错误 (Overrun error)
		USART_IT_RXNE：读取数据寄存器不为空 中断
		*/
void USART2_NewIstr(USART_TypeDef * pUSARTx)
{
    uint8_t Data;
    if (USART_GetFlagStatus(pUSARTx, USART_FLAG_PE) != RESET)
    {
        USART_ReceiveData(pUSARTx);
        USART_ClearFlag(pUSARTx, USART_FLAG_PE);
    }

    if (USART_GetFlagStatus(pUSARTx, USART_FLAG_ORE) != RESET)
    {
        USART_ReceiveData(pUSARTx);
        USART_ClearFlag(pUSARTx, USART_FLAG_ORE);
    }

    if (USART_GetFlagStatus(pUSARTx, USART_FLAG_FE) != RESET)
    {
        USART_ReceiveData(pUSARTx);
        USART_ClearFlag(pUSARTx, USART_FLAG_FE);
    }

    if(USART_GetITStatus(pUSARTx, USART_IT_RXNE) != RESET)
    {
        USART_ClearFlag(pUSARTx, USART_FLAG_RXNE);
        USART_ClearITPendingBit(pUSARTx, USART_IT_RXNE);
        Data = USART_ReceiveData(pUSARTx);
    }
}

/*****************  发送一个字符 **********************/
void Usart_SendByte( USART_TypeDef * pUSARTx, uint8_t ch)
{
    /* 发送一个字节数据到USART */
    USART_SendData(pUSARTx,ch);

    /* 等待发送数据寄存器为空 */
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);
}

/*****************  发送字符串 **********************/
void Usart_SendString( USART_TypeDef * pUSARTx, char *str)
{
    unsigned int k=0;
    do
    {
        Usart_SendByte( pUSARTx, *(str + k) );
        k++;
    } while(*(str + k)!='\0');

    /* 等待发送完成 */
    while(USART_GetFlagStatus(pUSARTx,USART_FLAG_TC)==RESET)
    {}
       //USART_ClearITPendingBit(LINKEC20_USART,USART_IT_RXNE);
}

/*****************  发送数组 **********************/
void Usart_SendBlock( USART_TypeDef * pUSARTx, uint8_t *Rxbuffer ,uint8_t  Rxcounter)
{
    i=0;
    while(Rxcounter--)
    {
        USART_SendData(pUSARTx,Rxbuffer[i++]);
    }
    /* 等待发送完成 */
    while(USART_GetFlagStatus(pUSARTx,USART_FLAG_TC)==RESET)
    {}
    Rxcounter=0;
}


/*****************  发送一个16位数 **********************/
void Usart_SendHalfWord( USART_TypeDef * pUSARTx, uint16_t ch)
{
    uint8_t temp_h, temp_l;

    /* 取出高八位 */
    temp_h = (ch&0XFF00)>>8;
    /* 取出低八位 */
    temp_l = ch&0XFF;

    /* 发送高八位 */
    USART_SendData(pUSARTx,temp_h);
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);

    /* 发送低八位 */
    USART_SendData(pUSARTx,temp_l);
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);
}




