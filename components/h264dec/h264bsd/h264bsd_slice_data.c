/*
 * Copyright (C) 2009 The Android Open Source Project
 * Modified for use by h264bsd standalone library
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*------------------------------------------------------------------------------

    Table of contents

     1. Include headers
     2. External compiler flags
     3. Module defines
     4. Local function prototypes
     5. Functions
          h264bsdDecodeSliceData
          SetMbParams
          h264bsdMarkSliceCorrupted

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "h264bsd_slice_data.h"
#include "h264bsd_esp.h"
#include <stdio.h>
#include "h264bsd_dpb.h"
#include "h264bsd_util.h"
#include "h264bsd_vlc.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

static void SetMbParams(mbStorage_t *pMb, sliceHeader_t *pSlice, u32 sliceId,
    i32 chromaQpIndexOffset);

/*------------------------------------------------------------------------------

   5.1  Function name: h264bsdDecodeSliceData

        Functional description:
            Decode one slice. Function decodes stream data, i.e. macroblocks
            and possible skip_run fields. h264bsdDecodeMacroblock function is
            called to handle all other macroblock related processing.
            Macroblock to slice group mapping is considered when next
            macroblock to process is determined (h264bsdNextMbAddress function)
            map

        Inputs:
            pStrmData       pointer to stream data structure
            pStorage        pointer to storage structure
            currImage       pointer to current processed picture, needed for
                            intra prediction of the macroblocks
            pSliceHeader    pointer to slice header of the current slice

        Outputs:
            currImage       processed macroblocks are written to current image
            pStorage        mbStorage structure of each processed macroblock
                            is updated here

        Returns:
            HANTRO_OK       success
            HANTRO_NOK      invalid stream data

------------------------------------------------------------------------------*/

#ifdef H264BSD_ESP_STATS
#include "esp_cpu.h"
u64 g_h264bsd_cyc_skip_mb, g_h264bsd_cyc_coded_mb, g_h264bsd_cyc_loop;
u32 g_h264bsd_n_skip_mb, g_h264bsd_n_coded_mb;
u64 g_h264bsd_cyc_parse, g_h264bsd_cyc_residual, g_h264bsd_cyc_mc,
    g_h264bsd_cyc_write, g_h264bsd_cyc_intra;
#define CYC() ((u32)esp_cpu_get_cycle_count())
#else
#define CYC() 0u
#endif

u32 g_h264bsd_pic_aliased;
u32 g_h264bsd_pic_copied;
#ifdef H264BSD_ESP_FASTPATH

/* Make the current picture hold the reference's pixels. With one reference
 * frame the DPB's sliding window drops the old reference the moment this
 * picture is marked, so its buffer is about to become the free slot anyway:
 * swap the two buffers' storage instead of copying 576 KB. With more
 * reference frames the old one stays in use — copy. */
static void EspWholePictureFromRef(storage_t *pStorage, image_t *currImage, u8 *ref)
{
    dpbStorage_t *dpb = pStorage->dpb;
    dpbPicture_t *cur = dpb->currentOut;
    dpbPicture_t *refPic = dpb->list[0];

    g_h264bsd_skip_zero_mbs  = pStorage->picSizeInMbs;
    g_h264bsd_skip_ref       = ref;
    g_h264bsd_skip_ref_mixed = 0;
    pStorage->espPicIsRefCopy = 1;

    if (dpb->maxRefFrames == 1 && refPic && refPic->data == ref &&
        cur && cur->data == currImage->data)
    {
        u8 *tmpData  = cur->data;
        u8 *tmpAlloc = cur->pAllocatedData;
        cur->data           = refPic->data;
        cur->pAllocatedData = refPic->pAllocatedData;
        refPic->data           = tmpData;
        refPic->pAllocatedData = tmpAlloc;
        currImage->data = cur->data;
        g_h264bsd_pic_aliased++;
        return;
    }

    u32 bytes = pStorage->picSizeInMbs * 384;
    const u64 *src = (const u64*)ref;
    u64 *dst = (u64*)currImage->data;
    for (u32 i = bytes / 32; i; i--)
    {
        dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
        src += 4; dst += 4;
    }
    g_h264bsd_pic_copied++;
}
#endif

u32 h264bsdDecodeSliceData(strmData_t *pStrmData, storage_t *pStorage,
    image_t *currImage, sliceHeader_t *pSliceHeader)
{

/* Variables */

    u8 mbData[384 + 15 + 32];
    u8 *data;
    u32 tmp;
    u32 skipRun;
    u32 prevSkipped;
    u32 currMbAddr;
    u32 moreMbs;
    u32 mbCount;
    i32 qpY;
    macroblockLayer_t *mbLayer;

/* Code */

    ASSERT(pStrmData);
    ASSERT(pSliceHeader);
    ASSERT(pStorage);
    ASSERT(pSliceHeader->firstMbInSlice < pStorage->picSizeInMbs);

    /* ensure 16-byte alignment */
    data = (u8*)ALIGN(mbData, 16);

    mbLayer = pStorage->mbLayer;

    currMbAddr = pSliceHeader->firstMbInSlice;
    skipRun = 0;
    prevSkipped = HANTRO_FALSE;

    /* increment slice index, will be one for decoding of the first slice of
     * the picture */
    pStorage->slice->sliceId++;

    /* lastMbAddr stores address of the macroblock that was last successfully
     * decoded, needed for error handling */
    pStorage->slice->lastMbAddr = 0;

    mbCount = 0;
    /* initial quantization parameter for the slice is obtained as the sum of
     * initial QP for the picture and sliceQpDelta for the current slice */
    qpY = (i32)pStorage->activePps->picInitQp + pSliceHeader->sliceQpDelta;
    do
    {
        u32 cyc0 = CYC();
        /* primary picture and already decoded macroblock -> error */
        if (!pSliceHeader->redundantPicCnt && pStorage->mb[currMbAddr].decoded)
        {
            EPRINT("Primary and already decoded");
            return(HANTRO_NOK);
        }

        SetMbParams(pStorage->mb + currMbAddr, pSliceHeader,
            pStorage->slice->sliceId, pStorage->activePps->chromaQpIndexOffset);

        if (!IS_I_SLICE(pSliceHeader->sliceType))
        {
            if (!prevSkipped)
            {
                tmp = h264bsdDecodeExpGolombUnsigned(pStrmData, &skipRun);
                if (tmp != HANTRO_OK)
                    return(tmp);
                /* skip_run shall be less than or equal to number of
                 * macroblocks left */
                if (skipRun > (pStorage->picSizeInMbs - currMbAddr))
                {
                    EPRINT("skip_run");
                    return(HANTRO_NOK);
                }
                if (skipRun)
                {
                    prevSkipped = HANTRO_TRUE;
                    memset(&mbLayer->mbPred, 0, sizeof(mbPred_t));
                    /* mark current macroblock skipped */
                    mbLayer->mbType = P_Skip;
                }
#ifdef H264BSD_ESP_FASTPATH
                /* The whole picture is one skip run: every macroblock is a
                 * P_Skip whose predicted motion vector is zero (the first MB
                 * has no neighbours, every later one has a zero-MV refIdx-0
                 * neighbour), no residual, and deblocking finds bS == 0 on
                 * every edge. The picture IS reference list[0], bit for bit.
                 * Skip the 1500-macroblock loop and the deblocking pass; the
                 * next picture only ever reads this picture's pixels, never
                 * its macroblock records. */
                if (skipRun == pStorage->picSizeInMbs && currMbAddr == 0 &&
                    !pSliceHeader->redundantPicCnt &&
                    pStorage->activePps->numSliceGroups == 1 &&
                    !h264bsdMoreRbspData(pStrmData))
                {
                    u8 *ref = h264bsdGetRefPicData(pStorage->dpb, 0);
                    if (ref)
                    {
                        EspWholePictureFromRef(pStorage, currImage, ref);
                        pStorage->slice->numDecodedMbs = pStorage->picSizeInMbs;
                        return(HANTRO_OK);
                    }
                }
#endif
            }
        }

        if (skipRun)
        {
            DEBUG(("Skipping macroblock %d\n", currMbAddr));
            skipRun--;
        }
        else
        {
            prevSkipped = HANTRO_FALSE;
            u32 cp0 = CYC();
            tmp = h264bsdDecodeMacroblockLayer(pStrmData, mbLayer,
                pStorage->mb + currMbAddr, pSliceHeader->sliceType,
                pSliceHeader->numRefIdxL0Active);
#ifdef H264BSD_ESP_STATS
            g_h264bsd_cyc_parse += CYC() - cp0;
#endif
            if (tmp != HANTRO_OK)
            {
                EPRINT("macroblock_layer");
                return(tmp);
            }
        }

        tmp = h264bsdDecodeMacroblock(pStorage->mb + currMbAddr, mbLayer,
            currImage, pStorage->dpb, &qpY, currMbAddr,
            pStorage->activePps->constrainedIntraPredFlag, data);
        if (tmp != HANTRO_OK)
        {
            EPRINT("MACRO_BLOCK");
            return(tmp);
        }
#ifdef H264BSD_ESP_STATS
        {
            u32 cyc1 = CYC();
            if (mbLayer->mbType == P_Skip && prevSkipped)
            {
                g_h264bsd_cyc_skip_mb += cyc1 - cyc0;
                g_h264bsd_n_skip_mb++;
            }
            else
            {
                g_h264bsd_cyc_coded_mb += cyc1 - cyc0;
                g_h264bsd_n_coded_mb++;
            }
            cyc0 = cyc1;
        }
#endif

        /* increment macroblock count only for macroblocks that were decoded
         * for the first time (redundant slices) */
        if (pStorage->mb[currMbAddr].decoded == 1)
            mbCount++;

        /* keep on processing as long as there is stream data left or
         * processing of macroblocks to be skipped based on the last skipRun is
         * not finished */
        moreMbs = (h264bsdMoreRbspData(pStrmData) || skipRun) ?
                                        HANTRO_TRUE : HANTRO_FALSE;

        /* lastMbAddr is only updated for intra slices (all macroblocks of
         * inter slices will be lost in case of an error) */
        if (IS_I_SLICE(pSliceHeader->sliceType))
            pStorage->slice->lastMbAddr = currMbAddr;

        currMbAddr = h264bsdNextMbAddress(pStorage->sliceGroupMap,
            pStorage->picSizeInMbs, currMbAddr);
#ifdef H264BSD_ESP_STATS
        g_h264bsd_cyc_loop += CYC() - cyc0;
#endif
        /* data left in the buffer but no more macroblocks for current slice
         * group -> error */
        if (moreMbs && !currMbAddr)
        {
            EPRINT("Next mb address");
            return(HANTRO_NOK);
        }

    } while (moreMbs);

    if ((pStorage->slice->numDecodedMbs + mbCount) > pStorage->picSizeInMbs)
    {
        EPRINT("Num decoded mbs");
        return(HANTRO_NOK);
    }

    pStorage->slice->numDecodedMbs += mbCount;

    return(HANTRO_OK);

}

/*------------------------------------------------------------------------------

   5.2  Function: SetMbParams

        Functional description:
            Set macroblock parameters that remain constant for this slice

        Inputs:
            pSlice      pointer to current slice header
            sliceId     id of the current slice
            chromaQpIndexOffset

        Outputs:
            pMb         pointer to macroblock structure which is updated

        Returns:
            none

------------------------------------------------------------------------------*/

void SetMbParams(mbStorage_t *pMb, sliceHeader_t *pSlice, u32 sliceId,
    i32 chromaQpIndexOffset)
{

/* Variables */
    u32 tmp1;
    i32 tmp2, tmp3;

/* Code */

    tmp1 = pSlice->disableDeblockingFilterIdc;
    tmp2 = pSlice->sliceAlphaC0Offset;
    tmp3 = pSlice->sliceBetaOffset;
    pMb->sliceId = sliceId;
    pMb->disableDeblockingFilterIdc = tmp1;
    pMb->filterOffsetA = tmp2;
    pMb->filterOffsetB = tmp3;
    pMb->chromaQpIndexOffset = chromaQpIndexOffset;

}

/*------------------------------------------------------------------------------

   5.3  Function name: h264bsdMarkSliceCorrupted

        Functional description:
            Mark macroblocks of the slice corrupted. If lastMbAddr in the slice
            storage is set -> picWidhtInMbs (or at least 10) macroblocks back
            from  the lastMbAddr are marked corrupted. However, if lastMbAddr
            is not set -> all macroblocks of the slice are marked.

        Inputs:
            pStorage        pointer to storage structure
            firstMbInSlice  address of the first macroblock in the slice, this
                            identifies the slice to be marked corrupted

        Outputs:
            pStorage        mbStorage for the corrupted macroblocks updated

        Returns:
            none

------------------------------------------------------------------------------*/

void h264bsdMarkSliceCorrupted(storage_t *pStorage, u32 firstMbInSlice)
{

/* Variables */

    u32 tmp, i;
    u32 sliceId;
    u32 currMbAddr;

/* Code */

    ASSERT(pStorage);
    ASSERT(firstMbInSlice < pStorage->picSizeInMbs);

    currMbAddr = firstMbInSlice;

    sliceId = pStorage->slice->sliceId;

    /* DecodeSliceData sets lastMbAddr for I slices -> if it was set, go back
     * MAX(picWidthInMbs, 10) macroblocks and start marking from there */
    if (pStorage->slice->lastMbAddr)
    {
        ASSERT(pStorage->mb[pStorage->slice->lastMbAddr].sliceId == sliceId);
        i = pStorage->slice->lastMbAddr - 1;
        tmp = 0;
        while (i > currMbAddr)
        {
            if (pStorage->mb[i].sliceId == sliceId)
            {
                tmp++;
                if (tmp >= MAX(pStorage->activeSps->picWidthInMbs, 10))
                    break;
            }
            i--;
        }
        currMbAddr = i;
    }

    do
    {

        if ( (pStorage->mb[currMbAddr].sliceId == sliceId) &&
             (pStorage->mb[currMbAddr].decoded) )
        {
            pStorage->mb[currMbAddr].decoded--;
        }
        else
        {
            break;
        }

        currMbAddr = h264bsdNextMbAddress(pStorage->sliceGroupMap,
            pStorage->picSizeInMbs, currMbAddr);

    } while (currMbAddr);

}

