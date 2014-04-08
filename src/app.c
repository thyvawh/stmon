/**
  ******************************************************************************
  * @file    app.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   This file provides all the Application firmware functions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/ 

#include "stm32f429i_discovery_sdram.h"
// #include "usbd_cdc_core.h"
// #include "usbd_usr.h"
#include "usb_conf.h"
// #include "usbd_desc.h"

#include "usbh_msc_core.h"
#include "usbh_usr.h"

#include "cbuf.h"
#include "stm32f4xx_rcc.h" 

#include "mems.h"

#include "global_includes.h"
#include "Global.h"
#include "GUI_Type.h"
#include "GUI.h"
#include "GUI_JPEG_Private.h"
#include "string.h"

#include <stdarg.h>
  
ErrorStatus HSEStartUpStatus; 

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */


/** @defgroup APP_VCP 
  * @brief Mass storage application module
  * @{
  */ 

/** @defgroup APP_VCP_Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_Defines
  * @{
  */ 

/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_Macros
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_Variables
  * @{
  */ 
  
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
   
__ALIGN_BEGIN USB_OTG_CORE_HANDLE    USB_OTG_dev __ALIGN_END ;
__ALIGN_BEGIN USB_OTG_CORE_HANDLE    USB_OTG_Core __ALIGN_END;

__ALIGN_BEGIN USBH_HOST                USB_Host __ALIGN_END;

extern FATFS fatfs;
extern FIL file;

uint32_t demo_mode = 0;
uint32_t init_done = 0;

/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_FunctionPrototypes
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_Functions
  * @{
  */ 

#define RTC_CLOCK_SOURCE_LSI

typedef struct 
{
      uint8_t tab[32];
} Table_TypeDef;

RTC_InitTypeDef RTC_InitStructure;
RTC_TimeTypeDef RTC_TimeStructure;
RTC_DateTypeDef RTC_DateStructure;
RTC_TimeTypeDef RTC_TimeStampStructure;
RTC_DateTypeDef RTC_TimeStampDateStructure;

uint32_t uwAsynchPrediv = 0;
uint32_t uwSynchPrediv = 0;
uint32_t uwSecondfraction = 0;

__IO uint32_t   uwLsiFreq = 0;
__IO uint32_t   LsiFreq = 0;
__IO uint32_t   uwCaptureNumber = 0; 
__IO uint32_t   uwPeriodValue = 0;


CircularBuffer 	g_LogCB;
uint8_t  g_LogCB_enable = 0;


short st_subsecond;

uint32_t g_dma_enable;
uint32_t g_init_usb_core;

extern uint8_t Image_Browser (char* path);  

uint32_t GetLSIFrequency(void)
{

  NVIC_InitTypeDef   NVIC_InitStructure;
  TIM_ICInitTypeDef  TIM_ICInitStructure;
  RCC_ClocksTypeDef  RCC_ClockFreq;

  /* Enable the LSI oscillator ************************************************/
   RCC_LSICmd(ENABLE);
  
  /* Wait till LSI is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
  {}

  /* TIM5 configuration *******************************************************/ 
  /* Enable TIM5 clock */
   RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
  
  /* Connect internally the TIM5_CH4 Input Capture to the LSI clock output */
   TIM_RemapConfig(TIM5, TIM5_LSI);

  /* Configure TIM5 presclaer */
   TIM_PrescalerConfig(TIM5, 0, TIM_PSCReloadMode_Immediate);
  
  /* TIM5 configuration: Input Capture mode ---------------------
     The LSI oscillator is connected to TIM5 CH4
     The Rising edge is used as active edge,
     The TIM5 CCR4 is used to compute the frequency value 
  ------------------------------------------------------------ */
   TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
   TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
   TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
   TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV8;
   TIM_ICInitStructure.TIM_ICFilter = 0;
   TIM_ICInit(TIM5, &TIM_ICInitStructure);
  
  /* Enable TIM5 Interrupt channel */
   NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);

  /* Enable TIM5 counter */
   TIM_Cmd(TIM5, ENABLE);

  /* Reset the flags */
   TIM5->SR = 0;
    
  /* Enable the CC4 Interrupt Request */  
  TIM_ITConfig(TIM5, TIM_IT_CC4, ENABLE);

  /* Wait until the TIM5 get 2 LSI edges (refer to TIM5_IRQHandler() in 
   *     stm32f4xx_it.c file) ******************************************************/
  while(uwCaptureNumber != 2)
  {
  }
  /* Deinitialize the TIM5 peripheral registers to their default reset values */
  TIM_DeInit(TIM5);


  /* Compute the LSI frequency, depending on TIM5 input clock frequency (PCLK1)*/
  /* Get SYSCLK, HCLK and PCLKx frequency */
  RCC_GetClocksFreq(&RCC_ClockFreq);

  /* Get PCLK1 prescaler */
  if ((RCC->CFGR & RCC_CFGR_PPRE1) == 0)
  { 
      /* PCLK1 prescaler equal to 1 => TIMCLK = PCLK1 */
      return ((RCC_ClockFreq.PCLK1_Frequency / uwPeriodValue) * 8);
  }
  else
  { /* PCLK1 prescaler different from 1 => TIMCLK = 2 * PCLK1 */
      return (((2 * RCC_ClockFreq.PCLK1_Frequency) / uwPeriodValue) * 8) ;
  }
}


int8_t RTC_Configuration(void)
{

  /* Enable the PWR clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

  /* Allow access to RTC */
  PWR_BackupAccessCmd(ENABLE);

/* LSI used as RTC source clock */
/* The RTC Clock may varies due to LSI frequency dispersion. */   
  /* Enable the LSI OSC */ 
  RCC_LSICmd(ENABLE);

  /* Wait till LSI is ready */  
  while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
  {
  }

  /* Select the RTC Clock Source */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
   
  /* Enable the RTC Clock */
  RCC_RTCCLKCmd(ENABLE);
  
    /* Wait for RTC APB registers synchronisation */
  RTC_WaitForSynchro();
  
  /* Calendar Configuration with LSI supposed at 32KHz */
  RTC_InitStructure.RTC_AsynchPrediv = 0x7F;
  RTC_InitStructure.RTC_SynchPrediv	=  0xFF; /* (32KHz / 128) - 1 = 0xFF*/
  RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
  RTC_Init(&RTC_InitStructure);  

  /* Get the LSI frequency:  TIM5 is used to measure the LSI frequency */
  LsiFreq = GetLSIFrequency();
   
  /* Adjust LSI Configuration */
  // RTC_InitStructure.RTC_AsynchPrediv = 0x7F;
  // RTC_InitStructure.RTC_SynchPrediv	=  ((LsiFreq/128) - 1);

  RTC_InitStructure.RTC_AsynchPrediv = 0x1;
  RTC_InitStructure.RTC_SynchPrediv	=  LsiFreq / (RTC_InitStructure.RTC_AsynchPrediv + 1) - 1;

  RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
  RTC_Init(&RTC_InitStructure);
  return 0;
}



static void RTC_Config(void)
{
    /* Enable the PWR clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

    /* Allow access to RTC */
    PWR_BackupAccessCmd(ENABLE);


#if defined (RTC_CLOCK_SOURCE_LSI)  /* LSI used as RTC source clock*/
    /* The RTC Clock may varies due to LSI frequency dispersion. */
    /* Enable the LSI OSC */ 
    RCC_LSICmd(ENABLE);

    /* Wait till LSI is ready */  
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
    {
    }

    /* Select the RTC Clock Source */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);

    /* ck_spre(1Hz) = RTCCLK(LSI) /(uwAsynchPrediv + 1)*(uwSynchPrediv + 1)*/
    uwSynchPrediv = 0x3FFF;  //    uwSynchPrediv = 0xFF;
    uwAsynchPrediv = 0x1; //  uwAsynchPrediv = 0x7F; // 0x7F;

#elif defined (RTC_CLOCK_SOURCE_LSE) /* LSE used as RTC source clock */
    /* Enable the LSE OSC */
    RCC_LSEConfig(RCC_LSE_ON);

    /* Wait till LSE is ready */  
    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
    {
    }

    /* Select the RTC Clock Source */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    /* ck_spre(1Hz) = RTCCLK(LSE) /(uwAsynchPrediv + 1)*(uwSynchPrediv + 1)*/
    uwSynchPrediv = 0x3FFF; // uwSynchPrediv = 0xFF;
    uwAsynchPrediv = 0x1; // uwAsynchPrediv = 0x7E;

#else
#error Please select the RTC Clock source inside the main.c file
#endif /* RTC_CLOCK_SOURCE_LSI */

    /* Configure the RTC data register and RTC prescaler */
    RTC_InitStructure.RTC_AsynchPrediv = uwAsynchPrediv;
    RTC_InitStructure.RTC_SynchPrediv = uwSynchPrediv;
    RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;

    /* Enable the RTC Clock */
    RCC_RTCCLKCmd(ENABLE);

    /* Wait for RTC APB registers synchronisation */
    RTC_WaitForSynchro();

    /* Clear the RTC Alarm Flag */
    RTC_ClearFlag(RTC_FLAG_ALRAF);

    /* Clear the EXTI Line 17 Pending bit (Connected internally to RTC Alarm) */
    EXTI_ClearITPendingBit(EXTI_Line17);

    RTC_Init(&RTC_InitStructure);

}

/**
 ** @brief  Returns the time entered by user, using Hyperterminal.
 ** @param  None
 ** @retval None
 **/
static void RTC_TimeRegulate(void)
{
    /* Set Time hh:mm:ss */
    RTC_TimeStructure.RTC_H12     = RTC_H12_AM;
    RTC_TimeStructure.RTC_Hours   = 0x08;  
    RTC_TimeStructure.RTC_Minutes = 0x10;
    RTC_TimeStructure.RTC_Seconds = 0x00;
    RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure);

    /* Set Date Week/Date/Month/Year */
    RTC_DateStructure.RTC_WeekDay = 0x01;
    RTC_DateStructure.RTC_Date = 0x31;
    RTC_DateStructure.RTC_Month = 0x12;
    RTC_DateStructure.RTC_Year = 0x12;
    RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure);

    /* Write BkUp DR0 */
    RTC_WriteBackupRegister(RTC_BKP_DR0, 0x32F2);
}

/**
 ** @brief  Returns the current time and sub second.
 ** @param  Secondfraction: the sub second fraction.
 ** @param  RTC_TimeStructure : pointer to a RTC_TimeTypeDef structure that 
 **         contains the current time values. 
 ** @retval table : return current time and sub second in a table form
 **/
static Table_TypeDef RTC_Get_Time_StringTbl(uint32_t Secondfraction , RTC_TimeTypeDef* RTC_TimeStructure )
{
    Table_TypeDef table2;

    /* Fill the table2 fields with the current Time*/
    table2.tab[0]  = (((uint8_t)(RTC_TimeStructure->RTC_Hours & 0xF0) >> 0x04) + 0x30);
    table2.tab[1]  = (((uint8_t)(RTC_TimeStructure->RTC_Hours & 0x0F))+ 0x30);
    table2.tab[2]  = 0x3A;

    table2.tab[3]  = (((uint8_t)(RTC_TimeStructure->RTC_Minutes & 0xF0) >> 0x04) + 0x30);
    table2.tab[4]  =(((uint8_t)(RTC_TimeStructure->RTC_Minutes & 0x0F))+ (uint8_t)0x30);
    table2.tab[5]  = 0x3A;

    table2.tab[6]  = (((uint8_t)(RTC_TimeStructure->RTC_Seconds & 0xF0) >> 0x04)+ 0x30);
    table2.tab[7]  = (((uint8_t)(RTC_TimeStructure->RTC_Seconds & 0x0F)) + 0x30);
    table2.tab[8]  = '.';

    table2.tab[9] = (uint8_t)(((Secondfraction % 100000) / 10000) + 0x30);
    table2.tab[10] = (uint8_t)(((Secondfraction % 10000) / 1000) + 0x30);
    table2.tab[11] = (uint8_t)(((Secondfraction % 1000) / 100) + 0x30);
    table2.tab[12] = (uint8_t)(((Secondfraction % 100) / 10) + 0x30);
    table2.tab[13] = (uint8_t)((Secondfraction % 10) + 0x30);

    /* return table2 */
    return table2;
}

/**
 ** @brief  Returns the current time and sub second.
 ** @param  Secondfraction: the sub second fraction.
 ** @param  RTC_TimeStructure : pointer to a RTC_TimeTypeDef structure that 
 **         contains the current time values. 
 ** @retval table : return current time and sub second in a table form
 **/
static Table_TypeDef RTC_Get_Date_StringTbl(RTC_DateTypeDef* RTC_DateStructure )
{
    Table_TypeDef table2;

    /* Fill the table2 fields with the current Time*/
    table2.tab[0]  = (((uint8_t)(RTC_DateStructure->RTC_Date & 0xF0) >> 0x04) + 0x30);
    table2.tab[1]  = (((uint8_t)(RTC_DateStructure->RTC_Date & 0x0F))+ 0x30);
    table2.tab[2]  = 0x2F;

    table2.tab[3]  = (((uint8_t)(RTC_DateStructure->RTC_Month & 0xF0) >> 0x04) + 0x30);
    table2.tab[4]  =(((uint8_t)(RTC_DateStructure->RTC_Month & 0x0F))+ (uint8_t)0x30);
    table2.tab[5]  = 0x2F;

    table2.tab[6]  = (((uint8_t)(RTC_DateStructure->RTC_Year & 0xF0) >> 0x04)+ 0x30);
    table2.tab[7]  = (((uint8_t)(RTC_DateStructure->RTC_Year & 0x0F)) + 0x30);
    table2.tab[8]  = 0xA0;

    /* return table2 */
    return table2;
}


static void RTC_display(Table_TypeDef table, uint8_t max_idx )
{   
      uint8_t index = 0;
      
      for (index = 0;index < max_idx; index++)
        {
         char c = table.tab[index];
         if (c == '\0')
             break;
         // VCP_DataTx (&c, 1);
        }  
}

static void RTC_Time_display(Table_TypeDef table) {
    RTC_display(table, 14);
}

static void RTC_Date_display(Table_TypeDef table) {
    RTC_display(table, 8);
}


char* itoa(int val, int base){
        
      static char buf[32] = {0};
      int i = 30;
      for(; val && i ; --i, val /= base)
          buf[i] = "0123456789abcdef"[val % base];
      return &buf[i+1];
}


char* itoa_ms(int val, int base){
        
      static char buf[5] = {'0', '0', '0', '0', 0};
      int i = 3;
      for(; val && i ; --i, val /= base)
          buf[i] = "0123456789abcdef"[val % base];
      return &buf[1];
}

static void print_str(char* buf) {
    
    int i;
    for (i=0 ; buf[i] != NULL ; i++) {
        // VCP_DataTx(&buf[i], 1);
    }
}

static void quick_print(char* buf, int len) {
    
    int i;
    int grp = 32;
    int blks = len / grp;
    int rest = len % grp;

    for (i = 0 ; i < blks * grp ; i++) {
        // VCP_DataTx(&buf[i], grp);
    }

    for (i = len - rest ; i < len ; i++) {
        // VCP_DataTx(&buf[i], 1);
    }
}



void msg(char* buf) {

    uint32_t ss, subs;
    RTC_WaitForSynchro();

    RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure);
    RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
    ss = RTC_GetSubSecond();
    // subs = 100000 - ((uint32_t)((uint32_t)RTC_GetSubSecond() * 100000) / (uint32_t) RTC_InitStructure.RTC_SynchPrediv);
    subs = st_subsecond * 10;
    RTC_Date_display(RTC_Get_Date_StringTbl(&RTC_DateStructure));
    print_str("T");
    RTC_Time_display(RTC_Get_Time_StringTbl(subs , &RTC_TimeStructure));
    print_str(" ");
    print_str(buf);
    print_str("\r\n");
}

static void log_append_datetime(Table_TypeDef table, uint8_t max_idx )
{   
      uint8_t index = 0;
      
      for (index = 0;index < max_idx; index++) {
          char c = table.tab[index];
          if (c == '\0')
              break;
          cbWrite(&g_LogCB, (ElemType*) &c);
      }  

}


void
do_log_append(char *fmt, uint8_t show_timestamp, va_list va) {

    int i = 0;
    char sep = 'T', space = ' ';
    uint32_t ss, subs;

    if (! g_LogCB_enable) 
	    return;

    if (show_timestamp == 1) { 

        RTC_WaitForSynchro();

        RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure);
        RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
        ss = RTC_GetSubSecond();
        subs = 100000 - ((uint32_t)((uint32_t)ss * 100000) / (uint32_t) RTC_InitStructure.RTC_SynchPrediv);

        Table_TypeDef date_tbl = RTC_Get_Date_StringTbl(&RTC_DateStructure);
        Table_TypeDef time_tbl = RTC_Get_Time_StringTbl(subs, &RTC_TimeStructure); 

        // Put timestamp first

        log_append_datetime(date_tbl, 8);
        cbWrite(&g_LogCB, &sep);
        log_append_datetime(time_tbl, 14);
        cbWrite(&g_LogCB, &space);
    }

    mini_vsnprintf(fmt, va);


}


void
log_append(char *fmt, ...)
{
    va_list va;

    __disable_irq();

    va_start(va, fmt);
    do_log_append(fmt, 1, va);
    va_end(va);

    __enable_irq();
}

void dirty_log_append(char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    do_log_append(fmt, 1, va);
    va_end(va);
}


void
log_raw_append(char* fmt, ...) {

    va_list va;

    __disable_irq();

    va_start(va, fmt);
    do_log_append(fmt, 0, va);
    va_end(va);

    __enable_irq();

}

void

dirty_log_raw_append(char* fmt, ...) {
    
    va_list va;
    va_start(va, fmt);
    do_log_append(fmt, 0, va);
    va_end(va);
}

void clockupdate_log(RCC_BUS_TYPE bus, uint32_t periph, FunctionalState newstate) {

    char* bus_name = NULL;

    __disable_irq();

    dirty_log_append("docea_monitoring %s ", newstate == ENABLE ? "enabling" : "disabling");

    switch (bus) {
        case RCC_AHB1_BUS:
            dirty_log_raw_append("AHB1 ");
            if (periph & RCC_AHB1Periph_GPIOA) dirty_log_raw_append("GPIOA ");
            if (periph & RCC_AHB1Periph_GPIOB) dirty_log_raw_append("GPIOB ");
            if (periph & RCC_AHB1Periph_GPIOC) dirty_log_raw_append("GPIOC ");
            if (periph & RCC_AHB1Periph_GPIOD) dirty_log_raw_append("GPIOD ");
            if (periph & RCC_AHB1Periph_GPIOE) dirty_log_raw_append("GPIOE ");
            if (periph & RCC_AHB1Periph_GPIOF) dirty_log_raw_append("GPIOF ");
            if (periph & RCC_AHB1Periph_GPIOG) dirty_log_raw_append("GPIOG ");
            if (periph & RCC_AHB1Periph_GPIOH) dirty_log_raw_append("GPIOH ");
            if (periph & RCC_AHB1Periph_GPIOI) dirty_log_raw_append("GPIOI ");
            if (periph & RCC_AHB1Periph_GPIOJ) dirty_log_raw_append("GPIOJ ");
            if (periph & RCC_AHB1Periph_GPIOK) dirty_log_raw_append("GPIOK ");
            if (periph & RCC_AHB1Periph_CRC) dirty_log_raw_append("CRC ");
            if (periph & RCC_AHB1Periph_BKPSRAM) dirty_log_raw_append("BKPSRAM interface ");
            if (periph & RCC_AHB1Periph_CCMDATARAMEN) dirty_log_raw_append("CCM data RAM interface ");
            if (periph & RCC_AHB1Periph_DMA1) dirty_log_raw_append("DMA1 ");
            if (periph & RCC_AHB1Periph_DMA2) dirty_log_raw_append("DMA2 ");
            if (periph & RCC_AHB1Periph_DMA2D) dirty_log_raw_append("DMA2D ");
            if (periph & RCC_AHB1Periph_ETH_MAC) dirty_log_raw_append("Ethernet MAC ");
            if (periph & RCC_AHB1Periph_ETH_MAC_Tx) dirty_log_raw_append("Ethernet Transmission ");
            if (periph & RCC_AHB1Periph_ETH_MAC_Rx ) dirty_log_raw_append("Ethernet Reception ");
            if (periph & RCC_AHB1Periph_ETH_MAC_PTP) dirty_log_raw_append("Ethernet PTP ");
            if (periph & RCC_AHB1Periph_OTG_HS) dirty_log_raw_append("USB OTG HS ");
            if (periph & RCC_AHB1Periph_OTG_HS_ULPI) dirty_log_raw_append("USB OTG HS ULPI ");
           break; 

        case RCC_AHB2_BUS:
           dirty_log_raw_append("AHB2 ");
           if (periph & RCC_AHB2Periph_DCMI) dirty_log_raw_append("DCMI ");
           if (periph & RCC_AHB2Periph_CRYP) dirty_log_raw_append("CRYP ");
           if (periph & RCC_AHB2Periph_HASH) dirty_log_raw_append("HASH ");
           if (periph & RCC_AHB2Periph_RNG) dirty_log_raw_append("RNG ");
           if (periph & RCC_AHB2Periph_OTG_FS) dirty_log_raw_append("USB OTG FS ");
           break;

        case RCC_AHB3_BUS:
           dirty_log_raw_append("AHB3 ");
// if (periph & RCC_AHB3Periph_FSMC) dirty_log_raw_append("FSMC ");
           if (periph & RCC_AHB3Periph_FMC) dirty_log_raw_append("FMC ");
           break;

        case RCC_APB1_BUS:
           dirty_log_raw_append("APB1 ");
           if (periph & RCC_APB1Periph_TIM2) dirty_log_raw_append("TIM2 ");
           if (periph & RCC_APB1Periph_TIM3) dirty_log_raw_append("TIM3 ");
           if (periph & RCC_APB1Periph_TIM4) dirty_log_raw_append("TIM4 ");
           if (periph & RCC_APB1Periph_TIM5) dirty_log_raw_append("TIM5 ");
           if (periph & RCC_APB1Periph_TIM6) dirty_log_raw_append("TIM6 ");
           if (periph & RCC_APB1Periph_TIM7) dirty_log_raw_append("TIM7 ");
           if (periph & RCC_APB1Periph_TIM12) dirty_log_raw_append("TIM12 ");
           if (periph & RCC_APB1Periph_TIM13) dirty_log_raw_append("TIM13 ");
           if (periph & RCC_APB1Periph_TIM14) dirty_log_raw_append("TIM14 ");
           if (periph & RCC_APB1Periph_WWDG) dirty_log_raw_append("WWDG ");
           if (periph & RCC_APB1Periph_SPI2) dirty_log_raw_append("SPI2 ");
           if (periph & RCC_APB1Periph_SPI3) dirty_log_raw_append("SPI3 ");
           if (periph & RCC_APB1Periph_USART2) dirty_log_raw_append("USART2 ");
           if (periph & RCC_APB1Periph_USART3) dirty_log_raw_append("USART3 ");
           if (periph & RCC_APB1Periph_UART4) dirty_log_raw_append("UART4 ");
           if (periph & RCC_APB1Periph_UART5) dirty_log_raw_append("UART5 ");
           if (periph & RCC_APB1Periph_I2C1) dirty_log_raw_append("I2C1 ");
           if (periph & RCC_APB1Periph_I2C2) dirty_log_raw_append("I2C2 ");
           if (periph & RCC_APB1Periph_I2C3) dirty_log_raw_append("I2C3 ");
           if (periph & RCC_APB1Periph_CAN1) dirty_log_raw_append("CAN1 ");
           if (periph & RCC_APB1Periph_CAN2) dirty_log_raw_append("CAN2 ");
           if (periph & RCC_APB1Periph_PWR) dirty_log_raw_append("PWR ");
           if (periph & RCC_APB1Periph_DAC) dirty_log_raw_append("DAC ");
           if (periph & RCC_APB1Periph_UART7) dirty_log_raw_append("UART7 ");
           if (periph & RCC_APB1Periph_UART8) dirty_log_raw_append("UART8 ");
           break;

        case RCC_APB2_BUS:
           dirty_log_raw_append("APB2 ");
           if (periph & RCC_APB2Periph_TIM1) dirty_log_raw_append("TIM1 ");
           if (periph & RCC_APB2Periph_TIM8) dirty_log_raw_append("TIM8 ");
           if (periph & RCC_APB2Periph_USART1) dirty_log_raw_append("USART1 ");
           if (periph & RCC_APB2Periph_USART6) dirty_log_raw_append("USART6 ");
           if (periph & RCC_APB2Periph_ADC1) dirty_log_raw_append("ADC1 ");
           if (periph & RCC_APB2Periph_ADC2) dirty_log_raw_append("ADC2 ");
           if (periph & RCC_APB2Periph_ADC3) dirty_log_raw_append("ADC3 ");
           if (periph & RCC_APB2Periph_SDIO) dirty_log_raw_append("SDIO ");
           if (periph & RCC_APB2Periph_SPI1) dirty_log_raw_append("SPI1 ");
           if (periph & RCC_APB2Periph_SPI4) dirty_log_raw_append("SPI4 ");
           if (periph & RCC_APB2Periph_SYSCFG) dirty_log_raw_append("SYSCFG ");
           if (periph & RCC_APB2Periph_TIM9) dirty_log_raw_append("TIM9 ");
           if (periph & RCC_APB2Periph_TIM10) dirty_log_raw_append("TIM10 ");
           if (periph & RCC_APB2Periph_TIM11) dirty_log_raw_append("TIM11 ");
           if (periph & RCC_APB2Periph_SPI5) dirty_log_raw_append("SPI5 ");
           if (periph & RCC_APB2Periph_SPI6) dirty_log_raw_append("SPI6 ");
           if (periph & RCC_APB2Periph_SAI1) dirty_log_raw_append("SAI1 ");
           if (periph & RCC_APB2Periph_LTDC) dirty_log_raw_append("LTDC ");
           break;
    }
    dirty_log_raw_append("clock\n");
    
    __enable_irq();
} 


void print_log() {

    char nl = '\n';
    char c = '.';

    while (!cbIsEmpty(&g_LogCB)) {
        cbRead(&g_LogCB, &c); 
        // VCP_DataTx(&c, 1);   
    }
    // VCP_DataTx(&nl, 1);   
}

void EnableFlashInterrupt() {

    NVIC_InitTypeDef NVIC_GenericInitStructure;

    NVIC_GenericInitStructure.NVIC_IRQChannel = FLASH_IRQn;
    NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_GenericInitStructure); 
}

NVIC_InitTypeDef NVIC_SPIInitStructure;

void EnableAllSPIInterrupt() {

    NVIC_SPIInitStructure.NVIC_IRQChannel = SPI5_IRQn;
    NVIC_SPIInitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_SPIInitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_SPIInitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_SPIInitStructure); 
}



void EnableAllDMA() {

  NVIC_InitTypeDef NVIC_GenericInitStructure;
 
  NVIC_GenericInitStructure.NVIC_IRQChannel = DMA1_Stream0_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 

  NVIC_GenericInitStructure.NVIC_IRQChannel = DMA1_Stream1_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 

  NVIC_GenericInitStructure.NVIC_IRQChannel = DMA1_Stream2_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 

  NVIC_GenericInitStructure.NVIC_IRQChannel = DMA1_Stream3_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 

  NVIC_GenericInitStructure.NVIC_IRQChannel = DMA1_Stream4_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 

  NVIC_GenericInitStructure.NVIC_IRQChannel = DMA1_Stream5_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 


  NVIC_GenericInitStructure.NVIC_IRQChannel = DMA1_Stream6_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 

  NVIC_GenericInitStructure.NVIC_IRQChannel = DMA1_Stream7_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 


  NVIC_GenericInitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 

  NVIC_GenericInitStructure.NVIC_IRQChannel = DMA2_Stream1_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 

  NVIC_GenericInitStructure.NVIC_IRQChannel = DMA2_Stream2_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 

  NVIC_GenericInitStructure.NVIC_IRQChannel = DMA2_Stream3_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 

  NVIC_GenericInitStructure.NVIC_IRQChannel = DMA2_Stream4_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 

  NVIC_GenericInitStructure.NVIC_IRQChannel =DMA2_Stream5_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 

  NVIC_GenericInitStructure.NVIC_IRQChannel =DMA2_Stream6_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 

  NVIC_GenericInitStructure.NVIC_IRQChannel =DMA2_Stream7_IRQn;
  NVIC_GenericInitStructure.NVIC_IRQChannelPreemptionPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelSubPriority = 10;
  NVIC_GenericInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_GenericInitStructure); 
}

void EnableDMA2DInterrupt(void) {
  
    NVIC_InitTypeDef NVIC_DMA2DInitStructure;

    NVIC_DMA2DInitStructure.NVIC_IRQChannel = DMA2D_IRQn;
    NVIC_DMA2DInitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_DMA2DInitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_DMA2DInitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_DMA2DInitStructure); 
}

extern void SetSysClockLQ();
extern void SystemInitLQ();


void SetSysClockTo168(void)
{
  /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration -----------------------------*/   
  /* RCC system reset(for debug purpose) */
  RCC_DeInit();
  
  /* Enable HSE */
  RCC_HSEConfig(RCC_HSE_ON);
  
  /* Wait till HSE is ready */
  HSEStartUpStatus = RCC_WaitForHSEStartUp();
  
  if (HSEStartUpStatus == SUCCESS)
  {
    /* Enable Prefetch Buffer */
    FLASH_PrefetchBufferCmd(ENABLE);
    
    /* Flash 2 wait state */
    FLASH_SetLatency(FLASH_Latency_5);
    
    /* PLL configuration */
    RCC_PLLConfig(RCC_PLLSource_HSE, 8, 336, 2, 7);
    /* Enable PLL */ 
    RCC_PLLCmd(ENABLE);
    
    /* Wait till PLL is ready */
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
    {
    }
    
    /* Select PLL as system clock source */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    
    /* Wait till PLL is used as system clock source */
    while(RCC_GetSYSCLKSource() != 0x08)
    {
    }
  }
  else
  { /* If HSE fails to start-up, the application will have wrong clock configuration.
    User can add here some code to deal with this error */    
    
    /* Go to infinite loop */
    while (1)
    {
    }
  }
}


xTaskHandle  Loop_Handle;

int Loop_Task(void * pvParameters) {
	// while (! init_done)
        //		;
	// log_append("loop start\n");
	while (1) {
		// log_append("Background loop\n");
		Routine_MEMS();
		vTaskDelay(100);
	}
}



xTaskHandle  Task_Handle;
int Main_Task(void * pvParameters) {
	
#define Loop_Task_PRIO          ( tskIDLE_PRIORITY  + 10 )
#define Loop_Task_STACK         ( 3048 )

	LowLevel_Init();


	USBH_Init(&USB_OTG_Core, 
			USB_OTG_HS_CORE_ID,
			&USB_Host,
			&USBH_MSC_cb, 
			&USR_cb); 

	/*
	   EnableAllDMA();

	   EnableFlashInterrupt();
	   FLASH_ITConfig(FLASH_IT_EOP, ENABLE);

	   EnableAllSPIInterrupt();
	   SPI_I2S_ITConfig(SPI5, SPI_I2S_IT_TXE, ENABLE); 
	   */

	InitMEMS(); 

	STM_EVAL_LEDInit(LED3); 
	STM_EVAL_LEDInit(LED4);


	xTaskCreate(Loop_Task,
              (signed char const*)"LOOP_DEMO",
              Loop_Task_STACK,
              NULL,
              Loop_Task_PRIO,
              &Loop_Handle);

	init_done = 1;

	while (1)
	{
		/* Host Task handler */
		USBH_Process(&USB_OTG_Core, &USB_Host);
		USB_OTG_BSP_mDelay(25);
		// Routine_MEMS();
		// log_append("docea_monitoring cpu_state sleep\n");
		__WFI(); 
	}
}



/**
  * @brief  Program entry point
  * @param  None
  * @retval None
  */
int main(void)
{
  __IO uint32_t i = 0;  


    /*!< At this stage the microcontroller clock setting is already configured, 
  this is done through SystemInit() function which is called from startup
  file (startup_stm32fxxx_xx.s) before to branch to application main.
  To reconfigure the default setting of SystemInit() function, refer to
  system_stm32fxxx.c file
  */  

//  if (RTC_ReadBackupRegister(RTC_BKP_DR0) != 0x32F2)
//  {
    RTC_Config();
    RTC_TimeRegulate();
//  }
    
    cbInit(&g_LogCB, 0xD0000000 + 0x400000, 0x400000);
	g_LogCB_enable = 1;
	
	log_append("Start application\n");
	log_append("System clock : %u Hz\n", SystemCoreClock);


    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    /* Setup SysTick Timer for 1 msec interrupts.*/
    if (SysTick_Config(SystemCoreClock / 1000))
    { 
	    /* Capture error */ 
	    while (1);
    }

    /* Enable the BUTTON Clock */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    /* Configure Button pin as Output */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Force capacity to be charged quickly */
    GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);
    demo_mode = 1;
    Delay (25);
    demo_mode = 0;

#define Main_Task_PRIO    ( tskIDLE_PRIORITY  + 9 )
#define Main_Task_STACK   ( 512 )


    xTaskCreate(Main_Task,
		    (signed char const*)"MAIN_GND",
		    Main_Task_STACK,
		    NULL,
		    Main_Task_PRIO,
		    &Task_Handle);


     vTaskStartScheduler();


} 

#ifdef USE_FULL_ASSERT
/**
* @brief  assert_failed
*         Reports the name of the source file and the source line number
*         where the assert_param error has occurred.
* @param  File: pointer to the source file name
* @param  Line: assert_param error line source number
* @retval None
*/
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  
  /* Infinite loop */
    while (1)
    {}
}
#endif


void vApplicationMallocFailedHook( void )
{
  while (1)
  {}
}


/**
  * @}
  */ 


/**
  * @}
  */ 


/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
