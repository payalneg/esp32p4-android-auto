/*
* Copyright 2023 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include "wels/codec_api.h"
#include "wels/codec_def.h"
#include <stdint.h>
#include <stddef.h>

void Yuv420p2Rgb565(uint16_t *dst, uint8_t *yp, uint8_t *up, uint8_t*vp, size_t width, size_t height);
int OpenH264_Init(void);
int OpenH264_Decode(const unsigned char* pSrc, const int iSrcLen, unsigned char** ppDst, SBufferInfo* pDstInfo);
void OpenH264_GetOption(void* pOption);
void OpenH264_FlashFrame(unsigned char** ppDst, SBufferInfo* pDstInfo);
void OpenH264_Uninit(void);
#ifdef __cplusplus
}
#endif
