#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "opt_inc.h"

#include "ext_blob.h"
#include <string.h>


/*=============================================================================
������    ��CAntImage::CAntImage()
����      ����CAntImage�Ĺ��캯��
�㷨ʵ��  ����
����˵��  ��vw  
vh  
����ֵ˵������
����˵��  ����
=============================================================================*/
CAntImage::CAntImage(int vw, int vh):m_width(vw),m_height(vh),m_nChannels(3)
{
    int i;
    //allocates memory for accompanying images
    m_pTempf1 = new int[vw*vh];

    m_pTempMask = new unsigned char[vw*vh];
    memset(m_pTempMask, 0, vw * vh*sizeof(unsigned char));

    for(i=0;i<MAX_NUM_BLOBS;i++)
    {
        m_blobs[i] = new AntBlob();
    }

    m_nBlobs = 0;
    m_minTargetSizeX = 3; //4 by 4 pixels
    m_minTargetSizeY = 3; //4 by 4 pixels
    m_x_dilate_size = 8; //size of x direction dilation
    m_y_dilate_size = 10; //size of y direction dilation
}

/*=============================================================================
������    ��CAntImage::~CAntImage()
����      ����CAntImage����������
�㷨ʵ��  ����
����˵��  ����
����ֵ˵������
����˵��  ����
=============================================================================*/
CAntImage::~CAntImage()
{
    FreeAllMemory();
}

/*=============================================================================
������    ��CAntImage::FreeAllMemory()
����      ���ͷ����е��ڴ�
�㷨ʵ��  ����
����˵��  ����
����ֵ˵������
����˵��  ����
=============================================================================*/
void CAntImage::FreeAllMemory()
{
    //allocates memory for accompanying images
    delete [] m_pTempf1;

    delete [] m_pTempMask;
}

/*=============================================================================
������    ��CAntImage::setParameters()
����      ��������̬ѧ�����Ĳ���
�㷨ʵ��  ����
����˵��  ��targetSizeX  
targetSizeY  
xDilateSize  
yDilateSize  
����ֵ˵������
����˵��  ����
=============================================================================*/
void CAntImage::setParameters(int targetSizeX,int targetSizeY, int xDilateSize, int yDilateSize)
{
    m_minTargetSizeX = targetSizeX;
    m_minTargetSizeY = targetSizeY;
    m_x_dilate_size = xDilateSize;
    m_y_dilate_size = yDilateSize;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
#include "intrinsic.h"
#define restrict
#endif

/*=============================================================================
������    ��dilate_erode_HTrans2()
����      �����ͺ͸�ʴ
�㷨ʵ��  ����̬ѧ���ͺ͸�ʴ
����˵��  ��pu8Src
m_width
m_height
pu8Dst
halfsizeX
dilate_mask
erode_mask
����ֵ˵������
����˵��  ����
=============================================================================*/
void dilate_erode_HTrans2(u8 * restrict pu8Src, int m_width, int m_height,  
                          u8 * restrict pu8Dst, int halfsizeX, int dilate_mask, int erode_mask)
{
    int i, j, n, k, id;
    int k_last;
    int k_end;
    int i_meet0;
    int i_meetX;
    int u8Cur, u8Prev;
    s64 s64Search, s64Data1, s64Data2, s64Data3, s64Data4;
    u32 u32Dilate, u32Erode, u32Mask, u32Mask1, u32Mask2, u32Mask3, u32Mask4;
    l32 l32BitCount;
    u8 * restrict pu8DstCol;

    int iChangePos[704];
    int iChangeCnt =0;
    int *piChangePos;

    u32Dilate = dilate_mask | (dilate_mask << 8);
    u32Dilate = u32Dilate | (u32Dilate << 16);

    u32Erode = erode_mask | (erode_mask << 8);
    u32Erode = u32Erode | (u32Erode << 16);

    for (i=0; i<m_height; i++)
    {
        pu8DstCol = pu8Dst + i;
#if 1
        piChangePos = iChangePos;
        iChangeCnt = 0;
        u8Prev = erode_mask;    //��һ������λ������Ҫ���͵�mask
        for(j=0;j<m_width;j++)
        {
            u8Cur = pu8Src[j];
            if(u8Cur != u8Prev)
            {
                //��¼����λ��
                *piChangePos++ = j;

            }
            u8Prev = u8Cur;
        }
        *piChangePos =  m_width;//һ�н���λ����Ϊһ������
        iChangeCnt = piChangePos - iChangePos;

        k =0;
        for(id=0; id<iChangeCnt; id+=2)
        {
            //һ��ɨ�����ڷ�Ϊ�������֣��ֱ����»��ߺ�˫�»��߱�ʶ
            // x x x x x 0 0 0 0 0 ?
            // --------- =========
            //�ӷ����������ʱ�̣�����ȷ��֮ǰ�ķ������ݸ��ƶ���
            //�������������ʱ�̣�����ȷ�����������ٸ��㵽Ŀ��
            i_meet0 = iChangePos[id];
            i_meetX = iChangePos[id+1];

            //���ϴ�Ŀ��λ�ã���i_meet0-halfsizeXλ�ã�����ԭʼ����
            k_end = i_meet0 - halfsizeX;
            if(i_meet0 == i_meetX) k_end = i_meet0; //��������������β
            for(; k<k_end; k++) 
            {
                //pu8DstCol[0] = pu8Src[k];
                pu8DstCol[0] = erode_mask;//ֱ��ʹ��ǰ�����ص㼴�ɣ����ø���
                pu8DstCol += m_height;
            }
            //��������㵽i_meetX + halfsizeX
            k_end = i_meetX + halfsizeX;
            if(k_end > m_width) k_end = m_width;
            for(; k<k_end; k++) 
            {
                pu8DstCol[0] = dilate_mask;
                pu8DstCol += m_height;
            }
        }
        for(; k<m_width; k++) 
        {
            pu8DstCol[0] = erode_mask;
            pu8DstCol += m_height;
        }

#else
        k_last = 0;
        for(j=0;j<m_width;j++)
        {
            //һ��ɨ�����ڷ�Ϊ�������֣��ֱ����»��ߺ�˫�»��߱�ʶ
            // x x x x x 0 0 0 0 0 ?
            // --------- =========
            //�ӷ����������ʱ�̣�����ȷ��֮ǰ�ķ������ݸ��ƶ���
            //�������������ʱ�̣�����ȷ�����������ٸ��㵽Ŀ��

#if 1
            while(pu8Src[j] != dilate_mask && j<m_width) j++;
            i_meet0 = j;
            while(pu8Src[j] == dilate_mask && j<m_width) j++;
            i_meetX = j;
#else
            //ʹ��compeq�γɵ�λͼ�����lmbdλ���ָ�����Ѱ��Ŀ��
            s64Search = _itoll(u32Dilate, u32Dilate);
            l32BitCount = 32;
            while(l32BitCount == 32 && j<m_width)
            {
                s64Data1 = _mem8_const(pu8Src + j);
                s64Data2 = _mem8_const(pu8Src + j + 8);
                s64Data3 = _mem8_const(pu8Src + j + 16);
                s64Data4 = _mem8_const(pu8Src + j + 24);

                u32Mask1 = _dcmpeq4(s64Data1, s64Search);
                u32Mask2 = _dcmpeq4(s64Data2, s64Search);
                u32Mask3 = _dcmpeq4(s64Data3, s64Search);
                u32Mask4 = _dcmpeq4(s64Data4, s64Search);

                u32Mask1 = _pack2(u32Mask2, u32Mask1);
                u32Mask3 = _pack2(u32Mask4, u32Mask3);
                u32Mask3 = _packl4(u32Mask3, u32Mask1);
                u32Mask3 = _bitr(u32Mask3);
                l32BitCount = _lmbd(1, u32Mask3);
                j+= 32;
            }
            j = j - 32 + (l32BitCount);
            if(j>m_width) j=m_width;
            i_meet0 = j;

            s64Search = _itoll(u32Erode, u32Erode);
            l32BitCount = 32;
            while(l32BitCount == 32 && j<m_width)
            {
                s64Data1 = _mem8_const(pu8Src + j);
                s64Data2 = _mem8_const(pu8Src + j + 8);
                s64Data3 = _mem8_const(pu8Src + j + 16);
                s64Data4 = _mem8_const(pu8Src + j + 24);

                u32Mask1 = _dcmpeq4(s64Data1, s64Search);
                u32Mask2 = _dcmpeq4(s64Data2, s64Search);
                u32Mask3 = _dcmpeq4(s64Data3, s64Search);
                u32Mask4 = _dcmpeq4(s64Data4, s64Search);

                u32Mask1 = _pack2(u32Mask2, u32Mask1);
                u32Mask3 = _pack2(u32Mask4, u32Mask3);
                u32Mask3 = _packl4(u32Mask3, u32Mask1);
                u32Mask3 = _bitr(u32Mask3);
                l32BitCount = _lmbd(1, u32Mask3);
                j+= 32;
            }
            j = j - 32 + (l32BitCount);
            if(j>m_width) j=m_width;
            i_meetX = j;
#endif
            //���ϴ�Ŀ��λ�ã���i_meet0-halfsizeXλ�ã�����ԭʼ����
            k_end = i_meet0 - halfsizeX;
            if(i_meet0 == i_meetX) k_end = i_meet0; //��������������β
            for(k=k_last; k<k_end; k++) 
            {
                //pu8DstCol[0] = pu8Src[k];
                pu8DstCol[0] = erode_mask;//ֱ��ʹ��ǰ�����ص㼴�ɣ����ø���
                pu8DstCol += m_height;
            }
            //��������㵽i_meetX + halfsizeX
            k_end = i_meetX + halfsizeX;
            if(k_end > m_width) k_end = m_width;
            for(; k<k_end; k++) 
            {
                pu8DstCol[0] = dilate_mask;
                pu8DstCol += m_height;
            }
            k_last = k;
        }
#endif
        /*
        //�������Ϊ�˱��ָ�Դ�㷨һ�£������ѭ������ȥ��
        pu8DstCol = pu8Dst + i + (m_width-halfsizeX)*m_height;
        for(j=m_width-halfsizeX;j<m_width; j++)
        {
        pu8DstCol[0] = 0;
        pu8DstCol += m_height;
        }
        */
        pu8Src += m_width;
    }
}

/*=============================================================================
������    ��MophOpen_V1()
����      ����̬ѧ���������
�㷨ʵ��  ����̬ѧ������
����˵��  ��m_pFgMask
m_width
m_height
m_minTargetSizeX  
m_minTargetSizeY  
m_x_dilate_size   
m_y_dilate_size   
m_pTempMask       
����ֵ˵������
����˵��  ����
=============================================================================*/
void MophOpen_V1(u8 *m_pFgMask, int m_width, int m_height, 
                 int m_minTargetSizeX, int m_minTargetSizeY,
                 int m_x_dilate_size, int m_y_dilate_size,
                 u8 *m_pTempMask)
{
    int halfsizeX;
    int halfsizeY;
    if(m_minTargetSizeX > 0 && m_minTargetSizeY > 0) 
    {
        halfsizeX = m_minTargetSizeX/2;
        halfsizeY = m_minTargetSizeY/2;
        dilate_erode_HTrans2(m_pFgMask, m_width, m_height,  m_pTempMask, halfsizeX, 0, 0xFF);
        dilate_erode_HTrans2(m_pTempMask, m_height, m_width,m_pFgMask, halfsizeY, 0, 0xFF);
    }
    
    if(m_x_dilate_size > 0 && m_y_dilate_size > 0)
    {
        halfsizeX = m_x_dilate_size/2;
        halfsizeY = m_y_dilate_size/2;
        dilate_erode_HTrans2(m_pFgMask, m_width, m_height,  m_pTempMask, halfsizeX, 0xFF, 0);
        dilate_erode_HTrans2(m_pTempMask, m_height, m_width,m_pFgMask, halfsizeY,  0xFF, 0);
    }
}


/*=============================================================================
������    ��NSET_link()
����      ������һ������
�㷨ʵ��  ����
����˵��  ��pNSET
id1
id2
����ֵ˵������
����˵��  ��NSETÿ��Ԫ�����������ԣ�id(��������)��next_id(����ֵ)
=============================================================================*/

//����һ������
void NSET_link(u32 * pNSET, u32 id1, u32 id2)
{
    int prev_id, next_id;
    int nz_id = 0;
    int other_id;

    if(pNSET[id1]) 
    {
        nz_id = id1;
        other_id = id2;
    }
    else 
        if(pNSET[id2]) 
        {
            nz_id = id2;
            other_id = id1;
        }

        if(nz_id == 0)
        {
            //����id���ǿյģ��½�һ��nset
            pNSET[id1] = id2;
            pNSET[id2] = id1;
        }
        else
        {
            //�ӷǿյ�nset�б���Ѱ���Ƿ��Ѿ����ڸ�other_id������
            next_id = pNSET[nz_id];
            while(next_id != nz_id)
            {
                if(next_id == other_id) return;

                prev_id = next_id;
                next_id = pNSET[prev_id];
            }

            if(pNSET[other_id])
            {
                //other_id����һ���µ�nset,��������nset
                //����otherid�����nset��·
                pNSET[prev_id] = other_id;
                do
                {
                    prev_id = pNSET[prev_id];
                }while(pNSET[prev_id] != other_id);
                //�޸Ļ�·�����nz_id��
                pNSET[prev_id] = nz_id;
            }
            else
            {
                //other_id����һ������Ԫ��,
                next_id = pNSET[nz_id];
                pNSET[nz_id] = other_id;
                pNSET[other_id] = next_id;
            }
        }
}


/*
�Զ�ֵ�����ͼ�������ͨ�������
��������matlab��bwlabel
����ֱ�ʹ��1~N��ʶ�ڽ��������

pu8SrcBW (l32Width x l32Height) ��ֵ�����Դͼ��
u8Fg                            ��ֵԴͼ���ǰ��ɫ
pu8DstLB                        ��ͨ����궨֮��Ľ��idͼ����СҲ��(l32Width x l32Height)
��Դͼ������Ӧ��idΪ0��������Ϊ1~N������ͨ��ǰ��
����ֵN                         ��ͨǰ��������
*/

u32 au32PrevConnID[IMAGE_WIDTH];
u32 au32NSET[IMAGE_WIDTH*IMAGE_HEIGHT/8];

TRunLength atLineRL[IMAGE_WIDTH*IMAGE_HEIGHT/4]; //��������ͼ���RunLength��(����֮ǰ��̬ѧ��Ե�ʣ��˱�����̫��)

/*=============================================================================
������    ��findConnectedComponents_V1()
����      ���Զ�ֵ�����ͼ�������ͨ�����������������matlab��bwlabel������ֱ�ʹ��1~N��ʶ�ڽ��������
�㷨ʵ��  ����
����˵��  ��pu8SrcBW    ��ֵ�����Դͼ��
pu32DstLB   ��ͨ����궨֮��Ľ��idͼ����СҲ��(l32Width x l32Height)
l32Width    ͼ���
l32Height   ͼ���
l32Stride   ͼ�������ݲ���
u8Fg        ��ֵԴͼ���ǰ��ɫ
m_map      
����ֵ˵������
����˵��  ��
=============================================================================*/
int findConnectedComponents_V1(u8 *pu8SrcBW, u32 *pu32DstLB, 
                               l32 l32Width, l32 l32Height, l32 l32Stride, u8 u8Fg, int *m_map)
{
    u32 *pu32PrevNB = au32PrevConnID + 1;
    u32 * restrict pu32LBCur;
    int *pl32IDMap = m_map;

    l32 x, y, id, area_id, next_id, overlap, x_st;
    u8 * restrict pu8Src;
    l32 seg_st, seg_ed, seg_id, conn_id;
    l32 l32PrevNBCount, l32GroupID;

    TRunLength * ptRLCur;
    TRunLength * ptRLPrev;
    TRunLength * ptRLNextPrev;
    TRunLength * ptRLTemp;

    s64 s64Data1, s64Data2, s64Data3, s64Data4, s64Search;
    u32 u32Mask1, u32Mask2, u32Mask3, u32Mask4;
    l32 l32BitCount;

    memset(au32NSET, 0, sizeof(au32NSET));

    //�Զ�����ɨ�裬�ۺ�һ������ͨ���֣��Լ���ֱ�������·ֲ����ͨ����
    //Ϊ�˳�����ͨ����ʹ��area_id�궨�������ڴ�ֱ���Ϸֲ�������Ѿ��궨
    //��area����ͨ�����¼��au8Link�����У������ۺ�֮��.
    ptRLCur = atLineRL;
    pu8Src = pu8SrcBW;
    area_id = 0;
    ptRLNextPrev = NULL;
    for(y=0; y<l32Height; y++)
    {
        //��¼��һ��ѭ����Prev RunLength��Ϣ
        ptRLPrev = ptRLNextPrev;
        ptRLNextPrev = ptRLCur;

        x = 0;
        while(x < l32Width)
        {
            //��־�����ı�����Ѱ�ұ���seg����ʼλ��
#if 0
            x_st = x;
            s64Search = _itoll(0xFFFFFFFF, 0xFFFFFFFF);
            l32BitCount = 32;
            while(l32BitCount == 32 && x<l32Width)
            {
                s64Data1 = _mem8_const(pu8Src + x);
                s64Data2 = _mem8_const(pu8Src + x + 8);
                s64Data3 = _mem8_const(pu8Src + x + 16);
                s64Data4 = _mem8_const(pu8Src + x + 24);

                u32Mask1 = _dcmpeq4(s64Data1, s64Search);
                u32Mask2 = _dcmpeq4(s64Data2, s64Search);
                u32Mask3 = _dcmpeq4(s64Data3, s64Search);
                u32Mask4 = _dcmpeq4(s64Data4, s64Search);

                u32Mask1 = _pack2(u32Mask2, u32Mask1);
                u32Mask3 = _pack2(u32Mask4, u32Mask3);
                u32Mask3 = _packl4(u32Mask3, u32Mask1);
                u32Mask3 = _bitr(u32Mask3);
                l32BitCount = _lmbd(1, u32Mask3);
                x+= 32;
            }
            x = x - 32 + (l32BitCount);
            if(x>l32Width) x = l32Width;
            seg_st = x; 
            if(x>=l32Width) break;

            s64Search = _itoll(0, 0);
            l32BitCount = 32;
            while(l32BitCount == 32 && x<l32Width)
            {
                s64Data1 = _mem8_const(pu8Src + x);
                s64Data2 = _mem8_const(pu8Src + x + 8);
                s64Data3 = _mem8_const(pu8Src + x + 16);
                s64Data4 = _mem8_const(pu8Src + x + 24);

                u32Mask1 = _dcmpeq4(s64Data1, s64Search);
                u32Mask2 = _dcmpeq4(s64Data2, s64Search);
                u32Mask3 = _dcmpeq4(s64Data3, s64Search);
                u32Mask4 = _dcmpeq4(s64Data4, s64Search);

                u32Mask1 = _pack2(u32Mask2, u32Mask1);
                u32Mask3 = _pack2(u32Mask4, u32Mask3);
                u32Mask3 = _packl4(u32Mask3, u32Mask1);
                u32Mask3 = _bitr(u32Mask3);
                l32BitCount = _lmbd(1, u32Mask3);
                x+= 32;
            }
            x = x - 32 + (l32BitCount);
            if(x>l32Width) x = l32Width;
            seg_ed = x;

#else
            while((pu8Src[x] == 0) & (x < l32Width))
            {
                x ++;
            }
            if(x == l32Width) break;
            seg_st = x;

            //Ѱ�ұ���seg�Ľ���λ��
            while((pu8Src[x] == u8Fg) & (x < l32Width))
            {
                x++;
            }
            seg_ed = x;
#endif

            //��ǰһ��seg���ڽӹ�ϵ
            l32PrevNBCount = 0;
            if(y > 0)
            {
                pu32PrevNB[-1] = 0xFFFFFFFF;//�˱��������ʡ����whileѭ���е�һ��Ԫ�ص��ж�
                //���ǰһ�е�RunLength��Ϣ�ж��ڽӹ�ϵ
                ptRLTemp = ptRLPrev;
                while(ptRLTemp->seg_id != 0)
                {
                    overlap = MIN(seg_ed, ptRLTemp->seg_ed) - MAX(seg_st, ptRLTemp->seg_st);
                    if(overlap >= 0) 
                    {
                        if(pu32PrevNB[l32PrevNBCount - 1] != ptRLTemp->seg_id)
                        {
                            pu32PrevNB[l32PrevNBCount] = ptRLTemp->seg_id;
                            l32PrevNBCount ++;
                        }
                    }
                    ptRLTemp ++;
                }
            }

            if(l32PrevNBCount == 0)
            {
                //δ���Ϸ��ҵ������������seg��Ӧһ��������
                area_id ++;
                seg_id = area_id;
            }
            else
            {
                //�Ϸ��ҵ���ͨ����
                seg_id = pu32PrevNB[0];
                for(id = 1; id<l32PrevNBCount; id++)
                {
                    //seg_id��conn_id�������ӹ�ϵ����¼����
                    if(seg_id < sizeof(au32NSET)/sizeof(au32NSET[0]))
                    {
                        NSET_link(au32NSET, seg_id, pu32PrevNB[id]);
                    }
                }
            }

            ptRLCur->seg_id = seg_id;
            ptRLCur->seg_st = seg_st;
            ptRLCur->seg_ed = seg_ed;
            ptRLCur ++;
        }

        //RunLength�н�����־
        ptRLCur->seg_id = 0;
        ptRLCur++;

        pu8Src += l32Stride;
    }

    //�������ӹ�ϵ�������ܹ������(�����ӹ�ϵ�������٣�����Ͽ�)
    //�õ�һ��ӳ�����������Ϊ֮ǰ�õ���labelλͼ�е�area_id,���Ϊ��ӳ�䵽����id
    //����֮����һ�����1�Ĺ�ϵ��
    //bmp8_write("label_step1.bmp", pDst, 10);

    //NSET���ݽṹ�Ѿ���������ͬ���������id���ӳ�Ϊһ��ѭ������
    //ֻҪ�ֱ����ѭ�������ɵõ�ӳ���
    l32GroupID = 1;
    for(id = 1; id <= area_id; id++)
    {
        pl32IDMap[id] = 0;
    }
    for(id = 1; id <= area_id; id++)
    {
        if(pl32IDMap[id] == 0)
        {
            if(au32NSET[id] == 0)
            {
                //����һ������
                pl32IDMap[id] = l32GroupID;
            }
            else
            {
                //����ͨ����
                next_id = id;
                do
                {
                    pl32IDMap[next_id] = l32GroupID;
                    next_id = au32NSET[next_id];
                }while(next_id != id);
            }
            l32GroupID ++;
        }
    }
    pl32IDMap[0] = 0;//����ɫ��Ȼӳ�䵽����ɫ,������0������


#if 1
    //�滻RunLength��idΪ�궨����Label id
    ptRLCur = atLineRL;
    for(y=0; y<l32Height; y++)
    {
        x = 0;
        while(ptRLCur->seg_id > 0)
        {
            id = pl32IDMap[ptRLCur->seg_id];
            ptRLCur->seg_id = id;

            ptRLCur++;
        }
        ptRLCur++;
    }
#else
    //�滻idλͼΪ��ͨԪ�ص���λͼ,�����滻������������ؽ��еģ����
    //�����ػ��õ���IDΪ������ID
    pu32LBCur = pu32DstLB;
    ptRLCur = atLineRL;
    for(y=0; y<l32Height; y++)
    {
        x = 0;
        while(ptRLCur->seg_id > 0)
        {
            id = pl32IDMap[ptRLCur->seg_id];

            for(;x<ptRLCur->seg_st;x++) pu32LBCur[x] = 0;
            for(;x<ptRLCur->seg_ed;x++) pu32LBCur[x] = id;

            ptRLCur++;
        }
        for(; x<l32Width; x++) pu32LBCur[x] = 0;

        ptRLCur++;
        pu32LBCur += l32Stride;
    }
#endif
    return l32GroupID;
}


/*=============================================================================
������    ��CAntImage::Handle()
����      ����ǰ��ͼ�������̬ѧ��������ͨ�����
�㷨ʵ��  ����
����˵��  ��blobs
nBlobs
����ֵ˵������
����˵��  ����
=============================================================================*/
void CAntImage::ExtractBlobByMophCC(u8 * pu8FgMask)                 //����ǰ��maskͼ��,����������֮��ᱻ���͸�ʴ�޸�
{
    int nCCNumber;
    //�ڲ����������˶���Ա��Ϊ�������
    m_pFgMask = pu8FgMask;

    //�����趨�Ĳ������͸�ʴ
    if(0)
    {
        //�߼���ʹ�ù̶����������Ŀ��С����˲���ʴ
        MophOpen_V1(m_pFgMask, m_width, m_height, 
                    0, 0,
                    m_x_dilate_size, m_y_dilate_size,
                    m_pTempMask);
        //
        //Ŀǰm_pTempf1û�������findBlob2ֱ��ʹ��RunLength��Ϣ����
        nCCNumber = findConnectedComponents_V1(m_pFgMask, (u32*)m_pTempf1,  m_width, m_height, m_width, 255, m_map);

        //findBlobs2���������m_minTargetSizeX, m_minTargetSizeY�趨ɾ����С��Ŀ��
        findBlobs2(atLineRL, nCCNumber);
    }
    else
    {
        MophOpen_V1(m_pFgMask, m_width, m_height, 
                    m_minTargetSizeX, m_minTargetSizeY,
                    m_x_dilate_size, m_y_dilate_size,
                    m_pTempMask);
        //Ŀǰm_pTempf1û�������findBlob2ֱ��ʹ��RunLength��Ϣ����
        nCCNumber = findConnectedComponents_V1(m_pFgMask, (u32*)m_pTempf1,  m_width, m_height, m_width, 255, m_map);
        findBlobs2(atLineRL, nCCNumber);
    }
    return;
}

/*=============================================================================
������    ��CAntImage::erodeMask()
����      ��
�㷨ʵ��  ����
����˵��  ����
����ֵ˵������
����˵��  ��
=============================================================================*/
BOOL CAntImage::erodeMask()
{
    int i, j, halfsizeX,halfsizeY, m,n;
    int IsSetToZero;

    halfsizeX = m_minTargetSizeX/2;
    halfsizeY = m_minTargetSizeY/2;

    // first copy foreground mask to temp mask
    memcpy(m_pTempMask, m_pFgMask, sizeof(unsigned char) * m_width * m_height);

    for (i=0; i<m_height; i++)
    {
        for(j=0; j<m_width; j++)
        {
            if(i>=halfsizeY && i<m_height-halfsizeY && j>=halfsizeX && j < m_width-halfsizeX)
            {
                IsSetToZero = 0;
                for(m=-halfsizeY; m<=halfsizeY && !IsSetToZero; m++)
                    for (n=-halfsizeX; n<=halfsizeX &&!IsSetToZero ;n++)
                    {
                        if(m_pFgMask[(i+m)*m_width + (j+n)] == 0)
                        {
                            m_pTempMask[i*m_width+j] = 0; //black
                            IsSetToZero = 1;
                        }
                    } 	
            }
            else
            {
                m_pTempMask[i*m_width+j] = 0; //black
            }
        }
    }

    return TRUE;
}

/*=============================================================================
������    ��CAntImage::dilateMask()
����      ��
�㷨ʵ��  ����
����˵��  ����
����ֵ˵����
����˵��  ��
=============================================================================*/
BOOL CAntImage::dilateMask()
{
    int i, j, x_halfsize, y_halfsize, m, n;

    // first copy in_mask to out_mask
    memcpy(m_pFgMask,m_pTempMask, sizeof(unsigned char) * m_width * m_height);

    x_halfsize = m_x_dilate_size/2; //horizontal half size
    y_halfsize = m_y_dilate_size/2; //vertical half size 
    for (i=y_halfsize; i<m_height-y_halfsize; i++)
    {
        for(j=x_halfsize;j<m_width-x_halfsize;j++)
        {
            if(m_pTempMask[i*m_width+j])
            {
                for(m=-y_halfsize; m<=y_halfsize; m++)
                    for (n=-x_halfsize; n<=x_halfsize;n++)
                    {
                        m_pFgMask[(i+m)*m_width + (j+n)] = 255; //white
                    }
            }
        }
    }

    return TRUE;
}

/*=============================================================================
������    ��CAntImage::findConnectedComponents()
����      ��
�㷨ʵ��  ����
����˵��  ����
����ֵ˵����
����˵��  ��
=============================================================================*/
BOOL CAntImage::findConnectedComponents()
{
    unsigned char *mask = (unsigned char*)m_pFgMask;
    int *id = (int*)m_pTempf1;
    int x, y, i, j, k, nextId;
    int ids[4];
    int idOfs[4] = { -m_width-1, -m_width, -m_width+1, -1 };   

    // to start, every id value maps to itself
    nextId = 1;
    for(i=0; i<MAX_CC_IDS; i++) m_map[i] = i;

    // scan first pixel as a special case
    if (*mask) 
        *id = nextId++;
    else 
        *id = 0;
    mask++;
    id++;

    // scan rest of first row as a special case   
    for(x=1; x<m_width; x++)
    {
        if (*mask)
        {
            j = *(id - 1);
            if (j > 0) 
                *id = j;
            else 
                *id = nextId++;
        }
        else *id = 0;

        mask++;
        id++;
    }

    // scan rest of rows
    for(y=1; y<m_height; y++)
    {
        // check first pixel of row as a special case
        if (*mask)
        {
            i = *(id - m_width);
            j = *(id - m_width + 1);

            if (j>i) i = j;
            if (i>0) *id = i;
            else *id = nextId++;
        }
        else *id = 0;
        mask++;
        id++;

        // now check the 'middle' of the row
        for(x=1; x<m_width-1; x++)
        {
            if (*mask)
            {
                j = 0;
                // find the max neighbor
                for(i=0; i<4; i++)
                {
                    k = *(id + idOfs[i]);
                    if(k>=MAX_CC_IDS)
                        ids[i] = 0;
                    else
                        ids[i] = m_map[k];
                    if (ids[i] > j) j = ids[i];
                }

                if (j > 0)
                {
                    for(i=0; i<4; i++)
                    {
                        if (ids[i]==0 || ids[i]==j) continue;
                        for(k=1; k<nextId; k++)
                        {
                            if (m_map[k]==ids[i]) m_map[k] = j;
                        }                  
                    }
                    *id = j;
                }
                else{
                    *id = nextId++;
                }
            }
            else *id = 0;

            mask++;
            id++;
        }

        // finally, we can check the last pixel of the row as a special case
        if (*mask)
        {
            i = *(id - m_width - 1);
            j = *(id - m_width);         
            if (j>i) i = j;

            j = *(id - 1);
            if (j>i) i = j;

            if (i>0) *id = i;
            else *id = nextId++;
        }
        else *id = 0;
        mask++;
        id++;

        if (nextId >= MAX_CC_IDS)
        {
            //printf("Error - not enough connected component ids (%d)\n", MAX_CC_IDS);
            return FALSE;
        }
    }

    // pass 2 - update ids in label image according to equiv map
    id = (int*)m_pTempf1;

    for(i=0; i<m_width*m_height; i++)
    {
        if (*id > 0) *id = m_map[*id];
        id++;
    }

    return TRUE;
}

/*=============================================================================
������    ��CAntImage::findBlobs()
����      ��
�㷨ʵ��  ����
����˵��  ����
����ֵ˵����
����˵��  ����
=============================================================================*/
BOOL CAntImage::findBlobs()
{
    int minSize = m_minTargetSizeX * m_minTargetSizeY;
    int *id = (int*)m_pTempf1;
    int x, y, i, j, n;
    AntBlob *tblob;

    m_nBlobs = 0;

    for(i=0; i<MAX_CC_IDS; i++) 
        m_map[i] = -1;

    n = 0;
    for(y=0; y<m_height; y++)
    {
        // blob[X]->user will serve a dual purpose:
        //  1) flag to say whether the blob 'grew' due to the last row
        //  2) index into map array that references this blob
        for(i=0; i<n; i++) 
        {
            m_blobs[i]->user = 0;
        }

        for(x=0; x<m_width; x++)
        {
            j = *id;         
            // is this a blob pixel?
            if (j > 0)
            {
                if (m_map[j] < 0)
                {
                    if (n < MAX_NUM_BLOBS)
                    {
                        // this is a new blob
                        m_map[j] = n;
                        m_blobs[n]->mass = 1;
                        //m_blobs[n]->centroid.x = x;
                        //m_blobs[n]->centroid.y = y;
                        m_blobs[n]->x0 = x;
                        m_blobs[n]->y0 = y;
                        m_blobs[n]->x1 = x;
                        m_blobs[n]->y1 = y;
                        m_blobs[n]->id = j;
                        m_blobs[n]->user = j;
                        n++;
                    }
                }
                else{
                    // this blob already exists
                    m_blobs[m_map[j]]->user = j;
                    j = m_map[j];
                    m_blobs[j]->mass++;
                    //m_blobs[j]->centroid.x += x;
                    //m_blobs[j]->centroid.y += y;
                    if (x > m_blobs[j]->x1) m_blobs[j]->x1 = x;
                    else if (x < m_blobs[j]->x0) m_blobs[j]->x0 = x;

                    if (y > m_blobs[j]->y1) m_blobs[j]->y1 = y;
                    else if (y < m_blobs[j]->y0) m_blobs[j]->y0 = y;               
                }
            }

            id++;
        }      

        // check for finished blobs
        for(i=0; i<n; i++)
        {
            if ((m_blobs[i]->user == 0) && (m_blobs[i]->mass < minSize))
            {
                // kill this blob by moving it to the end of the list and
                //  decrement the list size
                n--;

                // we have to do a proper swap
                tblob = m_blobs[i];
                m_blobs[i] = m_blobs[n];
                m_blobs[n] = tblob;            

                // we also have to update the map            
                m_map[m_blobs[i]->user] = i;

                // since we changed the blob at position <i> we want to reprocess
                //  it next time through the loop
                i--;
            }
        }
    }

    // do some per blob post-processing
    for(i=0; i<n; i++)
    {
        // we accumulated the sum of all (x, y) values
        //  now we need to divide by the mass to get the centroid
        //m_blobs[i]->centroid.x /= m_blobs[i]->mass;
        //m_blobs[i]->centroid.y /= m_blobs[i]->mass;
    }

    m_nBlobs = n;

    return TRUE;
}

/*=============================================================================
������    ��CAntImage::findBlobs2()
����      ��
�㷨ʵ��  ����
����˵��  ��ptLineRL
nCCNumber
����ֵ˵������
����˵��  ������RunLengthͼ����Blob,������ɣ�ͬʱ����ɸѡ��СBlob����˲���Ҫʹ��labelͼ����
=============================================================================*/
BOOL CAntImage::findBlobs2(TRunLength *ptLineRL, int nCCNumber)
{
    int minSize = m_minTargetSizeX * m_minTargetSizeY;
    int x, y, i, j, n, id;
    TRunLength * ptRLCur;
    int seg_len;
    AntBlob *tblob;

    m_nBlobs = 0;

    for(i = 0; i < nCCNumber; i++) m_map[i] = -1;

    //m_map[i]=j,�����i��label�㷨�õ���blob��ӳ�䵽��j��m_blobs�ṹ��
    n = 0;
    ptRLCur = ptLineRL;
    for(y=0; y<m_height; y++)
    {
        for(i=0; i<n; i++) m_blobs[i]->user = 0;

        //��y�����ݷ�������������ǰ����Ϊ��λ
        while(ptRLCur->seg_id > 0)
        {
            id = ptRLCur->seg_id;
            //(ptRLCur->seg_st, ptRLCur->seg_ed)֮�����ǰ��������idΪid
            //�ҵ���ӳ�䵽��blob

            if(m_map[id] == -1 && n<MAX_NUM_BLOBS)  //Խ���ж�
            {
                //�½�blob
                j = m_map[id] = n;
                n++;

                //��ʼ����blob
                //m_blobs[j]->centroid.x = 0;
                //m_blobs[j]->centroid.y = 0;
                m_blobs[j]->mass = 0;
                m_blobs[j]->x0 = m_width;
                m_blobs[j]->y0 = m_height;
                m_blobs[j]->x1 = 0;
                m_blobs[j]->y1 = 0;

            }

            if(m_map[id] != -1)      //Խ���ж�
            {
                j = m_map[id];
                m_blobs[j]->user = id;
                //�ۼ�R,G,B(�Ҷ����������)
                seg_len = (ptRLCur->seg_ed - ptRLCur->seg_st);
                i = ptRLCur->seg_st * 3;
                /*
                for(x = ptRLCur->seg_st; x<ptRLCur->seg_ed; x++, i+=3)
                {
                    m_blobs[j]->centroid.x += x;
                }
                m_blobs[j]->centroid.y += y*seg_len;
                */
                //��������
                m_blobs[j]->mass += seg_len;
                //��������
                if ((ptRLCur->seg_ed-1) > m_blobs[j]->x1) m_blobs[j]->x1 = (ptRLCur->seg_ed-1);
                if ((ptRLCur->seg_st)   < m_blobs[j]->x0) m_blobs[j]->x0 = (ptRLCur->seg_st);
                if (y > m_blobs[j]->y1) m_blobs[j]->y1 = y;
                if (y < m_blobs[j]->y0) m_blobs[j]->y0 = y;
            }
            ptRLCur++;
        }

        //���н��������ҽ�����blob����ʱɾ�������С��
        for(i=0; i<n; i++)
        {
            if ((m_blobs[i]->user == 0) && (m_blobs[i]->mass < minSize))
            {
                // kill this blob by moving it to the end of the list and decrement the list size
                n--;

                tblob = m_blobs[i];
                m_blobs[i] = m_blobs[n];
                m_blobs[n] = tblob;

                // we also have to update the map            
                m_map[m_blobs[i]->user] = i;

                // since we changed the blob at position <i> we want to reprocess
                //  it next time through the loop
                i--;
            }
        }

        ptRLCur++;
    }

    // do some per blob post-processing
    for(i=0; i<n; i++)
    {
        // we accumulated the sum of all (x, y) values
        //  now we need to divide by the mass to get the centroid
        //m_blobs[i]->centroid.x /= m_blobs[i]->mass;
        //m_blobs[i]->centroid.y /= m_blobs[i]->mass;
    }

    m_nBlobs = n;

    return TRUE;
}

#include <math.h>
