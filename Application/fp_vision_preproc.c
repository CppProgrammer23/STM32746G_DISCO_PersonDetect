/**
 ******************************************************************************
 * @file    fp_vision_preproc.c
 * @author  MCD Application Team
 * @brief   FP VISION Pre-Processing configuration
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

/* Includes ------------------------------------------------------------------*/
#include "fp_vision_preproc.h"
#include "fp_vision_utils.h"
#include <stdio.h>
#include <string.h>

/** @addtogroup STM32H747I-DISCO_Applications
 * @{
 */

/** @addtogroup Common
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
PreprocContext_TypeDef Preproc_Context;

/* Private function prototypes -----------------------------------------------*/
static void Preproc_Context_Init(PreprocContext_TypeDef *);

/* Functions Definition ------------------------------------------------------*/

/**
 * @brief Initializes the PreProcessing context structure
 * @param Preproc_Context_Ptr Pointer to PREPROC context
 */
static void Preproc_Context_Init(PreprocContext_TypeDef * Preproc_Context_Ptr)
{
  ;
}

/**
 * @brief PREPROC Init
 * @param Preproc_Context_Ptr Pointer to PREPROC context
 */
void PREPROC_Init(PreprocContext_TypeDef * Preproc_Context_Ptr)
{
  Preproc_Context_Init(Preproc_Context_Ptr);
}

/**
* @brief  Performs image (or selected Region Of Interest) resizing
* @param  srcImage     Pointer to source image buffer
* @param  srcW         Source image width
* @param  srcH         Source image height
* @param  pixelSize    Number of bytes per pixel
* @param  roiX         Region Of Interest x starting location 
* @param  roiY         Region Of Interest y starting location
* @param  roiW         Region Of Interest width
* @param  roiH         Region Of Interest height
* @param  dstImage     Pointer to destination image buffer
* @param  dstW         Destination image width
* @param  dstH         Destination image height
* @param  Top2Bottom   Value of 1/0 indicates that the rescales performs from the top/bottom to the bottom/top of the buffers
* @retval void         None
*/
void PREPROC_ImageResize(PreprocContext_TypeDef* Preproc_Context_Ptr)
{  
#if RESIZING_ALGO == RESIZING_NEAREST_NEIGHBOR
  ImageResize_NearestNeighbor(&Preproc_Context_Ptr->Resize_Src_Img, 
                              &Preproc_Context_Ptr->Resize_Dst_Img,
                              &Preproc_Context_Ptr->Roi);
/*
#else
 #error Please check definition of RESIZING_ALGO define
*/
#endif

  ImageResize_NearestNeighbor(&Preproc_Context_Ptr->Resize_Src_Img,
                                &Preproc_Context_Ptr->Resize_Dst_Img,
                                &Preproc_Context_Ptr->Roi);
}

/**
 * @brief Performs a pixel format conversion either by HW or SW
 *
 * @param pSrc address of the source
 * @param pDst address of the destination
 * @param x x position in the destination
 * @param y y position in the destination
 * @param xsize width of the source
 * @param ysize height of the source
 * @param rowStride width of the destination
 * @retval void         None
*/
void PREPROC_PixelFormatConversion(PreprocContext_TypeDef* Preproc_Context_Ptr)
{
#if PIXEL_FMT_CONV == HW_PFC
  if(Preproc_Context_Ptr->Pfc_Src_Img.format==PXFMT_RGB565 && 
     Preproc_Context_Ptr->Pfc_Dst_Img.format==PXFMT_RGB888)
  {
    /*DMA2D transfer w/ PFC*/
    UTILS_Dma2d_Memcpy((uint32_t *)(Preproc_Context_Ptr->Pfc_Src_Img.pData), 
                       (uint32_t *)(Preproc_Context_Ptr->Pfc_Dst_Img.pData), 
                       Preproc_Context_Ptr->Dma2dcfg.x,
                       Preproc_Context_Ptr->Dma2dcfg.y, 
                       Preproc_Context_Ptr->Pfc_Src_Img.width, 
                       Preproc_Context_Ptr->Pfc_Src_Img.height,
                       Preproc_Context_Ptr->Dma2dcfg.rowStride, 
                       DMA2D_INPUT_RGB565, 
                       DMA2D_OUTPUT_RGB888, 
                       1, 
                       Preproc_Context_Ptr->red_blue_swap);
  }
  else
  {
    while(1);
  }
#elif PIXEL_FMT_CONV == SW_PFC
  if(Preproc_Context_Ptr->Pfc_Src_Img.format==PXFMT_RGB565 &&
     Preproc_Context_Ptr->Pfc_Dst_Img.format==PXFMT_RGB888)
  {
    ImagePfc_Rgb565ToRgb888(&Preproc_Context_Ptr->Pfc_Src_Img,
                            &Preproc_Context_Ptr->Pfc_Dst_Img,
                            Preproc_Context_Ptr->red_blue_swap);
  }
  else if(Preproc_Context_Ptr->Pfc_Src_Img.format==PXFMT_RGB565 &&
          Preproc_Context_Ptr->Pfc_Dst_Img.format==PXFMT_GRAY8)
  {
    ImagePfc_Rgb565ToGrayscale(&Preproc_Context_Ptr->Pfc_Src_Img,
                               &Preproc_Context_Ptr->Pfc_Dst_Img);
  }
  else
  {
    while(1);
  }
/*
#else
 #error PFC method not valid!
*/
#endif

  if(Preproc_Context_Ptr->Pfc_Src_Img.format==PXFMT_RGB565 &&
       Preproc_Context_Ptr->Pfc_Dst_Img.format==PXFMT_RGB888)
    {
      ImagePfc_Rgb565ToRgb888(&Preproc_Context_Ptr->Pfc_Src_Img,
                              &Preproc_Context_Ptr->Pfc_Dst_Img,
                              Preproc_Context_Ptr->red_blue_swap);
    }
    else if(Preproc_Context_Ptr->Pfc_Src_Img.format==PXFMT_RGB565 &&
            Preproc_Context_Ptr->Pfc_Dst_Img.format==PXFMT_GRAY8)
    {
      ImagePfc_Rgb565ToGrayscale(&Preproc_Context_Ptr->Pfc_Src_Img,
                                 &Preproc_Context_Ptr->Pfc_Dst_Img);
    }
    else
    {
      while(1);
    }
}

/**
* @brief  Performs pixel R & B swapping 
* @param  pSrc            Pointer to source buffer
* @param  pDst            Pointer to destination buffer
* @param  pixels          Number of pixels
* @param  SrcBuff_Size    Size (in bytes) of the source buffer
* @retval void     None
*/
void PREPROC_Pixel_RB_Swap(void *pSrc, void *pDst, uint32_t pixels)
{
  struct rgb_Src
  {
    uint8_t r, g, b;
  };
  
  struct rgb_Dst
  {
    uint8_t r, g, b;
  };
  
  uint8_t tmp_r;
  
  struct rgb_Src *pivot = (struct rgb_Src *) pSrc;
  struct rgb_Dst *dest = (struct rgb_Dst *) pDst;
  
  for (int i = pixels-1; i >= 0; i--)
  {
    tmp_r=pivot[i].r;
    
    dest[i].r = pivot[i].b;
    dest[i].b = tmp_r;
    dest[i].g = pivot[i].g;
  }
}

/**
 * @}
 */

/**
 * @}
 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
