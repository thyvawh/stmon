/**
  ******************************************************************************
  * @file    LTDC_AnimatedPictureFromUSB/usbh_usr.c
  * @author  MCD Application Team
  * @version V1.0.1
  * @date    11-November-2013
  * @brief   This file includes the usb host library user callbacks
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
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
#include <string.h>
#include "usbh_usr.h"
#include "lcd_log.h"
#include "ff.h"       /* FATFS */
#include "usbh_msc_core.h"
#include "usbh_msc_scsi.h"
#include "usbh_msc_bot.h"


#include "cbuf.h"
#include "docea_log.h"

#include "global_includes.h"
#include "Global.h"
#include "GUI_Type.h"
#include "GUI.h"
#include "GUI_JPEG_Private.h"

/** @addtogroup USBH_USER
* @{
*/

/** @addtogroup USBH_MSC_DEMO_USER_CALLBACKS
* @{
*/

/** @defgroup USBH_USR 
* @brief    This file includes the usb host stack user callbacks
* @{
*/ 

/** @defgroup USBH_USR_Private_TypesDefinitions
* @{
*/ 
/**
* @}
*/ 

#define USB_OTG_BSP_mDelay(x)	vTaskDelay(x)

/** @defgroup USBH_USR_Private_Defines
* @{
*/ 
#define IMAGE_BUFFER_SIZE    512
/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Macros
* @{
*/ 
extern USB_OTG_CORE_HANDLE          USB_OTG_Core;
extern USBH_HOST USB_Host;
extern uint32_t g_dma_enable;
extern uint32_t g_init_usb_core;
extern uint8_t g_LogCB_enable;
/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Variables
* @{
*/ 
uint8_t USBH_USR_ApplicationState = USH_USR_FS_INIT;
uint8_t filenameString[15]  = {0};

FATFS fatfs;
FIL file;
FIL Video_File;
uint8_t Image_Buf[IMAGE_BUFFER_SIZE];
uint8_t line_idx = 0;   
uint32_t BytesRead = 0;

/*  Points to the DEVICE_PROP structure of current device */
/*  The purpose of this register is to speed up the execution */


USBH_Usr_cb_TypeDef USR_cb =
{
  USBH_USR_Init,
  USBH_USR_DeInit,
  USBH_USR_DeviceAttached,
  USBH_USR_ResetDevice,
  USBH_USR_DeviceDisconnected,
  USBH_USR_OverCurrentDetected,
  USBH_USR_DeviceSpeedDetected,
  USBH_USR_Device_DescAvailable,
  USBH_USR_DeviceAddressAssigned,
  USBH_USR_Configuration_DescAvailable,
  USBH_USR_Manufacturer_String,
  USBH_USR_Product_String,
  USBH_USR_SerialNum_String,
  USBH_USR_EnumerationDone,
  USBH_USR_UserInput,
  USBH_USR_MSC_Application,
  USBH_USR_DeviceNotSupported,
  USBH_USR_UnrecoveredError
    
};

/**
* @}
*/

/** @defgroup USBH_USR_Private_Constants
* @{
*/ 
/*--------------- LCD Messages ---------------*/
const uint8_t MSG_HOST_INIT[]        = "> Host Library Initialized\n";
const uint8_t MSG_DEV_ATTACHED[]     = "> Device Attached \n";
const uint8_t MSG_DEV_DISCONNECTED[] = "> Device Disconnected\n";
const uint8_t MSG_DEV_ENUMERATED[]   = "> Enumeration completed \n";
const uint8_t MSG_DEV_HIGHSPEED[]    = "> High speed device detected\n";
const uint8_t MSG_DEV_FULLSPEED[]    = "> Full speed device detected\n";
const uint8_t MSG_DEV_LOWSPEED[]     = "> Low speed device detected\n";
const uint8_t MSG_DEV_ERROR[]        = "> Device fault \n";

const uint8_t MSG_MSC_CLASS[]        = "> Mass storage device connected\n";
const uint8_t MSG_HID_CLASS[]        = "> HID device connected\n";
const uint8_t MSG_DISK_SIZE[]        = "> Size of the disk in MBytes: \n";
const uint8_t MSG_LUN[]              = "> LUN Available in the device:\n";
const uint8_t MSG_ROOT_CONT[]        = "> Exploring disk flash ...\n";
const uint8_t MSG_WR_PROTECT[]       = "> The disk is write protected\n";
const uint8_t MSG_UNREC_ERROR[]      = "> UNRECOVERED ERROR STATE\n";

/**
* @}
*/


/** @defgroup USBH_USR_Private_FunctionPrototypes
* @{
*/
uint8_t Explore_Disk (char* path , uint8_t recu_level);
uint8_t Image_Browser (char* path);
void     Show_Image(void);
void     Toggle_Leds(void);
/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Functions
* @{
*/ 


/**
* @brief  USBH_USR_Init 
*         Displays the message on LCD for host lib initialization
* @param  None
* @retval None
*/
void USBH_USR_Init(void)
{
  
  static uint8_t startup = 0;  
  
  if(startup == 0 )
  {
    startup = 1;
    /* Configure the LEDs */
    STM_EVAL_LEDInit(LED3); 
    STM_EVAL_LEDInit(LED4); 
    
    STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);

    LCD_ClassicInit();
       
    /* Initialize the LCD */
    // LCD_Init();
    // LCD_LayerInit();
    
    /* Set LCD background layer */
    // LCD_SetLayer(LCD_BACKGROUND_LAYER);
    
    /* Set LCD transparency */
    // LCD_SetTransparency(0);
    
    /* Set LCD foreground layer */
    // LCD_SetLayer(LCD_FOREGROUND_LAYER);
    
    /* LTDC reload configuration */  
    // LTDC_ReloadConfig(LTDC_IMReload);
    
    /* Enable the LTDC */
    // LTDC_Cmd(ENABLE);
    
    /* LCD Log initialization */
    LCD_LOG_Init(); 
    
    
#ifdef USE_USB_OTG_HS 
    // LCD_LOG_SetHeader("USB HS Host");
#else
    // LCD_LOG_SetHeader(" USB OTG FS MSC Host");
#endif
    LCD_LOG_SetHeader("Docea Demo");
    LCD_DisplayStringLine(LCD_LINE_14, "Searching for USB device... ");

    // LCD_UsrLog("> USB Host library started.\n"); 
    // LCD_LOG_SetFooter ("     USB Host Library v2.1.0" );
  }
}

/**
* @brief  USBH_USR_DeviceAttached 
*         Displays the message on LCD on device attached
* @param  None
* @retval None
*/
void USBH_USR_DeviceAttached(void)
{
  // LCD_UsrLog((void *)MSG_DEV_ATTACHED);
}


/**
* @brief  USBH_USR_UnrecoveredError
* @param  None
* @retval None
*/
void USBH_USR_UnrecoveredError (void)
{
  
  /* Set default screen color*/ 
  // LCD_ErrLog((void *)MSG_UNREC_ERROR); 
}


/**
* @brief  USBH_DisconnectEvent
*         Device disconnect event
* @param  None
* @retval Staus
*/
void USBH_USR_DeviceDisconnected (void)
{
  
  // LCD_LOG_ClearTextZone();
  
  LCD_DisplayStringLine( LCD_LINE_16, "USB disconnected!");
  
  /* Set default screen color*/
  // LCD_ErrLog((void *)MSG_DEV_DISCONNECTED);
}
/**
* @brief  USBH_USR_ResetUSBDevice 
* @param  None
* @retval None
*/
void USBH_USR_ResetDevice(void)
{
  /* callback for USB-Reset */
}


/**
* @brief  USBH_USR_DeviceSpeedDetected 
*         Displays the message on LCD for device speed
* @param  Device speed
* @retval None
*/
void USBH_USR_DeviceSpeedDetected(uint8_t DeviceSpeed)
{
  if(DeviceSpeed == HPRT0_PRTSPD_HIGH_SPEED)
  {
    // LCD_UsrLog((void *)MSG_DEV_HIGHSPEED);
  }  
  else if(DeviceSpeed == HPRT0_PRTSPD_FULL_SPEED)
  {
    // LCD_UsrLog((void *)MSG_DEV_FULLSPEED);
  }
  else if(DeviceSpeed == HPRT0_PRTSPD_LOW_SPEED)
  {
    // LCD_UsrLog((void *)MSG_DEV_LOWSPEED);
  }
  else
  {
    // LCD_UsrLog((void *)MSG_DEV_ERROR);
  }
}

/**
* @brief  USBH_USR_Device_DescAvailable 
*         Displays the message on LCD for device descriptor
* @param  device descriptor
* @retval None
*/
void USBH_USR_Device_DescAvailable(void *DeviceDesc)
{ 
  USBH_DevDesc_TypeDef *hs;
  hs = DeviceDesc;  
  
  
  // LCD_UsrLog("VID : %04Xh\n" , (uint32_t)(*hs).idVendor); 
  // LCD_UsrLog("PID : %04Xh\n" , (uint32_t)(*hs).idProduct); 
}

/**
* @brief  USBH_USR_DeviceAddressAssigned 
*         USB device is successfully assigned the Address 
* @param  None
* @retval None
*/
void USBH_USR_DeviceAddressAssigned(void)
{
  
}


/**
* @brief  USBH_USR_Conf_Desc 
*         Displays the message on LCD for configuration descriptor
* @param  Configuration descriptor
* @retval None
*/
void USBH_USR_Configuration_DescAvailable(USBH_CfgDesc_TypeDef * cfgDesc,
                                          USBH_InterfaceDesc_TypeDef *itfDesc,
                                          USBH_EpDesc_TypeDef *epDesc)
{
  USBH_InterfaceDesc_TypeDef *id;
  
  id = itfDesc;  
  
  if((*id).bInterfaceClass  == 0x08)
  {
    // LCD_UsrLog((void *)MSG_MSC_CLASS);
  }
  else if((*id).bInterfaceClass  == 0x03)
  {
    // LCD_UsrLog((void *)MSG_HID_CLASS);
  }    
}

/**
* @brief  USBH_USR_Manufacturer_String 
*         Displays the message on LCD for Manufacturer String 
* @param  Manufacturer String 
* @retval None
*/
void USBH_USR_Manufacturer_String(void *ManufacturerString)
{
  // LCD_UsrLog("Manufacturer : %s\n", (char *)ManufacturerString);
}

/**
* @brief  USBH_USR_Product_String 
*         Displays the message on LCD for Product String
* @param  Product String
* @retval None
*/
void USBH_USR_Product_String(void *ProductString)
{
  // LCD_UsrLog("Product : %s\n", (char *)ProductString);  
}

/**
* @brief  USBH_USR_SerialNum_String 
*         Displays the message on LCD for SerialNum_String 
* @param  SerialNum_String 
* @retval None
*/
void USBH_USR_SerialNum_String(void *SerialNumString)
{
  // LCD_UsrLog( "Serial Number : %s\n", (char *)SerialNumString);    
} 



/**
* @brief  EnumerationDone 
*         User response request is displayed to ask application jump to class
* @param  None
* @retval None
*/
void USBH_USR_EnumerationDone(void)
{
  
  /* Enumeration complete */
  // LCD_UsrLog((void *)MSG_DEV_ENUMERATED);
  
    LCD_SetTextColor(Green);
    LCD_DisplayStringLine( LCD_LINE_17, "USB detected, please press USR\n" );
    LCD_DisplayStringLine( LCD_LINE_18, "  button to continue\n");
    LCD_SetTextColor(LCD_LOG_DEFAULT_COLOR); 
//    USBH_USR_ApplicationState = USH_USR_FS_DRAW;
} 


/**
* @brief  USBH_USR_DeviceNotSupported
*         Device is not supported
* @param  None
* @retval None
*/
void USBH_USR_DeviceNotSupported(void)
{
  // LCD_ErrLog ("> Device not supported."); 
}  


/**
* @brief  USBH_USR_UserInput
*         User Action for application state entry
* @param  None
* @retval USBH_USR_Status : User response for key button
*/
USBH_USR_Status USBH_USR_UserInput(void)
{
  USBH_USR_Status usbh_usr_status;
  
  usbh_usr_status = USBH_USR_NO_RESP;  
  
  /*Key B3 is in polling mode to detect user action */
  // if(STM_EVAL_PBGetState(BUTTON_USER) != RESET) 
  // { 
    
    // USB_OTG_BSP_mDelay(500);
    usbh_usr_status = USBH_USR_RESP_OK;
    
  // }  
  return usbh_usr_status;
}  

/**
* @brief  USBH_USR_OverCurrentDetected
*         Over Current Detected on VBUS
* @param  None
* @retval Staus
*/
void USBH_USR_OverCurrentDetected (void)
{
  // LCD_ErrLog ("Overcurrent detected.");
}


/**
* @brief  USBH_USR_MSC_Application 
*         Demo application for mass storage
* @param  None
* @retval Staus
*/
int USBH_USR_MSC_Application(void)
{
  FRESULT res;
  uint16_t bytesWritten, bytesToWrite;
  
  /* Set LCD Layer size and pixel format */
  // LTDC_LayerPixelFormat(LTDC_Layer2, LTDC_Pixelformat_RGB565); 
  // LTDC_LayerSize(LTDC_Layer2, 240, 320);
  /* LTDC reload configuration */  
  // LTDC_ReloadConfig(LTDC_IMReload);
  
  switch(USBH_USR_ApplicationState)
  {
  case USH_USR_FS_INIT: 
    
   /* Initialises the File System*/
    // if ( f_mount( 0, &fatfs ) != FR_OK ) 
    // {
      /* efs initialisation fails*/
      // LCD_ErrLog("> Cannot initialize File System.\n");
    //   return(-1);
    // }
    // LCD_UsrLog("> File System initialized.\n");
    // LCD_UsrLog("> Disk capacity : %d Bytes\n", USBH_MSC_Param.MSCapacity * \
      USBH_MSC_Param.MSPageLength); 
    
    // if(USBH_MSC_Param.MSWriteProtect == DISK_WRITE_PROTECTED)
    // {
      // LCD_ErrLog((void *)MSG_WR_PROTECT);
    // }
    
   
      g_LogCB_enable = 0;
      while(/* g_init_usb_core < 2 && */ (HCD_IsDeviceConnected(&USB_OTG_Core)) && \ 
		      (STM_EVAL_PBGetState (BUTTON_USER) != SET) )          
      {
	      // USB_OTG_BSP_mDelay(100);
	      Toggle_Leds();
	      // Routine_MEMS();
      } 
      g_LogCB_enable = 1;

    LCD_X_DisplayDriver(1, LCD_X_INITCONTROLLER, NULL);
    GUI_Init();
    GUI_SelectLayer(1);  


    USBH_USR_ApplicationState = USH_USR_FS_DRAW;
    break;
  
  case USH_USR_FS_READVIDEO:

    GUI_Clear();
    GUI_DispStringAt("> Video Player\n", 15, 20);

    f_mount(0, &fatfs);
    int res = f_open(&Video_File, "0:plane.mov", FA_OPEN_EXISTING | FA_READ);
    if (res == FR_OK) {
	    while(_PlayMJPEG(&Video_File) >= 0)
		    ;
    }
    f_mount(0, NULL); 
   /* LCD_X_SETVIS_INFO lx_info;
    lx_info.OnOff = DISABLE;
    LCD_X_DisplayDriver(1, LCD_X_SETVIS, &lx_info); */

    USBH_USR_ApplicationState = USH_USR_FS_WRITEFILE;
    break;
    
  case USH_USR_FS_READLIST:
    
    // LCD_UsrLog((void *)MSG_ROOT_CONT);
    Explore_Disk("0:/", 1);
    line_idx = 0;   
    USBH_USR_ApplicationState = USH_USR_FS_WRITEFILE;
    
    break;
    
  case USH_USR_FS_WRITEFILE:
    
//    LCD_DeInit();
//    LCD_ClassicInit();
    // LCD_LOG_Init();

    
    log_append("Disable logging. The end.\n");
    g_LogCB_enable = 0;

    /*Key B3 in polling*/
/*    while((HCD_IsDeviceConnected(&USB_OTG_Core)) && \
       (STM_EVAL_PBGetState (BUTTON_USER) != RESET) )          
    {
      Toggle_Leds();
    } */
    /* Writes a text file, STM32.TXT in the disk*/
    // LCD_DrawFullRect(0, 0, 240, 320);

    GUI_DispStringAt("> Saving log....\n", 15, 220);
    // LCD_DisplayStringLine(LCD_LINE_14, "> Saving log..\n");

    if(USBH_MSC_Param.MSWriteProtect == DISK_WRITE_PROTECTED)
    {
      
      // LCD_ErrLog ( "> Disk flash is write protected \n");
      USBH_USR_ApplicationState = USH_USR_FS_DRAW;
      break;
    }
    
    /* Register work area for logical drives */
    f_mount(0, &fatfs);

    if(f_open(&file, "0:STM32.LOG",FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
    { 
      /* Write buffer to file */
      bytesToWrite = 0x10; // 0x20000;
//      res= f_write (&file, 0xD0000000 + 0x40000 ,  bytesToWrite, (void *)&bytesWritten);   
     char c; 
     char color;
      while (!cbIsEmpty(&g_LogCB)) {
        cbRead(&g_LogCB, &c);
        res = f_write(&file, &c, 1, (void *)&bytesWritten); 
      }
 
      if((bytesWritten == 0) || (res != FR_OK)) /*EOF or Error*/
      {
        // LCD_ErrLog("> STM32.TXT CANNOT be writen.\n");
      }
      else
      {
        // LCD_UsrLog("> 'STM32.TXT' file created\n");
      }
      
      /*close file and filesystem*/
      f_close(&file);
      f_mount(0, NULL); 
    }
    
    else
    {
      // LCD_UsrLog ("> STM32.TXT created in the disk\n");
    }

    // LCD_DisplayStringLine(LCD_LINE_15, "> Finished.\n");
    GUI_DispStringAt("> Finished\n", 15, 230);
    USBH_USR_ApplicationState = USH_USR_FS_NOTHING; 
    
    // LCD_SetTextColor(Green);
    // LCD_DisplayStringLine(LCD_LINE_15,"To start Image slide show           ");
    // LCD_DisplayStringLine(LCD_LINE_16,"Press Key                           "); 
    // LCD_SetTextColor(LCD_LOG_DEFAULT_COLOR); 
    /* while(STM_EVAL_PBGetState (BUTTON_USER) != SET)
      {
        Toggle_Leds();
      } */
    break;
    
  case USH_USR_FS_DRAW:
    
    /*Key B3 in polling*/
/*    while((HCD_IsDeviceConnected(&USB_OTG_Core)) && \
      ( STM_EVAL_PBGetState (BUTTON_USER) != RESET ))
    {
      Toggle_Leds();
    }
  */
    while(HCD_IsDeviceConnected(&USB_OTG_Core))
    {
	    if ( f_mount( 0, &fatfs ) != FR_OK ) 
      {
        /* fat_fs initialisation fails*/
        return(-1);
      }
      Image_Browser("0:/");
      // USB_OTG_BSP_mDelay(2000);
      /* if (g_dma_enable == 1) {
	      USB_OTG_BSP_mDelay(100);
	      // LCD_DrawFullRect(0, 0, 240, 320);
	      USB_OTG_BSP_mDelay(100);
	      LCD_DisplayStringLine(LCD_LINE_14, "> Disabling USB DMA\n");
	      LCD_DisplayStringLine(LCD_LINE_15, "> Continuing after two seconds\n");
	      USB_OTG_BSP_mDelay(100);
	      USBH_DeInit(&USB_OTG_Core, &USB_Host);
	      USBH_USR_ApplicationState = USH_USR_FS_INIT;
	      g_dma_enable = 0;
	      USBH_Init(&USB_OTG_Core, 
				 USB_OTG_HS_CORE_ID,
				 &USB_Host,
				 &USBH_MSC_cb, 
				 &USR_cb); 

      } else {
	      USBH_USR_ApplicationState = USH_USR_FS_WRITEFILE;
      } */
      
      LCD_DisplayStringLine(LCD_LINE_15, "> Press User button to continue..\n");
 
      log_append("LCD screen enters sleep mode\n");
      LCD_WriteCommand(0x10);
      vTaskDelay(2000);
      log_append("LCD screen quits sleep mode\n");
      LCD_WriteCommand(0x11);
      vTaskDelay(200);

      USBH_USR_ApplicationState = USH_USR_FS_READVIDEO;
      return (0);
    }
    break;
  case USH_USR_FS_NOTHING:
    USBH_USR_ApplicationState = USH_USR_FS_NOTHING;

    // StandbyRTCMode_Measure();
    // StopMode_Measure();

    int sw = 0;
    while (1) {
        if (sw) 
            SystemInitLQ();
        else
            SystemInit();

        SystemCoreClockUpdate();

        sw = !sw;
        vTaskDelay(1000);
    }

    break;
  default: break;
  }
  return(0);
}

/**
* @brief  Explore_Disk 
*         Displays disk content
* @param  path: pointer to root path
* @retval None
*/
uint8_t Explore_Disk (char* path , uint8_t recu_level)
{

  FRESULT res;
  FILINFO fno;
  DIR dir;
  char *fn;
  char tmp[14];
  
  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    while(HCD_IsDeviceConnected(&USB_OTG_Core)) 
    {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) 
      {
        break;
      }
      if (fno.fname[0] == '.')
      {
        continue;
      }

      fn = fno.fname;
      strcpy(tmp, fn); 

      line_idx++;
      if(line_idx > 9)
      {
        line_idx = 0;
        LCD_SetTextColor(Green);
        LCD_DisplayStringLine (LCD_LINE_15, "Press Key to continue...       ");
        LCD_DisplayStringLine (LCD_LINE_16, "                               ");
        LCD_SetTextColor(LCD_LOG_DEFAULT_COLOR); 
        
        /*Key B3 in polling*/
        /* while((HCD_IsDeviceConnected(&USB_OTG_Core)) && \
          ( STM_EVAL_PBGetState (BUTTON_USER) != SET  ))
        {
          Toggle_Leds();
          
        } */
      } 
      
      if(recu_level == 1)
      {
        // LCD_DbgLog("   |__");
      }
      else if(recu_level == 2)
      {
        // LCD_DbgLog("   |   |__");
      }
      if((fno.fattrib & AM_MASK) == AM_DIR)
      {
        strcat(tmp, "\n"); 
        // LCD_UsrLog((void *)tmp);
      }
      else
      {
        strcat(tmp, "\n"); 
        // LCD_DbgLog((void *)tmp);
      }

      if(((fno.fattrib & AM_MASK) == AM_DIR)&&(recu_level == 1))
      {
        Explore_Disk(fn, 2);
      }
    }
  }
  return res;
}

uint8_t Image_Browser (char* path)
{
  FRESULT res;
  uint8_t ret = 1, counter;
  FILINFO fno;
  DIR dir;
  char *fn;;
  
  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    
    for (counter=0;counter<15;counter++) {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) break;
      if (fno.fname[0] == '.') continue;

      fn = fno.fname;
 
      if (fno.fattrib & AM_DIR) 
      {
        continue;
      } 
      else 
      {
        if((strstr(fn, "bmp")) || (strstr(fn, "BMP")))
        {
          res = f_open(&file, fn, FA_OPEN_EXISTING | FA_READ);
          Show_Image();
          // USB_OTG_BSP_mDelay(2000);
          ret = 0;
          g_LogCB_enable = 0;
          while((HCD_IsDeviceConnected(&USB_OTG_Core)) && \
            ( STM_EVAL_PBGetState (BUTTON_USER) != SET  ))
          {
            Toggle_Leds();
          } 
          g_LogCB_enable = 1;
          f_close(&file);
	  break;
        }
      }
    }  
  }
  
  #ifdef USE_USB_OTG_HS  
  // LCD_LOG_SetHeader("USB HS Host");
#else
  // LCD_LOG_SetHeader(" USB OTG FS MSC Host");
#endif
  // LCD_LOG_SetFooter ("     USB Host Library v2.1.0" );
  // LCD_UsrLog("> Disk capacity : %d Bytes\n", USBH_MSC_Param.MSCapacity * \
      USBH_MSC_Param.MSPageLength); 
  USBH_USR_ApplicationState = USH_USR_FS_WRITEFILE; //USH_USR_FS_READLIST;
  return ret;
}


/**
  * @brief  Open a file and copy its content to a buffer
  * @param  DirName: the Directory name to open
  * @param  FileName: the file name to open
  * @param  BufferAddress: A pointer to a buffer to copy the file to
  * @param  FileLen: the File lenght
  * @retval err: Error status (0=> success, 1=> fail)
  */
uint32_t Storage_OpenReadFile(uint32_t Address)
{
  uint32_t index = 0, size = 0, i1 = 0;
  uint32_t BmpAddress;

  f_read(&file, &Image_Buf, 30,  (UINT *)&BytesRead);
  BmpAddress = (uint32_t)Image_Buf;

  /* Read bitmap size */
  size = *(uint16_t *) (BmpAddress + 2);
  size |= (*(uint16_t *) (BmpAddress + 4)) << 16;  
 
  /* Get bitmap data address offset */
  index = *(uint16_t *) (BmpAddress + 10);
  index |= (*(uint16_t *) (BmpAddress + 12)) << 16;  
  
  f_lseek (&file, 0);

  do
  {
    if (size < 256*2)
    {
      i1 = size;
    }
    else
    {
      i1 = 256*2;
    }
    size -= i1;
    f_read(&file, &Image_Buf, i1, (UINT *)&BytesRead);

    for (index = 0; index < i1; index++)
    {
      *(__IO uint8_t*) (Address) = *(__IO uint8_t *)BmpAddress;
      
      BmpAddress++;  
      Address++;
    }  
    
    BmpAddress = (uint32_t)Image_Buf;
  }
  while (size > 0);
  
  return 1;
}

/**
* @brief  Show_Image 
*         Displays BMP image
* @param  None
* @retval None
*/
void Show_Image(void)
{
  Storage_OpenReadFile(SDRAM_BANK_ADDR);
  // LCD_WriteBMP(SDRAM_BANK_ADDR);
  GUI_BMP_Draw(SDRAM_BANK_ADDR, 0, 0);
}

/**
* @brief  Toggle_Leds
*         Toggle leds to shows user input state
* @param  None
* @retval None
*/
void Toggle_Leds(void)
{
  static uint32_t i;
  if (i++ == 0x10000)
  {
    STM_EVAL_LEDToggle(LED3);
    STM_EVAL_LEDToggle(LED4);
    i = 0;
  }  
}
/**
* @brief  USBH_USR_DeInit
*         Deint User state and associated variables
* @param  None
* @retval None
*/
void USBH_USR_DeInit(void)
{
  USBH_USR_ApplicationState = USH_USR_FS_INIT;
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

/**
* @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

