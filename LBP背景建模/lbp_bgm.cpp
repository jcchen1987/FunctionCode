#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ai_defs.h"
#include "lbp.h"

#ifdef WIN32
#include "intrinsic.h"
#else

#endif

//#include "opt_inc.h"

//��ͨ�����趨�Ĳ���
#define LBP_HYSTERSIS 3
#define LBP_HISTORY 300
//GMMģ�Ͳ���
#define LBP_TP_LOW 0.50f
#define LBP_TP_HI  0.60f
#define LBP_TB 0.75f  
#define LBP_INIT_WEIGHT 0.01f
//ͳ������֡��
#define LBP_NOISYSUM_FRAMES 4500  //3����
#define BLOCK_WIDTH  8
#define BLOCK_HEIGHT 6
#define HIST_BINS    64
//hist bins is (1<<P)


/////////////////////////////////////////////////////////////////////
//���¹̶������޸ģ��Ż������мٶ���,C��������ݺ깤����
#define LBP_MAX_MODELS      3
#define HIST_FIX_BIT_CNT    9


#ifdef ERR
#define MALLOC AVMalloc
#define FREE AVFree
#else
#define MALLOC malloc
#define FREE free
#endif

#ifndef PI
#define PI 3.1415926f
#endif

typedef struct
{
	f32 f32Weight;
	u32 au32Hist[HIST_BINS];
}TPixelLBPModel;

typedef struct
{
	l32 l32Width;  //ͼ��Ŀ��
	l32 l32Height;  //ͼ��ĸ߶�
	
    l32 l32BlockW;  //Patch�������
    l32 l32BlockH;  //Patch�����߶�
	TPixelLBPModel *ptLBP;   //LBP����ģ�Ͳ���

	u8 *pu8Texture;
    l32 l32Frames;

    //ͳ��ֱ��ͼ���
    l32 l32HistBin;
    u8 *pu8Hist;

    //����������ģ�ͼ�⵽�������������LBP��ֵ
    u8 *apu8LBPFG[2];
    l32 *pl32LBPNoisy;   //ǰ��������Ĵ���
    l32 l32NoisySumFrames;  //ͳ��Nosiy����֡��
    u8 *pu8IsNosiy;      //�Ƿ����Noisy������δ��ʱ���ģ���Ƿ���ܸ�����ֵ

    u8 * pu8TempBuff;   //��ʱ����
}TLBPModel;



static f32 CalcHistIntersect(u8 *pu8HistA, f32 *pu32HistB, l32 l32Dim)
{
	l32 l32Index;
	f32 u32Dist = 0;
	for(l32Index = 0; l32Index < l32Dim; l32Index++)
	{
		u32Dist += MIN(pu8HistA[l32Index], (pu32HistB[l32Index]));
	}
	return u32Dist;
}

static u32 CalcHistIntersectU32(u8 *pu8HistA, u32 *pu32HistB, l32 l32Dim)
{
	l32 i;
	u32 u32Dist = 0;
    u32 u32Data0, u32Data1;
	for(i = 0; i < l32Dim; i++)
	{
        u32Data0 = (pu8HistA[i] << HIST_FIX_BIT_CNT);
        u32Data1 = pu32HistB[i];
		u32Dist += MIN(u32Data0, u32Data1);
	}
	return u32Dist;
}
static u32 CalcHistIntersectU8(u8 *pu8HistA, u32 *pu32HistB, l32 l32Dim)
{
	l32 i;
	u32 u32Dist = 0;
	for(i = 0; i < l32Dim; i++)
	{
		u32Dist += MIN(pu8HistA[i], (pu32HistB[i] >> HIST_FIX_BIT_CNT));
	}
	return u32Dist;
}

const int CompareTable[8][3]=
{
    {0,1,2},//
    {0,2,1},
    {1,0,2},
    {0,1,2},
    {2,1,0},
    {2,0,1},
    {1,2,0},
    {0,1,2}//
};


//�޼�Ȩ����ѧϰ��DSP�汾(���������滻��ʽѧϰ),�ڲ�ģ���������ͼ�Ϊu8��������Լ�����Ż�����
static u8 RegionProcess_GMM_noLearn_DSP(TPixelLBPModel *ptLBP, u8 *pu8Hist, l32 l32Offset, f32 f32LearnRate, f32 f32Tp, u8 u8Label)
{
    l32 l32Index;
    TPixelLBPModel *ptMatch = NULL;
    TPixelLBPModel *ptCurLBP = ptLBP + LBP_MAX_MODELS * l32Offset; //��ǰ���ص�LBP
    f32 f32TotalWeight;
    u32 u32Dist, u32Tp;
    u32 u32Data0, u32Data1;
    u8 u8FG = 0;
    u8 * restrict pu8HistA = pu8Hist;

    /*
    //��̬Ŀ�괦����أ�Ŀǰ�Ȳ�֧�֣����ǽӿڱ���
    if(u8Label == 1)
    {
        f32LearnRate = 0;
    }
    if(u8Label == 2)
    {    
        for(l32Index = 0; l32Index < HIST_BINS; l32Index++) 
            ptCurLBP->au32Hist[l32Index] = pu8Hist[l32Index] << HIST_FIX_BIT_CNT;
        return 0;
    }
    */

    // see if the current pixel matches any of the models
    u32Tp = (u32)(f32Tp + 0.499f);
    
    //���մӴ�С��������
    int compid;
    float v0, v1, v2;
    v0 = ptCurLBP[0].f32Weight;
    v1 = ptCurLBP[1].f32Weight;
    v2 = ptCurLBP[2].f32Weight;
    compid = (v0 > v1) + (v1 > v2)*2 + (v2 > v0)*4;
    
    f32TotalWeight = 0.0f;
    for(l32Index = 0; l32Index < 3; l32Index++)
    {
        ptMatch = ptCurLBP + CompareTable[compid][l32Index];
        //64Bin
        u8 * restrict pu8HistB = (u8*)ptMatch->au32Hist;
        u32Dist = 0;
	    for(int i = 0; i < HIST_BINS; i+=4)
	    {
            u32Data0 = _amem4(pu8HistA + i);
            u32Data1 = _amem4(pu8HistB + i);
            u32Data0 = _minu4(u32Data0, u32Data1);
			u32Dist = _add4(u32Dist, u32Data0);
	    }
        u32Dist = _dotpu4(u32Dist, 0x01010101);
        if(u32Dist > u32Tp) break;
        f32TotalWeight += ptMatch->f32Weight;
    }
    
    if(l32Index < 3)
    {
        //�ҵ�ƥ��
        if(f32TotalWeight > LBP_TB) u8FG = 255;  //���ж�Ϊǰ��Component
        //��Ȩֵ����ѧϰ
        //ͳ�Ʊ����˷�֧��������У�Ϊ�������ٶȣ�ʹ�������Ȩ���޸Ĳ����Ա����һ��������
        float f1Nr = (1.0f - f32LearnRate);
        ptCurLBP[0].f32Weight *= f1Nr;
        ptCurLBP[1].f32Weight *= f1Nr;
        ptCurLBP[2].f32Weight *= f1Nr;
        ptMatch->f32Weight += f32LearnRate;
    }
    else
    {
        //δ�ҵ�ƥ�䣬�滻Ȩ����͵��Ǹ�ģ��
        u8FG = 255;
        ptMatch = ptCurLBP + CompareTable[compid][2];
        ptMatch->f32Weight = LBP_INIT_WEIGHT;

        u8 * restrict pu8HistB = (u8*)ptMatch->au32Hist;
        memcpy(pu8HistB, pu8HistA, HIST_BINS);

        //��һ��Ȩ��
        f32TotalWeight = ptCurLBP[0].f32Weight + ptCurLBP[1].f32Weight + ptCurLBP[2].f32Weight;
        f32TotalWeight = _rcpsp(f32TotalWeight);
        ptCurLBP[0].f32Weight *= f32TotalWeight;
        ptCurLBP[1].f32Weight *= f32TotalWeight;
        ptCurLBP[2].f32Weight *= f32TotalWeight;
    }
    return u8FG;
}

//�޼�Ȩ����ѧϰ��C�汾(���������滻��ʽѧϰ)
static u8 RegionProcess_GMM_noLearn_CFix(TPixelLBPModel *ptLBP, u8 *pu8Hist, l32 l32Offset, f32 f32LearnRate, f32 f32Tp, u8 u8Label)
{
    l32 l32Index;
    TPixelLBPModel *ptMatch = NULL;
    TPixelLBPModel *ptCurLBP = ptLBP + LBP_MAX_MODELS * l32Offset; //��ǰ���ص�LBP
    f32 f32TotalWeight;
    u32 u32Dist, u32Tp;
    u32 u32Data0, u32Data1;
    u8 u8FG = 0;
    u8 * restrict pu8HistA = pu8Hist;

    /*
    //��̬Ŀ�괦����أ�Ŀǰ�Ȳ�֧�֣����ǽӿڱ���
    if(u8Label == 1)
    {
        f32LearnRate = 0;
    }
    if(u8Label == 2)
    {    
        for(l32Index = 0; l32Index < HIST_BINS; l32Index++) 
            ptCurLBP->au32Hist[l32Index] = pu8Hist[l32Index] << HIST_FIX_BIT_CNT;
        return 0;
    }
    */

    // see if the current pixel matches any of the models
    u32Tp = (u32)(f32Tp + 0.499f);
    
    //���մӴ�С��������
    int compid;
    float v0, v1, v2;
    v0 = ptCurLBP[0].f32Weight;
    v1 = ptCurLBP[1].f32Weight;
    v2 = ptCurLBP[2].f32Weight;
    compid = (v0 > v1) + (v1 > v2)*2 + (v2 > v0)*4;
    
    f32TotalWeight = 0.0f;
    for(l32Index = 0; l32Index < 3; l32Index++)
    {
        ptMatch = ptCurLBP + CompareTable[compid][l32Index];
        //64Bin
        u8 * restrict pu8HistB = (u8*)ptMatch->au32Hist;
        u32Dist = 0;
	    for(int i = 0; i < HIST_BINS; i++)
	    {
            u32Data0 = pu8HistA[i];
            u32Data1 = pu8HistB[i];
			if(u32Data0 > u32Data1) u32Data0 = u32Data1;
			u32Dist += u32Data0;
	    }
        if(u32Dist > u32Tp) break;
        f32TotalWeight += ptMatch->f32Weight;
    }
    
    if(l32Index < 3)
    {
        //�ҵ�ƥ��
        if(f32TotalWeight > LBP_TB) u8FG = 255;  //���ж�Ϊǰ��Component
        //��Ȩֵ����ѧϰ
        //ͳ�Ʊ����˷�֧��������У�Ϊ�������ٶȣ�ʹ�������Ȩ���޸Ĳ����Ա����һ��������
        float f1Nr = (1.0f - f32LearnRate);
        ptCurLBP[0].f32Weight *= f1Nr;
        ptCurLBP[1].f32Weight *= f1Nr;
        ptCurLBP[2].f32Weight *= f1Nr;
        ptMatch->f32Weight += f32LearnRate;
    }
    else
    {
        //δ�ҵ�ƥ�䣬�滻Ȩ����͵��Ǹ�ģ��
        u8FG = 255;
        ptMatch = ptCurLBP + CompareTable[compid][2];
        ptMatch->f32Weight = LBP_INIT_WEIGHT;

        u8 * restrict pu8HistB = (u8*)ptMatch->au32Hist;
        for(l32Index = 0; l32Index < HIST_BINS; l32Index++) pu8HistB[l32Index] = pu8HistA[l32Index];

        //��һ��Ȩ��
        f32TotalWeight = ptCurLBP[0].f32Weight + ptCurLBP[1].f32Weight + ptCurLBP[2].f32Weight;
        f32TotalWeight = _rcpsp(f32TotalWeight);
        ptCurLBP[0].f32Weight *= f32TotalWeight;
        ptCurLBP[1].f32Weight *= f32TotalWeight;
        ptCurLBP[2].f32Weight *= f32TotalWeight;
    }
    return u8FG;
}

//���㻯���޸�д��,DSPָ���Ż�������ģ�Ͷ�̬����
static u8 RegionProcess_GMM_DSP(TPixelLBPModel *ptLBP, u8 *pu8Hist, l32 l32Offset, f32 f32LearnRate, f32 f32Tp, u8 u8Label)
{
    l32 l32Index, l32M;
    TPixelLBPModel *ptMatch = NULL;
    TPixelLBPModel *ptCurLBP = ptLBP + LBP_MAX_MODELS * l32Offset; //��ǰ���ص�LBP
    f32 f32TotalWeight;
    u32 u32Dist, u32Tp, u32LearnRateN, u32LearnRateP;
    u32 u32Data0, u32Data1, u32Data2, u32Data3;
    l32 l32NzID, l32Pos;
    u8 u8FG = 0;
    
    /*
    //��̬Ŀ�괦����أ�Ŀǰ�Ȳ�֧�֣����ǽӿڱ���
    if(u8Label == 1)
    {
        f32LearnRate = 0;
    }
    if(u8Label == 2)
    {    
        for(l32Index = 0; l32Index < HIST_BINS; l32Index++) 
            ptCurLBP->au32Hist[l32Index] = pu8Hist[l32Index] << HIST_FIX_BIT_CNT;
        return 0;
    }
    */

    // see if the current pixel matches any of the models
    //
    //Ȩ�صĸ��㾫�Ȳ�һ��Ҫ�����ݵĸ��㾫��һ�£�Ϊ��ʹ�� DOTPRSU2 ָ���Ż�ѧϰ��
    //Ȩ��Ϊ16bit�����޷�����������ʹ��9bit����������
    u32LearnRateP = (u32)(f32LearnRate * (1<<16));
    u32LearnRateN = (1<<16) - u32LearnRateP;
    u32Tp = (u32)((f32Tp) * (1<<HIST_FIX_BIT_CNT));
    
    //���մӴ�С��������
    int compid;
    float v0, v1, v2;
    v0 = ptCurLBP[0].f32Weight;
    v1 = ptCurLBP[1].f32Weight;
    v2 = ptCurLBP[2].f32Weight;
    compid = (v0 > v1) + (v1 > v2)*2 + (v2 > v0)*4;
    
    //����ͳ�Ʒ���pu8HistA��Ϊ��Ԫ��5���ڷ���Ԫ�أ��������Ѱ�ҷ����Ԫ��
    //���γ�һ��bitmap��Ȼ�����������Щ���������Ԫ��
    u32 u32Bitmap0;
    u32 u32Bitmap1;
    u8 * restrict pu8HistA = pu8Hist;
    u8 * restrict pu8HistB = pu8Hist + 32;
    u8 * restrict pu8BitmapA =(u8*) &u32Bitmap0;
    u8 * restrict pu8BitmapB =(u8*) &u32Bitmap1;
    for(int i = 0; i < 32; i+=8)
    {
        u32Data0 = _amem4(pu8HistA + i);
        u32Data1 = _amem4(pu8HistA + i + 4);
        u32Data0 = _cmpgtu4(u32Data0, 0);
        u32Data1 = _cmpgtu4(u32Data1, 0);
        u32Data0 = (u32Data1 << 4) | u32Data0;
        
        u32Data2 = _amem4(pu8HistB + i);
        u32Data3 = _amem4(pu8HistB + i + 4);
        u32Data2 = _cmpgtu4(u32Data2, 0);
        u32Data3 = _cmpgtu4(u32Data3, 0);
        u32Data2 = (u32Data3 << 4) | u32Data2;
        
        *pu8BitmapA++ = u32Data0;
        *pu8BitmapB++ = u32Data2;
    }
    //��תbitmap������lmbdָ��ͻ᷵�ط���Ԫ�ص�ƫ����
    u32Bitmap0 = _bitr(u32Bitmap0);
    u32Bitmap1 = _bitr(u32Bitmap1);
    
    f32TotalWeight = 0.0f;
    for(l32Index = 0; l32Index < 3; l32Index++)
    {
        ptMatch = ptCurLBP + CompareTable[compid][l32Index];
        //u32Dist = CalcHistIntersectU32(pu8Hist, ptMatch->au32Hist, HIST_BINS);
        u16 * restrict pu16HistB = (u16 *)ptMatch->au32Hist;
        u32 u32Bmp0 = u32Bitmap0;
        u32 u32Bmp1 = u32Bitmap1;

        l32NzID = 0;
        u32Dist = 0;

        //����ͷ32�������еķ���Ԫ��
        u8 * restrict pu8HistTmp = pu8Hist;
        while(u32Bmp0)
        {
            l32Pos = _lmbd(1, u32Bmp0);
            u32Bmp0 = u32Bmp0 << (l32Pos);
            l32NzID += l32Pos;

            //����ǰλ�õ�����
            u32Data0 = pu8HistTmp[l32NzID] << HIST_FIX_BIT_CNT;
            u32Data1 = pu16HistB[l32NzID];
		    if(u32Data0 > u32Data1) u32Data0 = u32Data1;
		    u32Dist += u32Data0;

            u32Bmp0 <<= 1;
            l32NzID ++;
        }

        //�����32�������еķ���Ԫ��
        l32NzID = 32;
        while(u32Bmp1)
        {
            l32Pos = _lmbd(1, u32Bmp1);
            u32Bmp1 = u32Bmp1 << (l32Pos);
            l32NzID += l32Pos;

            //����ǰλ�õ�����
            u32Data0 = pu8HistTmp[l32NzID] << HIST_FIX_BIT_CNT;
            u32Data1 = pu16HistB[l32NzID];
		    if(u32Data0 > u32Data1) u32Data0 = u32Data1;
		    u32Dist += u32Data0;

            u32Bmp1 <<= 1;
            l32NzID ++;
        }

        /*
	    for(int i = 0; i < HIST_BINS; i++)
	    {
            u32Data0 = pu8HistA[i] << HIST_FIX_BIT_CNT;
            u32Data1 = pu32HistB[i];
            
			if(u32Data0 > u32Data1) u32Data0 = u32Data1;

			u32Dist += u32Data0;

            //l32Mask = ((l32)u32Data0 - (l32)u32Data1);
            //l32Mask = l32Mask >> 31;
		    //u32Dist += (u32Data0 & l32Mask) + (u32Data1 & (~l32Mask));
	    }
    	*/
        if(u32Dist > u32Tp) break;
        f32TotalWeight += ptMatch->f32Weight;
    }
    
    if(l32Index < 3)
    {
        //�ҵ�ƥ��
        if(f32TotalWeight > LBP_TB) u8FG = 255;  //���ж�Ϊǰ��Component

		u16 * restrict pu16HistB = (u16 *)ptMatch->au32Hist;
        u16 * restrict pu16HistDst = pu16HistB;
		u8 * restrict pu8HistA = pu8Hist;
        u32 u32LearnPair = _pack2(u32LearnRateN, u32LearnRateP);
        l32 l32Data0, l32Data1;
        for(l32M = 0; l32M < HIST_BINS; l32M+=2)
        {
            //ʹ��_dotprsu2ָ���������ļ���
			//pu32Hist[l32M] = (u32LearnRateN * pu32Hist[l32M] + u32LearnRateP * (pu8HistA[l32M] << HIST_FIX_BIT_CNT) + 0x8000) >> 16;
            //pu32Hist[l32M] = pu32Hist[l32M] << 1;
            u32Data0 = _mem2(pu8HistA + l32M);
            u32Data1 = _mem4(pu16HistB + l32M);
            u32Data0 = _unpklu4(u32Data0) << HIST_FIX_BIT_CNT;
            
            l32Data1 = _packh2(u32Data1, u32Data0);
            l32Data0 = _pack2(u32Data1, u32Data0);
            u32Data0 = _dotprsu2(l32Data0, u32LearnPair);
            u32Data1 = _dotprsu2(l32Data1, u32LearnPair);

            _mem4(pu16HistDst + l32M) = _pack2(u32Data1, u32Data0);
        }

        //ͳ�Ʊ����˷�֧��������У�Ϊ�������ٶȣ�ʹ�������Ȩ���޸Ĳ����Ա����һ��������
        float f1Nr = (1.0f - f32LearnRate);
        ptCurLBP[0].f32Weight *= f1Nr;
        ptCurLBP[1].f32Weight *= f1Nr;
        ptCurLBP[2].f32Weight *= f1Nr;
        ptMatch->f32Weight += f32LearnRate;
    }
    else
    {
        //δ�ҵ�ƥ�䣬�滻Ȩ����͵��Ǹ�ģ��
        u8FG = 255;
        ptMatch = ptCurLBP + CompareTable[compid][2];
        ptMatch->f32Weight = LBP_INIT_WEIGHT;

        u16 * restrict pu16HistB = (u16*)ptMatch->au32Hist;
        u8 * restrict pu8HistA = pu8Hist;
        for(l32M = 0; l32M < HIST_BINS; l32M+=2) 
        {
            u32Data0 = _mem2(pu8HistA + l32M);
            u32Data0 = _unpklu4(u32Data0) << HIST_FIX_BIT_CNT;
            _mem4(pu16HistB + l32M) = u32Data0;
        }

        //��һ��Ȩ��
        f32TotalWeight = ptCurLBP[0].f32Weight + ptCurLBP[1].f32Weight + ptCurLBP[2].f32Weight;
        f32TotalWeight = _rcpsp(f32TotalWeight);
        ptCurLBP[0].f32Weight *= f32TotalWeight;
        ptCurLBP[1].f32Weight *= f32TotalWeight;
        ptCurLBP[2].f32Weight *= f32TotalWeight;
    }
    return u8FG;
}

//���㻯���޸�д�����C�汾
static u8 RegionProcess_GMM_CFix(TPixelLBPModel *ptLBP, u8 *pu8Hist, l32 l32Offset, f32 f32LearnRate, f32 f32Tp, u8 u8Label)
{
    l32 l32Index, l32M;
    TPixelLBPModel *ptMatch = NULL;
    TPixelLBPModel *ptCurLBP = ptLBP + LBP_MAX_MODELS * l32Offset; //��ǰ���ص�LBP
    f32 f32TotalWeight;
    u32 u32Dist, u32Tp, u32LearnRateN, u32LearnRateP;
    u32 u32Data0, u32Data1;
    u8 u8FG = 0;
    
    if(u8Label == 1)
    {
        f32LearnRate = 0;
    }
    if(u8Label == 2)
    {    
        for(l32Index = 0; l32Index < HIST_BINS; l32Index++) 
            ptCurLBP->au32Hist[l32Index] = pu8Hist[l32Index] << HIST_FIX_BIT_CNT;
        return 0;
    }

    //Ȩ�صĸ��㾫�Ȳ�һ��Ҫ�����ݵĸ��㾫��һ�£�Ϊ��ʹ�� DOTPRSU2 ָ���Ż�ѧϰ��
    //Ȩ��Ϊ16bit�����޷�����������ʹ��9bit����������
    u32LearnRateP = (u32)(f32LearnRate * (1<<16));
    u32LearnRateN = (1<<16) - u32LearnRateP;
    u32Tp = (u32)((f32Tp) * (1<<HIST_FIX_BIT_CNT));
    
    //���մӴ�С��������
    int compid;
    float v0, v1, v2;
    v0 = ptCurLBP[0].f32Weight;
    v1 = ptCurLBP[1].f32Weight;
    v2 = ptCurLBP[2].f32Weight;
    compid = (v0 > v1) + (v1 > v2)*2 + (v2 > v0)*4;
    
    f32TotalWeight = 0.0f;
    for(l32Index = 0; l32Index < 3; l32Index++)
    {
        ptMatch = ptCurLBP + CompareTable[compid][l32Index];
        //u32Dist = CalcHistIntersectU32(pu8Hist, ptMatch->au32Hist, HIST_BINS);
        
        //64Bin
        u8 * restrict pu8HistA = pu8Hist;
        u32 * restrict pu32HistB = ptMatch->au32Hist;
        u32Dist = 0;
	    for(int i = 0; i < HIST_BINS; i++)
	    {
            u32Data0 = pu8HistA[i] << HIST_FIX_BIT_CNT;
            u32Data1 = pu32HistB[i];
			if(u32Data0 > u32Data1) u32Data0 = u32Data1;
			u32Dist += u32Data0;
	    }
        if(u32Dist > u32Tp) break;
        f32TotalWeight += ptMatch->f32Weight;
    }

    if(l32Index < 3)
    {
        //�ҵ�ƥ��
        if(f32TotalWeight > LBP_TB) u8FG = 255;  //���ж�Ϊǰ��Component

        // update the parameters for the matched model
		u32 * restrict pu32Hist = ptMatch->au32Hist;
		u8 * restrict pu8HistA = pu8Hist;
        for(l32M = 0; l32M < HIST_BINS; l32M++)
        {
			//pu32Hist[l32M] = ((u32LearnRateN * pu32Hist[l32M]) >> HIST_FIX_BIT_CNT) + (u32LearnRateP * pu8HistA[l32M]);
            pu32Hist[l32M] = (u32LearnRateN * pu32Hist[l32M] + u32LearnRateP * (pu8HistA[l32M] << HIST_FIX_BIT_CNT) + 0x8000) >> 16;
        }

        //ͳ�Ʊ����˷�֧��������У�Ϊ�������ٶȣ�ʹ�������Ȩ���޸Ĳ����Ա����һ��������
        float f1Nr = (1.0f - f32LearnRate);
        ptCurLBP[0].f32Weight *= f1Nr;
        ptCurLBP[1].f32Weight *= f1Nr;
        ptCurLBP[2].f32Weight *= f1Nr;
        ptMatch->f32Weight += f32LearnRate;
    }
    else
    {
        //δ�ҵ�ƥ�䣬�滻Ȩ����͵��Ǹ�ģ��
        u8FG = 255;
        ptMatch = ptCurLBP + CompareTable[compid][2];
        ptMatch->f32Weight = LBP_INIT_WEIGHT;
        for(l32Index = 0; l32Index < HIST_BINS; l32Index++)
            ptMatch->au32Hist[l32Index] = pu8Hist[l32Index] << HIST_FIX_BIT_CNT;

        //��һ��Ȩ��
        f32TotalWeight = ptCurLBP[0].f32Weight + ptCurLBP[1].f32Weight + ptCurLBP[2].f32Weight;
        f32TotalWeight = _rcpsp(f32TotalWeight);
        ptCurLBP[0].f32Weight *= f32TotalWeight;
        ptCurLBP[1].f32Weight *= f32TotalWeight;
        ptCurLBP[2].f32Weight *= f32TotalWeight;
    }
    return u8FG;
}

//����汾
static u8 RegionProcess_GMM_C(TPixelLBPModel *ptLBP, u8 *pu8Hist, l32 l32Offset, f32 f32LearnRate, f32 f32Tp, u8 u8Label)
{
    l32 l32Index, l32M;
    TPixelLBPModel tTmp, *ptMatch = NULL;
    TPixelLBPModel *ptCurLBP = ptLBP + LBP_MAX_MODELS * l32Offset, *ptComponent = NULL; //��ǰ���ص�LBP
    f32 f32Weight, f32TotalWeight = 0.0f, f32Tmp;
    u32 u32Dist, u32Tp, u32LearnRateN, u32LearnRateP;
    u8 u8FG = 0;
    
    if(u8Label == 1)
    {
        f32LearnRate = 0;
    }
    if(u8Label == 2)
    {    
        //memcpy(ptCurLBP->au32Hist, pu8Hist, l32HistDim * sizeof(f32));
        for(l32Index = 0; l32Index < HIST_BINS; l32Index++)
        {
            ptCurLBP->au32Hist[l32Index] = pu8Hist[l32Index] << HIST_FIX_BIT_CNT;
        }
        return 0;
    }

    // see if the current pixel matches any of the models
    f32Tmp = 1.0f - f32LearnRate;
    u32LearnRateP = (u32)(f32LearnRate * (1<<HIST_FIX_BIT_CNT));
    u32LearnRateN = (1<<HIST_FIX_BIT_CNT) - u32LearnRateP;
    u32Tp = (u32)((f32Tp) * (1<<HIST_FIX_BIT_CNT));
    
    //u32Tp = f32Tp - 12.5f ;//* (1<<HIST_FIX_BIT_CNT);

    for(l32Index = 0; l32Index < LBP_MAX_MODELS; l32Index++)
    {
        ptComponent = ptCurLBP + l32Index;
        f32Weight = f32Tmp * ptComponent->f32Weight;
        if(NULL == ptMatch)
        {
            //f32Dist = CalcHistIntersect(pu8Hist, ptComponent->af32Hist, HIST_BINS);
            u32Dist = CalcHistIntersectU32(pu8Hist, ptComponent->au32Hist, HIST_BINS);
            //u32Dist = CalcHistIntersectU8(pu8Hist, ptComponent->au32Hist, HIST_BINS);
            if(u32Dist > u32Tp)
            {
                if(f32TotalWeight > LBP_TB * f32Tmp)  //���ж�Ϊǰ��Component
                {
                    u8FG = 255;
                }

                ptMatch = ptComponent;
                f32Weight += f32LearnRate;  // this is the matched model, so M = 1

                // update the parameters for the matched model
                for(l32M = 0; l32M < HIST_BINS; l32M++)
                {
                    //ptMatch->af32Hist[l32M] = f32Tmp * ptMatch->af32Hist[l32M] + f32LearnRate * pu8Hist[l32M];
                    ptMatch->au32Hist[l32M] = 
                        ((u32LearnRateN * ptMatch->au32Hist[l32M]) >> HIST_FIX_BIT_CNT) + (u32LearnRateP * pu8Hist[l32M]);
                }

                //��Ȩ������
                for(l32M = l32Index; l32M > 0; l32M--)
                {
                    if(f32Weight < ptCurLBP[l32M - 1].f32Weight)
                    {
                        break;
                    }
                    else
                    {
                        //����
                        tTmp = ptCurLBP[l32M];
                        ptCurLBP[l32M] = ptCurLBP[l32M - 1];
                        ptCurLBP[l32M - 1] = tTmp;
                        ptComponent--;
                    }
                }
            }
        }

        f32TotalWeight += f32Weight;
        ptComponent->f32Weight = f32Weight;
    }

    // we didn't match anything 
    if(NULL == ptMatch)
    {
        u8FG = 255;
        ptComponent = ptCurLBP + LBP_MAX_MODELS - 1;

        //init a new model
        f32 f32TmpW = LBP_INIT_WEIGHT * f32Tmp;
        f32TotalWeight = f32TotalWeight - ptComponent->f32Weight + f32TmpW;
        ptComponent->f32Weight = f32TmpW;
        //memcpy(ptComponent->af32Hist, pf32Hist, l32HistDim * sizeof(f32));
        for(l32Index = 0; l32Index < HIST_BINS; l32Index++)
        {
            ptComponent->au32Hist[l32Index] = pu8Hist[l32Index] << HIST_FIX_BIT_CNT;
            //ptComponent->af32Hist[l32Index] = pu8Hist[l32Index];
        }
        for(l32M = LBP_MAX_MODELS - 1; l32M > 0; l32M--)
        {
            if(f32TmpW < ptCurLBP[l32M - 1].f32Weight)
            {
                break;
            }
            else
            {
                //����
                tTmp = ptCurLBP[l32M];
                ptCurLBP[l32M] = ptCurLBP[l32M - 1];
                ptCurLBP[l32M - 1] = tTmp;
            }
        }
    }

    //��һ��Ȩ��
    ptComponent = ptCurLBP;
    for(l32Index = 0; l32Index < LBP_MAX_MODELS; l32Index++,ptComponent++)
    {
        ptComponent->f32Weight /= f32TotalWeight;
    }

    return u8FG;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//���iLBP(p=6,r=2�����Ż�)
//���ڵ�6���������
//  (+/-)2,   0
//  (+/-)1    (+/-)1.732051
//
//Ϊ���Ż�������ʹ������������滻
//  (+/-)2,   0
//  (+/-)1    (+/-)1.5
//
// pu8Temp4LineBuffΪ4�еĴ�ֱ���壬�ֱ𱣴� -1.5��-0.5��+0.5��+1.5�еĴ�ֱ��ֵ���
// �����������Ҹ���2��/�в��ܸ������
//
//  ����3��ֵ����֮�󣬾Ϳ�������2��P6R2��LBP��
//  �ձ�Ľ�����(y,y+1)��ֵ���֮�󣬿��Խ��е�y-1�е�LBP����
//  ������                      ������
//   y-3                       ( - 1.5��)
//   y-2                       ( - 0.5��)
//   y-1 --------------------- ( + 0.5��) -------------��ǰ��
//   y                         ( + 1.5��)
//


//pu8TempBuffLBP�� 4*l32Width �Ķ��뻺��
static void iLBPR_p6r2_C(u8 *pu8Img, u8 *pu8Texture, l32 l32Width, l32 l32Height, u8 * pu8TempBuffLBP)
{
    u8 * restrict pu8Temp4LineBuff = pu8TempBuffLBP;
    u8 * pu8Src = pu8Img;
    u8 * pu8Tex = pu8Texture;
    int x, y;
    memset(pu8Tex, 0, l32Width * 2);    //ͷ2�е�LBP����޷��������㶨Ϊ��
    pu8Tex += l32Width * 2;
    for(y=0; y<l32Height-1; y++)
    {
        //��ֱ��ֵһ��.ע��y<l32Height-1����˲���ʹ�õ��Ƿ���y+1����������
        u8 * pu8Dst = pu8Temp4LineBuff + (y & 3) * l32Width;
        for(x=0; x<l32Width; x++) pu8Dst[x] = (pu8Src[x] + pu8Src[x + l32Width] + 1) >> 1;
        if(y >= 3)
        {
            //�������ע��
            u8 * restrict pu8SrcCur = pu8Src - l32Width;
            u8 * restrict pu8SrcTop = pu8Temp4LineBuff + ((y-3) & 3) * l32Width;
            u8 * restrict pu8SrcBot = pu8Temp4LineBuff + ((y) & 3) * l32Width;
            u8 * restrict pu8Dst = pu8Tex;
            
            pu8Dst[0] = pu8Dst[1] = 0;
            for(x=2;x<l32Width-2;x++)
            {
                u8 u8Center = pu8SrcCur[x];
                u8 u8Pt0 = pu8SrcCur[x-2];
                u8 u8Pt1 = pu8SrcTop[x-1];
                u8 u8Pt2 = pu8SrcTop[x+1];
                u8 u8Pt3 = pu8SrcCur[x+2];
                u8 u8Pt4 = pu8SrcBot[x+1];
                u8 u8Pt5 = pu8SrcBot[x-1];
                
                if(u8Center + LBP_HYSTERSIS > 255) u8Center = 255;
                else u8Center = u8Center + LBP_HYSTERSIS;

                pu8Dst[x] = ((u8Pt5 > u8Center)?0x01:0x00) |
                            ((u8Pt3 > u8Center)?0x02:0x00) |
                            ((u8Pt2 > u8Center)?0x04:0x00) |
                            ((u8Pt1 > u8Center)?0x08:0x00) |
                            ((u8Pt4 > u8Center)?0x10:0x00) |
                            ((u8Pt0 > u8Center)?0x20:0x00);
            }
            pu8Dst[x] = pu8Dst[x+1] = 0;
            
            pu8Tex += l32Width;
        }
        pu8Src += l32Width;
    }
    memset(pu8Tex, 0, l32Width * 2);    //���2�е�LBP����޷��������㶨Ϊ��
}

//pu8TempBuffLBP�� 4*l32Width �Ķ��뻺��
static void iLBPR_p6r2_DSP(u8 *pu8Img, u8 *pu8Texture, l32 l32Width, l32 l32Height, u8 * pu8TempBuffLBP)
{
    u8 * restrict pu8Temp4LineBuff = pu8TempBuffLBP;
    u8 * restrict pu8Src = pu8Img;
    u8 * restrict pu8Tex = pu8Texture;
    u8 * restrict pu8Src2;
    u8 * restrict pu8Dst;

    u32 u32Data0, u32Data1, u32Data2, u32Data3;
    u32 u32Data4, u32Data5, u32Data6, u32Data7;
    u32 u32Center, u32ConstHYS;

    int x, y;

    u32ConstHYS = _pack2(LBP_HYSTERSIS, LBP_HYSTERSIS);
    u32ConstHYS = _packl4(u32ConstHYS, u32ConstHYS);

    memset(pu8Tex, 0, l32Width * 2);    //ͷ2�е�LBP����޷��������㶨Ϊ��
    pu8Tex += l32Width * 2;
    for(y=0; y<l32Height-1; y++)
    {
        //��ֱ��ֵһ��.ע��y<l32Height-1����˲���ʹ�õ��Ƿ���y+1����������
        pu8Dst = pu8Temp4LineBuff + (y & 3) * l32Width;
        pu8Src2 = pu8Src + l32Width;
        for(x=0; x<l32Width; x += 4) 
        {
            //pu8Dst[x] = (pu8Src[x] + pu8Src[x + l32Width] + 1) >> 1;
            u32Data0 = _amem4(pu8Src + x);
            u32Data1 = _amem4(pu8Src2 + x);
            u32Data0 = _avgu4(u32Data0, u32Data1);
            _amem4(pu8Dst + x) = u32Data0;
        }
        if(y >= 3)
        {
            u8 * restrict pu8SrcCur = pu8Src - l32Width;
            u8 * restrict pu8SrcTop = pu8Temp4LineBuff + ((y-3) & 3) * l32Width;
            u8 * restrict pu8SrcBot = pu8Temp4LineBuff + ((y) & 3) * l32Width;
            //6���������Ƚϣ����ݱȽϽ���ۼӶ�Ӧbit
            pu8Dst = pu8Tex;
            for(x=0;x<l32Width;x+=4)
            {
                u32Center = _amem4(pu8SrcCur + x);
                u32Data0 = _mem4(pu8SrcCur + x-2);
                u32Data1 = _mem4(pu8SrcTop + x-1);
                u32Data2 = _mem4(pu8SrcTop + x+1);
                u32Data3 = _mem4(pu8SrcCur + x+2);
                u32Data4 = _mem4(pu8SrcBot + x+1);
                u32Data5 = _mem4(pu8SrcBot + x-1);
                
                u32Center = _saddu4(u32Center, u32ConstHYS);

                u32Data0 = _cmpgtu4(u32Data0, u32Center);
                u32Data1 = _cmpgtu4(u32Data1, u32Center);
                u32Data2 = _cmpgtu4(u32Data2, u32Center);
                u32Data3 = _cmpgtu4(u32Data3, u32Center);
                u32Data4 = _cmpgtu4(u32Data4, u32Center);
                u32Data5 = _cmpgtu4(u32Data5, u32Center);
                

                u32Data0 |= u32Data1 << 4;  //10
                u32Data2 |= u32Data3 << 4;  //32
                u32Data4 |= u32Data5 << 4;  //54
                
                //   XX 32 XX 10  (X����4bit)
                u32Data0 = _pack2(u32Data2, u32Data0);
                //   XX ZZ XX 54
                u32Data4 = _pack2(0,        u32Data4); 
                //   ZZ 54 32 10
                u32Data0 = _packl4(u32Data4, u32Data0);
                
                //u32Data0 :  ---a7---a6---a5---a4|---a3---a2---a1---a0  
                //              (---����dcb����3��ˮƽ���ڵ�.a6,a7����0)
                u32Data0 = _shfl(u32Data0);
                //u32Data0 :  ------a7a3------a6a2|------a5a1------a4a0
                u32Data0 = _shfl(u32Data0);
                //u32Data0 :  ------------a7a5a3a1|------------a6a4a2a0
                u32Data0 = _shfl(u32Data0);
                //u32Data0 :  -------------------------a7a5a3a1a6a4a2a0
                _amem4(pu8Dst + x) = u32Data0;
                /*
                pu8Dst[x] = ((u8Pt0 >= u8Center)?0x01:0x00) |
                            ((u8Pt1 >= u8Center)?0x02:0x00) |
                            ((u8Pt2 >= u8Center)?0x04:0x00) |
                            ((u8Pt3 >= u8Center)?0x08:0x00) |
                            ((u8Pt4 >= u8Center)?0x10:0x00) |
                            ((u8Pt5 >= u8Center)?0x20:0x00);
                */
            }
            pu8Dst[0] = pu8Dst[1] = 0;
            pu8Dst[l32Width-1] = pu8Dst[l32Width-2] = 0;
            
            pu8Tex += l32Width;
        }
        pu8Src += l32Width;
    }
    memset(pu8Tex, 0, l32Width * 2);    //���2�е�LBP����޷��������㶨Ϊ��
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//LBPģ����Դ����


l32 LBPModelOpen(void **pvvLBPHandle, l32 l32Width, l32 l32Height)
{
	l32 l32Index;
	if(NULL == pvvLBPHandle)
	{
		return FALSE;
	}

	TLBPModel *ptLBPModel = (TLBPModel *)MALLOC(sizeof(TLBPModel));
    memset(ptLBPModel, 0, sizeof(TLBPModel));
	ptLBPModel->l32Width = l32Width;
	ptLBPModel->l32Height = l32Height;
	ptLBPModel->pu8Texture = (u8 *)MALLOC(l32Width * l32Height);
    memset(ptLBPModel->pu8Texture, 0, l32Width * l32Height);

    //����ռ�
	ptLBPModel->l32BlockW = (ptLBPModel->l32Width - BLOCK_WIDTH) / 2 + 1;
	ptLBPModel->l32BlockH = (ptLBPModel->l32Height - BLOCK_HEIGHT) / 2 + 1;

    l32 l32BlockCnt = ptLBPModel->l32BlockW * ptLBPModel->l32BlockH;
	ptLBPModel->ptLBP = (TPixelLBPModel *)MALLOC(l32BlockCnt * LBP_MAX_MODELS * sizeof(TPixelLBPModel));  //LBPģ�Ͳ���
	memset(ptLBPModel->ptLBP, 0, l32BlockCnt * LBP_MAX_MODELS * sizeof(TPixelLBPModel));

	for(l32Index = 0; l32Index < l32BlockCnt * LBP_MAX_MODELS; l32Index++)
	{
		ptLBPModel->ptLBP[l32Index].f32Weight = 0;
	}
    ptLBPModel->pu8Hist = (u8 *)MALLOC(HIST_BINS * sizeof(u8));

    //����ռ�
    ptLBPModel->apu8LBPFG[0] = (u8 *)MALLOC(2 * l32Width * l32Height);
    memset(ptLBPModel->apu8LBPFG[0], 255, 2 * l32Width * l32Height);
    ptLBPModel->apu8LBPFG[1] = ptLBPModel->apu8LBPFG[0] + l32Width * l32Height;

    ptLBPModel->pl32LBPNoisy = (l32 *)MALLOC(l32Width * l32Height * sizeof(l32));
    memset(ptLBPModel->pl32LBPNoisy, 0, l32Width * l32Height * sizeof(l32));

    ptLBPModel->pu8IsNosiy = (u8 *)MALLOC(l32Width * l32Height);
    memset(ptLBPModel->pu8IsNosiy, 0, l32Width * l32Height);
    ptLBPModel->l32NoisySumFrames = 0;  //��ǰͳ����������֡��

    //��ʱ����
    ptLBPModel->pu8TempBuff = (u8 *)MALLOC(6 * l32Width);

	*pvvLBPHandle = ptLBPModel;

	return TRUE;
}


//////////////////////////////////////////////////////////////////////
//����P6R2��LBP�Ż�
#define iLBPR_p6r2 iLBPR_p6r2_DSP
//#define iLBPR_p6r2 iLBPR_p6r2_C

//////////////////////////////////////////////////////////////////////

//��ģ�͸��µİ汾--�����Դ󣬼��Ч������
//#define RegionProcess_GMM RegionProcess_GMM_noLearn_DSP    
//#define RegionProcess_GMM RegionProcess_GMM_noLearn_CFix

//�޸�д���Ͷ��㻯���C�汾
#define RegionProcess_GMM RegionProcess_GMM_DSP
//#define RegionProcess_GMM RegionProcess_GMM_CFix

//ԭʼC�汾
//#define RegionProcess_GMM RegionProcess_GMM_C


l32 LBPModelProcess(void *pvLBPHandle, u8 *pu8Img, u8 *pu8FG)
{
    l32 l32Index, l32Row, l32Col;
	if(NULL == pvLBPHandle /*|| NULL == pu8Img || NULL == pu8FG || NULL == pf32LBPTp*/)
	{
		return FALSE;
	}

	TLBPModel *ptLBPModel = (TLBPModel *)pvLBPHandle;

	l32 l32BlockW = ptLBPModel->l32BlockW;
	l32 l32BlockH = ptLBPModel->l32BlockH;

    iLBPR_p6r2(pu8Img, ptLBPModel->pu8Texture, ptLBPModel->l32Width, ptLBPModel->l32Height, ptLBPModel->pu8TempBuff);

    //printf("\n");
    //for(int i=0;i<2;i++) printf("%I64d,", gst[i]);
    //printf("\n");

    //copy texture
    //memcpy(g_pu8Texure, ptLBPModel->pu8Texture, ptLBPModel->l32Width * ptLBPModel->l32Height);
// 	Mat mLbp(Size(ptLBPModel->l32Width, ptLBPModel->l32Height), CV_8UC1, ptLBPModel->pu8Texture);
// 	imshow("mLbp", mLbp);

	//�ܹ�Ҫ�����Block�ܸ���
    l32 l32Off = 0;

    //�Ƿ�ʼͳ������
    l32 l32IsNosiySum = ptLBPModel->l32NoisySumFrames < LBP_NOISYSUM_FRAMES;  //To delete
    f32 f32LearnRate = 1.0f / MIN(2 * (ptLBPModel->l32Frames + 1), LBP_HISTORY);

    u8 *pu8OrgFG    = ptLBPModel->apu8LBPFG[ptLBPModel->l32Frames & 1];
    u8 *pu8PreOrgFG = ptLBPModel->apu8LBPFG[!(ptLBPModel->l32Frames & 1)];
    
    memset(pu8FG, 0, ptLBPModel->l32Width * ptLBPModel->l32Height);

    u8 *pu8TexTmp = ptLBPModel->pu8Texture;
    l32 l32RowStride = ptLBPModel->l32Width * 2;
    for(l32Row = 0; l32Row < l32BlockH; l32Row++, pu8TexTmp += l32RowStride)
    {
        //ÿ�е�һ����
        u8 * restrict pu8Src = pu8TexTmp;
		u8 * restrict pu8SrcB;
        u8 * restrict pu8DstHist = ptLBPModel->pu8Hist;
		u8 * restrict pu8DstHistB = pu8DstHist;
        memset(pu8DstHist, 0, HIST_BINS);
        for(int y=0;y<BLOCK_HEIGHT;y++)
        {
			pu8SrcB = pu8Src + 1;
            for(int x=0;x<BLOCK_WIDTH;x+=2) 
			{
				pu8DstHist[pu8Src[x]]++;
				pu8DstHistB[pu8SrcB[x]]++;
			}
            pu8Src += ptLBPModel->l32Width;
        }
        
        int off_start = l32Row * l32RowStride;
        for(l32Col = 0; l32Col < l32BlockW; l32Col++, l32Off++, off_start += 2)
        {
            //������LBP����ģ��
            u8 u8Fg = RegionProcess_GMM(ptLBPModel->ptLBP, 
                                        ptLBPModel->pu8Hist, 
                                        l32Off, f32LearnRate,
                                        (ptLBPModel->pu8IsNosiy[l32Off]? LBP_TP_LOW : LBP_TP_HI) * (BLOCK_WIDTH * BLOCK_HEIGHT),
                                        0);
            
            pu8OrgFG[l32Off] = u8Fg;
            //��������
            ptLBPModel->pl32LBPNoisy[l32Off] += (pu8OrgFG[l32Off] != pu8PreOrgFG[l32Off]);

            if(u8Fg)
            {
                u8 * restrict pu8Src = pu8FG + off_start;
                u8 * restrict pu8Dst = pu8FG + off_start;
                u32 u32Data0, u32Data1;
                for(int i=0;i<BLOCK_HEIGHT;i++)
                {
                    //�Ե�ǰ��λ��8�����ǰ����������1
                    //for(int j=0;j<BLOCK_WIDTH;j++) pu8FG[off + j] ++;
                    u32Data0 = _mem4(pu8Src);
                    u32Data1 = _mem4(pu8Src + 4);
                    u32Data0 = _add4(u32Data0, 0x01010101);
                    u32Data1 = _add4(u32Data1, 0x01010101);
                    _mem4(pu8Dst) = u32Data0;
                    _mem4(pu8Dst + 4) = u32Data1;

                    pu8Dst += ptLBPModel->l32Width;
                    pu8Src += ptLBPModel->l32Width;
                }
            }

            //����������һ�����Histogram
            if(l32Col < l32BlockW - 1)
            {
                u8 * restrict pu8Src = pu8TexTmp + l32Col * 2;
				u8 * restrict pu8SrcB = pu8Src + BLOCK_WIDTH;
				u8 * restrict pu32DstHistB = pu8DstHist;
                u32 u32Data0, u32Data1;
                for(int y=0;y<BLOCK_HEIGHT;y++)
                {
                    //pu8DstHist[pu8Src[0]] --;
                    //pu8DstHist[pu8Src[1]] --;
                    //pu8DstHist[pu8Src[BLOCK_WIDTH]] ++;
                    //pu8DstHist[pu8Src[BLOCK_WIDTH+1]] ++;

                    u32Data0 = _mem4(pu8Src);
                    u32Data1 = _mem4(pu8SrcB);
                    
                    pu8DstHist[u32Data0 & 0xFF]--;
                    pu8DstHist[_extu(u32Data0, 16, 24)]--;

                    pu32DstHistB[u32Data1 & 0xFF]++;
                    pu32DstHistB[_extu(u32Data1, 16, 24)]++;

                    pu8Src += ptLBPModel->l32Width;
					pu8SrcB+= ptLBPModel->l32Width;
                }
            }
        }
    }
    ptLBPModel->l32NoisySumFrames++;

    //ͳ�������ӣ���ҪӰ��δ���������ӱ�����ģ����ֵ����
    if(ptLBPModel->l32NoisySumFrames == LBP_NOISYSUM_FRAMES || ptLBPModel->l32Frames < LBP_NOISYSUM_FRAMES)  //25*60*2
    {
        for(l32Index = 0; l32Index < l32BlockW * l32BlockH; l32Index++)
        {
            ptLBPModel->pu8IsNosiy[l32Index] = FALSE;
            if(25 * ptLBPModel->pl32LBPNoisy[l32Index] > ptLBPModel->l32NoisySumFrames)
            {
                ptLBPModel->pu8IsNosiy[l32Index] = TRUE;  //�ж�Ϊ��������
            }
        }
        //��ʼ��Ϊ0
        if(ptLBPModel->l32Frames >= LBP_NOISYSUM_FRAMES)
        {
            memset(ptLBPModel->pl32LBPNoisy, 0, l32BlockW * l32BlockH * sizeof(l32));
            ptLBPModel->l32NoisySumFrames = 0;
        }
    }
    ptLBPModel->l32Frames++;
    
    //��ǰ������ͼ����к�����
    if(1)
    {
        u32 u32Data0;
        u8 * restrict pu8Src = pu8FG;
        u8 * restrict pu8Dst = pu8FG;
        for(l32Index = 0; l32Index < ptLBPModel->l32Width * ptLBPModel->l32Height; l32Index+=4)
        {
            //pu8FG[l32Index] = pu8FG[l32Index] > 3?255:0;
            u32Data0 = _amem4(pu8Src + l32Index);
            u32Data0 = _cmpgtu4(u32Data0, 0x03030303);
            u32Data0 = _xpnd4(u32Data0);
            _amem4(pu8Dst + l32Index) = u32Data0;
        }
    }
	return TRUE;
}

//LBP����ģ�ͣ��ͷſռ�
l32 LBPModelClose(void *pvLBPHandle)
{
	if(NULL == pvLBPHandle)
	{
		return FALSE;
	}

	TLBPModel *ptLBPModel = (TLBPModel *)pvLBPHandle;
	if(ptLBPModel->ptLBP)
	{
		FREE(ptLBPModel->ptLBP);
		ptLBPModel->ptLBP = NULL;
	}

	if(ptLBPModel->pu8Texture)
	{
		FREE(ptLBPModel->pu8Texture);
		ptLBPModel->pu8Texture = NULL;
	}

    if(ptLBPModel->pu8Hist)
    {
        FREE(ptLBPModel->pu8Hist);
        ptLBPModel->pu8Hist = NULL;
    }

    if(ptLBPModel->apu8LBPFG[0])
    {
        FREE(ptLBPModel->apu8LBPFG[0]);
        ptLBPModel->apu8LBPFG[0] = NULL;
    }

    if(ptLBPModel->pl32LBPNoisy)
    {
        FREE(ptLBPModel->pl32LBPNoisy);
        ptLBPModel->pl32LBPNoisy = NULL;
    }

    if(ptLBPModel->pu8IsNosiy)
    {
        FREE(ptLBPModel->pu8IsNosiy);
        ptLBPModel->pu8IsNosiy = NULL;
    }
    if(ptLBPModel->pu8TempBuff)
    {
        FREE(ptLBPModel->pu8TempBuff);
        ptLBPModel->pu8TempBuff = NULL;
    }

    FREE(ptLBPModel);
	ptLBPModel = NULL;
	return TRUE;
}
