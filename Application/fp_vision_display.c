/**
 ******************************************************************************
 * @file    fp_vision_display.c
 * @author  MCD Application Team
 * @brief   Library to manage LCD display through DMA2D
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
#include "food_logos.h"
#include "main.h"
#include "stlogo.h"
#include "stm32h7logo.h"
#include "fp_vision_display.h"
#include "fp_vision_app.h"

#include "../Drivers/Include/basic_gui.h"

/** @addtogroup STM32H747I-DISCO_Applications
 * @{
 */

/** @addtogroup Common
 * @{
 */
 
/* Private defines -----------------------------------------------------------*/
#define SDRAM_BANK_SIZE   (8 * 1024 * 1024)  /*!< IS42S32800J has 4x8MB banks */

/* Global variables ----------------------------------------------------------*/
DisplayContext_TypeDef Display_Context;

/*
 * LCD display buffers in external SDRAM
 * When double framebuffer technique is used, it is recommended to have these
 * buffers in two separate banks.
 * AN4861: 4.5.3 - Optimizing the LTDC framebuffer fetching from SDRAM.
 */
#if defined(__ICCARM__)
#pragma location = "Lcd_Display"
#pragma data_alignment=32
#elif defined(__CC_ARM)
__attribute__((section(".Lcd_Display"), zero_init))
__attribute__ ((aligned (32)))
#elif defined(__GNUC__)
__attribute__((section(".Lcd_Display")))
__attribute__ ((aligned (32)))
#else
#error Unknown compiler
#endif
uint8_t lcd_display_global_memory[SDRAM_BANK_SIZE + LCD_FRAME_BUFFER_SIZE];
uint8_t *lcd_display_read_buffer = lcd_display_global_memory;
uint8_t *lcd_display_write_buffer = lcd_display_global_memory + SDRAM_BANK_SIZE;

/* Private function prototypes -----------------------------------------------*/
static void Display_Context_Init(DisplayContext_TypeDef* );

/* Functions Definition ------------------------------------------------------*/
/**
 * @brief  Display context Initialization
 * @param  DisplayContext_TypeDef* Ptr to Display context
 * @retval None
 */
static void Display_Context_Init(DisplayContext_TypeDef* Display_Context_Ptr)
{
  Display_Context_Ptr->lcd_frame_read_buff=lcd_display_read_buffer;
  Display_Context_Ptr->lcd_frame_write_buff=lcd_display_write_buffer;
  Display_Context_Ptr->lcd_sync=0;
}

/**
 * @brief  Dispaly Initialization
 * @param  DisplayContext_TypeDef* Ptr to Display context
 * @retval None
 */
BSP_LCD_Ctx_t       Lcd_Ctx[1];
LTDC_HandleTypeDef  hlcd_ltdc;
GUI_Drv_t LCD_Drv =
{
  BSP_LCD_DrawBitmap,
  //BSP_LCD_FillRGBRect,
  BSP_LCD_DrawHLine,
  BSP_LCD_DrawVLine,
  BSP_LCD_FillRect,
  BSP_LCD_ReadPixel,
  //BSP_LCD_WritePixel,
  BSP_LCD_GetXSize,
  BSP_LCD_GetYSize,
  //BSP_LCD_SetActiveLayer,
  //BSP_LCD_GetPixelFormat
};
void DISPLAY_Init(DisplayContext_TypeDef* Display_Context_Ptr)
{
  MX_LTDC_LayerConfig_t config;

  Display_Context_Init(Display_Context_Ptr);

  /*
   * Disable FMC Bank1 to prevent CPU speculative read accesses
   * AN4861: 4.6.1 Disable FMC bank1 if not used.
   */
  __FMC_NORSRAM_DISABLE(FMC_Bank1, FMC_NORSRAM_BANK1);

  BSP_LCD_Init();/*by default, 0xC0000000 is used as start address for lcd frame buffer*/

  config.X0          = 0;
  config.X1          = LCD_DEFAULT_WIDTH;
  config.Y0          = 0;
  config.Y1          = LCD_DEFAULT_HEIGHT;
  config.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  config.Address     = (uint32_t)Display_Context_Ptr->lcd_frame_read_buff;/*lcd_frame_read_buff buffer used as lcd frame buffer*/
  //BSP_LCD_ConfigLayer(0, 0, &config);//overwrite config
  
  GUI_SetFuncDriver(&LCD_Drv);
  GUI_SetLayer(0);
  
  GUI_SetBackColor(GUI_COLOR_BLACK);
  GUI_SetTextColor(GUI_COLOR_WHITE);
  GUI_SetFont(&Font24);
  
  /*Use lcd_frame_write_buff buffer for display composition*/
  hlcd_ltdc.LayerCfg[Lcd_Ctx[0].ActiveLayer].FBStartAdress=(uint32_t)Display_Context_Ptr->lcd_frame_write_buff;
  
  /*LCD sync: set LTDCreload type to vertical blanking*/
  HAL_LTDC_Reload(&hlcd_ltdc, LTDC_RELOAD_VERTICAL_BLANKING);
}

/**
* @brief Displays a Welcome screen with information about the memory and camera configuration. Also test for WakeUp
* button input in order to start the magic menu.
*
* @param  DisplayContext_TypeDef* Ptr to Display context
* @return int boolean value, 1 if WakeUp button has been pressed, 0 otherwise
*/
int DISPLAY_WelcomeScreen(DisplayContext_TypeDef* Display_Context_Ptr)
{
  int magic_menu = 0;
  GUI_Clear(GUI_COLOR_BLACK);

  /* Draw logos.*/
  BSP_LCD_DrawBitmap(100, 77, (uint8_t *)stlogo);
  BSP_LCD_DrawBitmap(620, 65, (uint8_t *)stm32h7logo);

  /*Display welcome message*/
  GUI_DisplayStringAt(0, LINE(6), (uint8_t *)"VISION1 Function Pack", CENTER_MODE);
  GUI_DisplayStringAt(0, LINE(7), (uint8_t *)"V2.0.0", CENTER_MODE);
  GUI_DisplayStringAt(0, LINE(10), (uint8_t *)WELCOME_MSG_0, CENTER_MODE);
  GUI_DisplayStringAt(0, LINE(11), (uint8_t *)WELCOME_MSG_2, CENTER_MODE);
  GUI_DisplayStringAt(0, LINE(12), (uint8_t *)WELCOME_MSG_1, CENTER_MODE);
  GUI_DisplayStringAt(0, LINE(13), (uint8_t *)WELCOME_MSG_4, CENTER_MODE);
  GUI_DisplayStringAt(0, LINE(14), (uint8_t *)WELCOME_MSG_5, CENTER_MODE);
  GUI_DisplayStringAt(0, LINE(15), (uint8_t *)WELCOME_MSG_3, CENTER_MODE);

  DISPLAY_Refresh(Display_Context_Ptr);

  for (int i = 0; i < 5; i++)
  {
    HAL_Delay(500);
    if (BSP_PB_GetState(BUTTON_WAKEUP) != RESET)
    {
      magic_menu = 1;
    }
  }

  GUI_Clear(GUI_COLOR_BLACK);

  return magic_menu;
}

/**
* @brief Displays a food logo to LCD
*
* @param  DisplayContext_TypeDef* Ptr to Display context
* @param x x position in pixel
* @param y y position in pixel
* @param index Index of logo to be displayed. Should be between 0 and number of logos
 */
void DISPLAY_FoodLogo(DisplayContext_TypeDef* Display_Context_Ptr, const uint32_t x, const uint32_t y, const size_t index)
{
  DISPLAY_Copy2LCDWriteBuffer(Display_Context_Ptr, (uint32_t *)(Logos_128x128_raw[index]), x,
                               y, 128, 128, DMA2D_INPUT_RGB888,
                               1);
}

/**
* @brief Refreshes LCD screen by performing a DMA transfer from lcd write buffer to lcd read buffer
* @param  DisplayContext_TypeDef* Ptr to Display context
*
*/
void DISPLAY_Refresh(DisplayContext_TypeDef* Display_Context_Ptr)
{
  /*LCD sync: wait for next VSYNC event before refreshing, i.e. before updating the content of the buffer that will be read by the LTDC for display. 
  The refresh occurs during the blanking period => this sync mecanism should enable to avoid tearing effect*/
  Display_Context_Ptr->lcd_sync =0;
  while(Display_Context_Ptr->lcd_sync==0);
  
  /*Coherency purpose: clean the lcd_frame_write_buff area in L1 D-Cache before DMA2D reading*/
  UTILS_DCache_Coherency_Maintenance((void *)Display_Context_Ptr->lcd_frame_write_buff, LCD_FRAME_BUFFER_SIZE, CLEAN);
  
  UTILS_Dma2d_Memcpy((uint32_t *)(Display_Context_Ptr->lcd_frame_write_buff), (uint32_t *)(Display_Context_Ptr->lcd_frame_read_buff), 0, 0, LCD_RES_WIDTH,
                     LCD_RES_HEIGHT, LCD_RES_WIDTH, DMA2D_INPUT_ARGB8888, DMA2D_OUTPUT_ARGB8888, 0, 0);
}

/**
* @brief Performs a DMA transfer from buffer to LCD write buffer with optional pixel format conversion and red/blue
* channel swap
*
* @param DisplayContext_TypeDef* Ptr to Display context
* @param pSrc pointer to input buffer
* @param x x position on LCD in pixels
* @param y y position on LCD in pixels
* @param xsize width of the image to write in pixels
* @param ysize height of the image to write in pixels
* @param input_color_format input color format (e.g DMA2D_INPUT_RGB888)
* @param red_blue_swap boolean flag for red-blue channel swap, 0 is no swap, 1 is swap
*/
void DISPLAY_Copy2LCDWriteBuffer(DisplayContext_TypeDef* Display_Context_Ptr, uint32_t *pSrc, uint16_t x, uint16_t y, uint16_t xsize, uint16_t ysize,
                              uint32_t input_color_format, int red_blue_swap)
{
  UTILS_Dma2d_Memcpy((uint32_t *)pSrc, (uint32_t *)Display_Context_Ptr->lcd_frame_write_buff, x, y, xsize, ysize, LCD_RES_WIDTH,
                input_color_format, DMA2D_OUTPUT_ARGB8888, 1, red_blue_swap);
}

void HAL_LTDC_ReloadEventCallback(LTDC_HandleTypeDef *hltdc)
{
  Display_Context.lcd_sync=1;
  
  /*Set LTDCreload type to vertical blanking*/
  HAL_LTDC_Reload(hltdc, LTDC_RELOAD_VERTICAL_BLANKING);
}

/**
 * @}
 */

/**
 * @}
 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
