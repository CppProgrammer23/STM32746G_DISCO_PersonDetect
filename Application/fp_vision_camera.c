/**
 ******************************************************************************
 * @file    fp_vision_camera.c
 * @author  MCD Application Team
 * @brief   Library to manage camera related operation
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
#include "fp_vision_camera.h"
#include "ov9655.h"

/** @addtogroup STM32H747I-DISCO_Applications
 * @{
 */

/** @addtogroup Common
 * @{
 */
 
/* Global variables ----------------------------------------------------------*/
CameraContext_TypeDef CameraContext;

/* Private function prototypes -----------------------------------------------*/

/**
 * @brief Initializes the camera context structure
 * @param  Camera_Context_Ptr Pointer to camera context
 */
void CAMERA_Context_Init(CameraContext_TypeDef *Camera_Context_Ptr)
{
  Camera_Context_Ptr->new_frame_ready=0;
  Camera_Context_Ptr->Tframe_evt=0;
  Camera_Context_Ptr->Tvsync_evt=0;
  Camera_Context_Ptr->vsync_it=0;
}

/**
 * @brief  CAMERA Initialization
 * @param  Camera_Context_Ptr Pointer to camera context
 * @retval None
 */
void CAMERA_Init(CameraContext_TypeDef* Camera_Context_Ptr)
{
  CAMERA_Context_Init(Camera_Context_Ptr);

  /* Reset and power down camera to be sure camera is Off prior start */
  BSP_CAMERA_PwrDown();
  
  /* Wait delay */ 
  HAL_Delay(200);
  
  /* Initialize the Camera */
  if (BSP_CAMERA_Init(CAMERA_RESOLUTION) != CAMERA_OK)
  {
    Error_Handler();
  }

  /* Set camera mirror / flip configuration */
  CAMERA_Set_MirrorFlip(Camera_Context_Ptr, Camera_Context_Ptr->mirror_flip);

  HAL_Delay(100);
  
  /* Start the Camera Capture */    
  /*if(BSP_CAMERA_ContinuousStart((uint8_t *)Camera_Context_Ptr->camera_capture_buffer)!=CAMERA_OK)
  {
    while(1);
  }*/
  BSP_CAMERA_ContinuousStart((uint8_t *)Camera_Context_Ptr->camera_capture_buffer);
  
  /* Wait for the camera initialization after HW reset */ 
  HAL_Delay(200);
   
#if MEMORY_SCHEME == FULL_INTERNAL_MEM_OPT
  /* Wait until camera acquisition of first frame is completed => frame ignored*/
  while (Camera_Context_Ptr->new_frame_ready == 0)
  {
    BSP_LED_Toggle(LED_GREEN);
    HAL_Delay(100);
  };
  Camera_Context_Ptr->new_frame_ready = 0;
#endif
}

/**
 * @brief Set the camera Mirror/Flip.
 * @param  Camera_Context_Ptr Pointer to camera context
 * @param  MirrorFlip CAMERA_MIRRORFLIP_NONE or any combination of
 *                    CAMERA_MIRRORFLIP_FLIP and CAMERA_MIRRORFLIP_MIRROR
 * @retval None
 */
void CAMERA_Set_MirrorFlip(CameraContext_TypeDef* Camera_Context_Ptr, uint32_t MirrorFlip)
{
  Camera_Context_Ptr->mirror_flip = MirrorFlip;
  /*if (BSP_CAMERA_SetMirrorFlip(0, Camera_Context_Ptr->mirror_flip) != CAMERA_OK)
  {
    while(1);
  }*/
}

/**
 * @brief  CAMERA Set test bar mode (grayscale bar)
 * @param  Camera_Context_Ptr Pointer to camera context
 * @retval None
 */
void                *Camera_CompObj = NULL;
void CAMERA_Set_TestBar_Mode(CameraContext_TypeDef* Camera_Context_Ptr)
{
  uint8_t com20_reg_content;
  OV9655_Object_t *pObj=Camera_CompObj;
  
  /*Send I2C command to configure the camera in test color bar mode*/
  com20_reg_content=ov9655_read_reg(&pObj->Ctx, OV9655_COMMON_CTRL20, &com20_reg_content, 1);//Read COM20 register content
  
  com20_reg_content |= 0x10;
  
  ov9655_write_reg(&pObj->Ctx, OV9655_COMMON_CTRL20, &com20_reg_content, 1);//Write COM20 register content
  
  HAL_Delay(500);
}

/**
* @brief  Camera Frame Event callback
* @param  None
* @retval None
*/
void BSP_CAMERA_FrameEventCallback()
{
  AppContext_TypeDef *App_Cxt_Ptr=CameraContext.AppCtxPtr;
  
  __disable_irq();
  
  /*Notifies the backgound task about new frame available for processing*/
  CameraContext.new_frame_ready = 1;
  
  App_Cxt_Ptr->Utils_ContextPtr->ExecTimingContext.tcapturestop = HAL_GetTick();
  
  CameraContext.new_frame_ready = 1;
  
  CameraContext.Tframe_evt=HAL_GetTick();
  
  if((CameraContext.Tframe_evt-CameraContext.Tvsync_evt)<3)
  {
    CameraContext.vsync_it =2;
  }
  
  /*Suspend acquisition of the data stream coming from camera*/
  BSP_CAMERA_Suspend();
  
  __enable_irq();
}

/**
* @brief  VSYNC Event callback.
* @retval None
*/
void BSP_CAMERA_VsyncEventCallback()
{ 
  AppContext_TypeDef *App_Cxt_Ptr=CameraContext.AppCtxPtr;
  
  __disable_irq();
  
  CameraContext.Tvsync_evt=HAL_GetTick();
  
  if(CameraContext.vsync_it==0)
  {
    CameraContext.vsync_it ++;
    App_Cxt_Ptr->Utils_ContextPtr->ExecTimingContext.tcapturestart2 = HAL_GetTick();
    App_Cxt_Ptr->Utils_ContextPtr->ExecTimingContext.tcapturestart=App_Cxt_Ptr->Utils_ContextPtr->ExecTimingContext.tcapturestart1;
  }
  else if(CameraContext.vsync_it==1 && ((CameraContext.new_frame_ready == 0) || ((CameraContext.new_frame_ready == 1) && ((CameraContext.Tvsync_evt - CameraContext.Tframe_evt) < 3))))//3 ms: in reality the time diff is in the magnitude of a few hundreds of ns, but some margin is required because there could be other interrupts in between the vsync_evt IT and the frame_evt IT.
  {
    App_Cxt_Ptr->Utils_ContextPtr->ExecTimingContext.tcapturestart=App_Cxt_Ptr->Utils_ContextPtr->ExecTimingContext.tcapturestart2;
    CameraContext.vsync_it ++;
  }
  
  if(CameraContext.Tvsync_evt - CameraContext.Tframe_evt <3)
  {
    CameraContext.vsync_it =2;
  }
  
  __enable_irq();
}

/**
 * @}
 */

/**
 * @}
 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
