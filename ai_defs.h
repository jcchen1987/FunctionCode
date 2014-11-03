/******************************************************************************
ģ����      �� ai_defs
******************************************************************************/
#ifndef __AIDEFS__H__
#define __AIDEFS__H__

#ifdef __cplusplus
extern "C" {
#endif

//==========================���Ͷ���==========================
#ifdef NEED_TYPE_BOOL32
    typedef int BOOL32;
#endif

typedef char            s8;            
typedef short           s16;
typedef unsigned char   u8;   
typedef unsigned short  u16;	
typedef float           f32;
typedef double          d64;

#if defined _TMS320C6400 || defined(ARCH_C667X) || defined (ARCH_C674X) //TI DM64Xƽ̨�궨��
    typedef double             x64;
    typedef long long		   s64;
    typedef unsigned long long u64;
    typedef int                l32;
    typedef unsigned int       u32;
	
	#define INLINE inline
	#define FAST_CALL 
	#define EXPORT 
    #define RESTRICT restrict
#elif defined(ARCH_X86_LINUX) || defined(ARCH_ATOM_LINUX) || defined(ARCH_ARM_LINUX)  || defined(ARCH_POWERPC_LINUX)//(x86��ARMƽ̨)Linuxϵͳ
    typedef unsigned long long x64;
    typedef long long		   s64;
    typedef unsigned long long u64;
    typedef long               l32;
    typedef unsigned long      u32;
	
	#define INLINE inline
	#define FAST_CALL __attribute__((fastcall))
	#define EXPORT 
    #define RESTRICT 
#else //Ŀǰ����Windowsϵͳ
    typedef unsigned __int64   x64;
    typedef __int64			   s64;
    typedef unsigned __int64   u64;
    typedef long               l32;
    typedef unsigned long      u32;
	
	#define INLINE __inline
	#define FAST_CALL __fastcall
	//Dll����֧��
    #define EXPORT __declspec(dllexport)
    #define RESTRICT 
#endif

//Ubuntuϵͳ����
#ifdef ARCH_ARM_UBUNTU
    typedef signed char     s8;         
    typedef signed short    s16;
	  typedef signed __int64	s64;
#endif

// s8���е�ƽ̨����c665x����Ҫ��ʽ����Ϊ�з��ŵ�
#ifdef NEED_TYPE_SIGNED_CHAR
    typedef signed char     s8;
#endif

#ifndef TRUE
    #define TRUE    1
#endif

#ifndef FALSE
    #define FALSE   0
#endif

#ifndef NULL
  #ifdef  __cplusplus
    #define NULL    0
  #else
    #define NULL    ((void *)0)
  #endif
#endif

#ifndef ARCH_ARM_WINCE
//typedef int BOOL;
#if !defined(TYPE_BOOL) && !defined(__INCvxTypesh)
    typedef int BOOL, *PBOOL;
    #define TRUE 1
    #define FALSE 0
    #define TYPE_BOOL
#endif /* BOOL */

#endif

//==========================�����궨��==========================
#ifndef MIN
#define MIN(a, b)   ((a) > (b) ? (b) : (a))
#endif
#ifndef MAX
#define MAX(a, b)   ((a) > (b) ? (a) : (b))
#endif
#ifndef ABS
#define ABS(x) ((x) >= 0 ? (x) : (-(x)))
#endif

//==========================�����ṹ�嶨��======================
#define MAX_NUM_LINE_POINT  64        //���ߵĵ��������
#define MAX_NUM_POLYGON_POINT  64     //����εĵ��������
typedef struct
{
    l32 l32X;
    l32 l32Y;
}TPoint;

typedef struct
{
    f32 f32X;
    f32 f32Y;
}TFPoint;

typedef struct                      //��������
{
    l32 l32PointNum;
    TPoint atPoint[MAX_NUM_LINE_POINT];
}TLine;

typedef struct                      //��������
{
    l32 l32FPointNum;
    TFPoint atFPoint[MAX_NUM_LINE_POINT];
}TFLine;

//�������ұ߽磬���±߽�
//l32Left <= x < l32Top + l32Width;
//l32Top <= x < l32Top + l32Height;
typedef struct
{
    l32 l32Left;  //x����
    l32 l32Top;   //y����
    l32 l32Width;
    l32 l32Height;
}TRect;

typedef struct 
{
    l32 l32PointNum;    
    TPoint atPoint[MAX_NUM_POLYGON_POINT];
}TPolygon;

typedef struct 
{
    l32 l32FPointNum;    
    TFPoint atFPoint[MAX_NUM_POLYGON_POINT];
}TFPolygon;

typedef struct
{
    l32 l32Width;   //���
    l32 l32Height;  //�߶�
}TSize;

//����ͬͼ�����ԣ����ͣ���صĳ���
enum EImageAttribute
{
    //ͼ�����ԣ�Attribute
    //FRAME_I_FORMAT = INTERLACE_CAPTURE
    //FRAME_FORMAT = 0
    //FIELD_FORMAT = INTERLACE_CAPTURE | FIELD_STORE
    FIELD_STORE = (1 << 0),             //0bit:���д��
    INTERLACE_CAPTURE = (1 << 1),       //1bit:���вɼ�    
	FLIP_V = (1 << 2),                  //2bit:��ֱ��ת
	FLIP_H = (1 << 3)                   //3bit:ˮƽ��ת
};

#define ATTRIBUTE_FRAME_I_FORMAT (INTERLACE_CAPTURE)
#define ATTRIBUTE_FRAME_FORMAT (0)
#define ATTRIBUTE_FIELD_FORMAT (INTERLACE_CAPTURE | FIELD_STORE)

#define MAKE_FOURCC(a, b, c, d) (((u32)(a) << 24) | ((u32)(b) << 16) | ((u32)(c) << 8) | (u32)(d))

//����ͼ������Ϊ��Attribute + Type
typedef enum 
{ 
    //�޷���FOURCC��ʾ�����Ͷ����ڴ˴�
    AI_RGB16,
    AI_RGB24,
    AI_RGB = AI_RGB24,
    AI_RGB32,
    AI_HSV,
    AI_Y,

    //=======================================================
    //YUV���ݸ�ʽ��ͬ�洢��ʽ���ܽ��
    //-------------------------------
    //YUV444: Y Plane: U Plane: V Plane
    //      Horizontal      Vertical
    //Y         1               1
    //UV        1               1
    AI_Y444 = MAKE_FOURCC('Y', '4', '4', '4'),
    //-------------------------------
    //UYVY(Y422): YUV 4:2:2 
    //      [U0 Y0 V0 Y1][U2 Y2 V2 Y3]
    //      Horizontal      Vertical
    //Y         1               1
    //UV        2               1
    AI_UYVY = MAKE_FOURCC('U', 'Y', 'V', 'Y'),
    AI_Y422 = MAKE_FOURCC('Y', '4', '2', '2'),
    //-------------------------------
    //YUYV: YUV 4:2:2 
    //      [Y0 U0 Y1 V0][Y2 U2 Y3 V2]
    //      Horizontal      Vertical
    //Y         1               1
    //UV        2               1
    AI_YUYV = MAKE_FOURCC('Y', 'U', 'Y', 'V'),
    //-------------------------------
    //YVYU: YUV 4:2:2 
    //      [Y0 V0 Y1 U0][Y2 V2 Y3 U2]
    //      Horizontal      Vertical
    //Y         1               1
    //UV        2               1
    AI_YVYU = MAKE_FOURCC('Y', 'V', 'Y', 'U'),
    //-------------------------------
    //YV16: YUV422:
    //      Y[W,H] U[W/2, H] V[W/2, H]
    //      Horizontal      Vertical
    //Y         1               1
    //UV        2               1
    AI_YV16 = MAKE_FOURCC('Y', 'V', '1', '6'),
    //-------------------------------
    //I422: YUV422:
    //      Y[N,M] U[N/2,M] V[N/2, M]
    //      Horizontal      Vertical
    //Y         1               1
    //UV        2               1
    AI_I422 = MAKE_FOURCC('I', '4', '2', '2'),
    //-------------------------------
    //I420: YUV420:
    //      Y[N,M] U[N/2,M/2] V[N/2, M/2]
    //      Horizontal      Vertical
    //Y         1               1
    //UV        2               2
    AI_I420 = MAKE_FOURCC('I', '4', '2', '0'),
    //-------------------------------
    //YV12: YUV420:
    //      Y[N,M] V[N/2,M/2] U[N/2, M/2]
    //      Horizontal      Vertical
    //Y         1               1
    //UV        2               2
    AI_YV12 = MAKE_FOURCC('Y', 'V', '1', '2'),
    //-------------------------------
    //NV12: YUV420:
    //      Y[N,M] UV[N/2,M/2]
    //      [U0,V0] [U1,V1]
    //      Horizontal      Vertical
    //Y         1               1
    //UV        2               2
    AI_NV12 = MAKE_FOURCC('N', 'V', '1', '2')
}EImageType;

typedef  struct  
{
    l32 l32Width;
    l32 l32Height;
    l32 l32Stride;
    void *pvBuffer;
}TPlane;

typedef struct
{
	u32 u32Attribute;      //ͼ������,�������ݲμ�EImageAttribute
	u32 u32Type;           //ͼ������,�������ݲμ�EImageType
    //���ΪYUV��֯ģʽ:
    //      atPlane[0]:��Ž�֯����
    //      atPlane[1,2,3]Ϊ0,��Ч
    //���ΪYUV Planeģʽ��
    //      atPlane[0]:���Y����
    //      atPlane[1]:���U����
    //      atPlane[2]:���V����
    //      atPlane[3]Ϊ0,��Ч
    //���ΪY Plane UV��֯���ģʽ��
    //      atPlane[0]:���Y����
    //      atPlane[1]:��Ž�֯��UV����
    //      atPlane[2,3]Ϊ0,��Ч
    //���ΪRGB+������ͨ����
    //      atPlane[0]:���R����
    //      atPlane[1]:���G����
    //      atPlane[2]:���B����
    //      atPlane[3]:��Ű�����ͨ������
	TPlane atPlane[4];
	u32 u32Reserved;
}TImage;

//����ʱ�����TImage
typedef struct
{
    s64 s64TimeStamp; //����ʱ��ms   
    TImage tImg;	
}TFrame;

//�ṩ�궨������TImageͼ���ʽ:
//����RGB24����bufferͼ��ṹ��
#define SET_TIMAGE_RGB24_CONTINUOUS(pImageIn, width, height, Buffer) \
UtiSetTImage(pImageIn, 0, AI_RGB24, width, height, Buffer, NULL, NULL, NULL, 0, 0, 0, 0);
//����RGB32����bufferͼ��ṹ��
#define SET_TIMAGE_RGB32_CONTINUOUS(pImageIn, width, height, Buffer) \
UtiSetTImage(pImageIn, 0, AI_RGB32, width, height, Buffer, NULL, NULL, NULL, 0, 0, 0, 0);
//����I420����bufferͼ��ṹ��
#define SET_TIMAGE_I420_CONTINUOUS(pImageIn, width, height, Buffer) \
UtiSetTImage(pImageIn, 0, AI_I420, width, height, Buffer, NULL, NULL, NULL, 0, 0, 0, 0);
//����YV12����bufferͼ��ṹ��
#define SET_TIMAGE_YV12_CONTINUOUS(pImageIn, width, height, Buffer) \
UtiSetTImage(pImageIn, 0, AI_YV12, width, height, Buffer, NULL, NULL, NULL, 0, 0, 0, 0);
//����NV12����bufferͼ��ṹ��
#define SET_TIMAGE_NV12_CONTINUOUS(pImageIn, width, height, Buffer) \
UtiSetTImage(pImageIn, 0, AI_NV12, width, height, Buffer, NULL, NULL, NULL, 0, 0, 0, 0);
//����YV16����bufferͼ��ṹ��
#define SET_TIMAGE_YV16_CONTINUOUS(pImageIn, width, height, Buffer) \
UtiSetTImage(pImageIn, 0, AI_YV16, width, height, Buffer, NULL, NULL, NULL, 0, 0, 0, 0);

//����I420������bufferͼ��ṹ��,buffer Stride��ͬ�ڸ����������
#define SET_TIMAGE_I420_SEPARATED(pImageIn, width, height, Buffer0, Buffer1, Buffer2) \
UtiSetTImage(pImageIn, 0, AI_I420, width, height, Buffer0, Buffer1, Buffer2, NULL, 0, 0, 0, 0);
//����YV12������bufferͼ��ṹ��,buffer Stride��ͬ�ڸ����������
#define SET_TIMAGE_YV12_SEPARATED(pImageIn, width, height, Buffer0, Buffer1, Buffer2) \
UtiSetTImage(pImageIn, 0, AI_YV12, width, height, Buffer0, Buffer1, Buffer2, NULL, 0, 0, 0, 0);
//����NV12������bufferͼ��ṹ��,buffer Stride��ͬ�ڸ����������
#define SET_TIMAGE_NV12_SEPARATED(pImageIn, width, height, Buffer0, Buffer1, Buffer2) \
UtiSetTImage(pImageIn, 0, AI_NV12, width, height, Buffer0, Buffer1, Buffer2, NULL, 0, 0, 0, 0);
//����YV16������bufferͼ��ṹ��,buffer Stride��ͬ�ڸ����������
#define SET_TIMAGE_YV16_SEPARATED(pImageIn, width, height, Buffer0, Buffer1, Buffer2) \
UtiSetTImage(pImageIn, 0, AI_YV16, width, height, Buffer0, Buffer1, Buffer2, NULL, 0, 0, 0, 0);

//����I420������bufferͼ��ṹ��,buffer Stride�Լ��趨
#define SET_TIMAGE_I420_SEPARATED_STEP(pImageIn, width, height, Buffer0, Buffer1, Buffer2, Stride0, Stride1, Stride2) \
UtiSetTImage(pImageIn, 0, AI_I420, width, height, Buffer0, Buffer1, Buffer2, NULL, Stride0, Stride1, Stride2, 0);
//����YV12������bufferͼ��ṹ��,buffer Stride�Լ��趨
#define SET_TIMAGE_YV12_SEPARATED_STEP(pImageIn, width, height, Buffer0, Buffer1, Buffer2, Stride0, Stride1, Stride2) \
UtiSetTImage(pImageIn, 0, AI_YV12, width, height, Buffer0, Buffer1, Buffer2, NULL, Stride0, Stride1, Stride2, 0);
//����NV12������bufferͼ��ṹ��,buffer Stride�Լ��趨
#define SET_TIMAGE_NV12_SEPARATED_STEP(pImageIn, width, height, Buffer0, Buffer1, Buffer2, Stride0, Stride1, Stride2) \
UtiSetTImage(pImageIn, 0, AI_NV12, width, height, Buffer0, Buffer1, Buffer2, NULL, Stride0, Stride1, Stride2, 0);
//����YV16������bufferͼ��ṹ��,buffer Stride�Լ��趨
#define SET_TIMAGE_YV16_SEPARATED_STEP(pImageIn, width, height, Buffer0, Buffer1, Buffer2, Stride0, Stride1, Stride2) \
UtiSetTImage(pImageIn, 0, AI_YV16, width, height, Buffer0, Buffer1, Buffer2, NULL, Stride0, Stride1, Stride2, 0);

/*====================================================================
������      ��  UtiSetTImage
����        ��	����ͼ��ṹ��
����ȫ�ֱ�����	��
�������˵����  Out:   *pImageIn:       ͼ��ṹ��
                In:     u32Attribute    ͼ������
                In:     u32Type         ͼ������
                In:     u32Width        ͼ����
                In:     u32Height       ͼ��߶�
                In:     pvBuffer0       buffer0��ַ
                In:     pvBuffer1       buffer1��ַ
                In:     pvBuffer2       buffer2��ַ
                In:     pvBuffer3       buffer3��ַ
                In:     u32Stride0      buffer0����
                In:     u32Stride1      buffer1����
                In:     u32Stride2      buffer2����
                In:     u32Stride3      buffer3����
For example:
                1:����YUV��stride���ڿ��
                UtiSetTImage(pImageSrc, 0, I420, 352, 288, 
                          u8YUVBuf, 0, 0, 0,
                          0, 0, 0, 0);
                2:VU buffer������Y buffer ����
                UtiSetTImage(pImageSrc, 0, YV16,  352, 288, 
                          u8YBuf, u8UBuf, 0, 0,
                          720, 352, 0, 0);
                3:YU buffer������ V buffer���� ����ʽ,ˮƽ��ת
                UtiSetTImage(pImageSrc, INTERLACE | FIELD | FLIP_H, I420,  352, 288, 
                          u8YBuf, 0, u8VBuf, 0,
                          1024, 0, 720, 0);
��ֵ˵��  ��	��
=============================================================================*/
EXPORT void UtiSetTImage(TImage *pImageIn, 
                         u32 u32Attribute, u32 u32Type, 
                         l32 l32Width, l32 l32Height,
                         void *pvBuffer0, void *pvBuffer1, void *pvBuffer2,void *pvBuffer3,
                         l32 l32Stride0, l32 l32Stride1, l32 l32Stride2, l32 l32Stride3);
                      
/*====================================================================
������      ��  UtiIsTImageLegal
����        ��  �ж�ͼ��ṹ���Ƿ�Ϸ�
����ȫ�ֱ�����  ��
�������˵����  in:   pImageIn:       ͼ��ṹ��
                  in:   u32WDivisor0, u32HDivisor0, Plane0���Լ��
                  in:   u32WDivisor1, u32HDivisor1, Plane1,Plane2,Plane3���Լ��
				  in:   u32Align0:      Plane0�������
                  in:   u32Align1:      Plane1�������
                  in:   u32Align2:      Plane2�������
                  in:   u32Align3:      Plane3�������
��ֵ˵��  ��	 u32Result��
                 32                 --->               0
                 .... .... .... ..xx xxxx xxxx xxxx xxxx
Field[3-0]: ��ʾPlane3�Ƿ�������
            bit0��bufferΪ��
            bit1��bufferû���������Ҫ��
            bit2��buffer��߳������Χ��û������Լ��Ҫ��
            bit3��buffer����С�ڿ��
Field[7-4]: ��ʾPlane2�Ƿ�������
            bit4��bufferΪ��
            bit5��bufferû���������Ҫ��
            bit6��buffer��߳������Χ��û������Լ��Ҫ��
            bit7��buffer����С�ڿ��
Field[11-8]: ��ʾPlane1�Ƿ�������
            bit8��bufferΪ��
            bit9��bufferû���������Ҫ��
            bit10��buffer��߳������Χ��û������Լ��Ҫ��
            bit11��buffer����С�ڿ��
Field[15-12]: ��ʾPlane0�Ƿ�������
            bit12��bufferΪ��
            bit13��bufferû���������Ҫ��
            bit14��buffer��߳������Χ��û������Լ��Ҫ��
            bit15��buffer����С�ڿ��
Field[17-16]:
            bit16��ͼ��������ϴ���
            bit17��ͼ�����ʹ���
=============================================================================*/                      
EXPORT u32 UtiIsTImageLegal(const TImage *pImageIn, 
                            u32 u32WDivisor0, u32 u32HDivisor0, 
                            u32 u32WDivisor1, u32 u32HDivisor1, 
                            u32 u32Align0, u32 u32Align1, u32 u32Align2, u32 u32Align3);

//==========================�����붨��==========================
typedef enum
{
    EStatus_Success = 0,                             // �ɹ�
    EStatus_InvalidParameter = 0x80000001,           // ��������
    EStatus_OutOfMemory = 0x80000002,                // �ڴ����ʧ��
    EStatus_InsufficientBuffer = 0x80000003,         // buffer��С����
    EStatus_GenericError = 0x80000004,               // �ض�����
    EStatus_Undefined = 0x80000005                   // δ֪����
}EStatus;
     
//==========================�澯����============================         
//���û����÷���һ��
typedef enum
{

    //��ͨ�ܽ�澯����
    ALARM_CROSS_LINE = 4,           //���߸澯
    ALARM_CROSS_WALL,           //��ǽ�澯
    ALARM_CROSS_AREA,           //Խ����澯
    ALARM_WANDER,               //�ǻ��澯

    //��Ϊ�����澯
    ALARM_RABBLE = 4000,               //���ڸ澯
    ALARM_RUNNING,              //���ܸ澯
    ALARM_FIGHTING,              //��ܸ澯
    ALARM_AREAGUARD_IN,          //���򿴷�����澯
    ALARM_AREAGUARD_OVERTIME,    //���򿴷�������ʱ�澯
    ALARM_AREAGUARD_OUT,         //���򿴷���ȥ�澯
    ALARM_RISE,                  //������澯
    ALARM_ONDUTYDETECT_IN,          //ֵ�ڼ�����澯
    ALARM_ONDUTYDETECT_OVERTIME,    //ֵ�ڼ�ⶺ����ʱ�澯
    ALARM_ONDUTYDETECT_OUT,         //ֵ�ڼ���ȥ�澯
	ALARM_SPEECHDETECT,             //��������쳣�澯
    
    //������ѯ�澯
	ALARM_BLUR = 5000,                 //ģ��
	ALARM_BRIGHTHIGH,            //����
	ALARM_BRIGHTLOW,             //����
	ALARM_BRIGHTCON,             //�Աȶ��쳣
	ALARM_FREEZE,                //���涳��
	ALARM_HUE,                   //ɫƫ
	ALARM_INTERFERENCE,          //����
	ALARM_PTZSTATIC,             //PTZ��⾲ֹ
	ALARM_PTZLEFT,               //PTZ�������
	ALARM_PTZRIGHT,              //PTZ�������
	ALARM_PTZUP,                 //PTZ�������
	ALARM_PTZDOWN,               //PTZ�������
	ALARM_PTZUPLEFT,             //PTZ���������
	ALARM_PTZUPRIGHT,            //PTZ���������
	ALARM_PTZDOWNLEFT,           //PTZ���������
	ALARM_PTZDOWNRIGHT,          //PTZ���������
	ALARM_PTZZOOMIN,             //PTZ���ZoomIn
	ALARM_PTZZOOMOUT,            //PTZ���ZoomOut
	ALARM_SHAKE,                 //����
	ALARM_SHELTER,               //�ڵ�
	ALARM_SIGLOSS,               //�źŶ�ʧ
	ALARM_SCENECHANGE            //�����л�
}EAlarmType;

#ifdef __cplusplus
}
#endif

#endif
