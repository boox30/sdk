// ivTTSStandardAPISample.c : Defines the entry point for the console application.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ivTTS.h"

/* constant for TTS heap size */
//#define ivTTS_HEAP_SIZE		38000 /* �������Ч */
#define ivTTS_HEAP_SIZE		58000 /* �������Ч */

/* constant for cache allocation */
#define ivTTS_CACHE_SIZE	512
#define ivTTS_CACHE_COUNT	1024
#define ivTTS_CACHE_EXT		8

/* Message */
ivTTSErrID DoMessage()
{
	/* ��ȡ��Ϣ���û�ʵ�� */
	if(1)
	{
		/* �����ϳ� */
		return ivTTS_ERR_OK;
	}
	else
	{
		/* �˳��ϳ� */
		return ivTTS_ERR_EXIT;
	}
}

FILE *fpOutput = 0;
/* output callback */
ivTTSErrID OnOutput(
		ivUInt16		nCode,			/* [in] output data code */
		ivCPointer		pcData,			/* [in] output data buffer */
		ivSize			nSize )			/* [in] output data size */
{
	/* play */
	/* ����ʵ��ƽ̨���������ݴ��������ӿڣ�����ֻ�Ǽ򵥵Ľ��������ݱ������ļ��� */
	fwrite(pcData, 1, nSize, fpOutput);
	return ivTTS_ERR_OK;
}

/* read resource callback */
void ivCall ReadResCB(
		ivPointer		pParameter,		/* [in] user callback parameter */
		ivPointer		pBuffer,		/* [out] read resource buffer */
		ivResAddress	iPos,			/* [in] read start position */
		ivResSize		nSize )			/* [in] read size */
{
	FILE* pFile = (FILE*)pParameter;
	fseek(pFile, iPos, SEEK_SET);
	fread(pBuffer, nSize, 1, pFile);
}

/* output callback */
ivTTSErrID ivCall OutputCB(
		ivPointer		pParameter,		/* [in] user callback parameter */
		ivUInt16		nCode,			/* [in] output data code */
		ivCPointer		pcData,			/* [in] output data buffer */
		ivSize			nSize )			/* [in] output data size */
{
	/* ��ȡ�߳���Ϣ���Ƿ��˳��ϳ� */
	ivTTSErrID tErr = DoMessage();
	if ( tErr != ivTTS_ERR_OK ) return tErr;
	/* ������������ȥ���� */
	return OnOutput(nCode, pcData, nSize);
}

int main(void)
{
	ivHTTS			hTTS;
	ivPByte			pHeap;
	ivTResPackDesc	tResPackDesc;
	ivTTSErrID		ivReturn;

	if (1)
	{
		/* ����� */
		pHeap = (ivPByte)malloc(ivTTS_HEAP_SIZE);
		memset(pHeap, 0, ivTTS_HEAP_SIZE);
		fpOutput = fopen("OutPcm.pcm","wb+");
		if( !fpOutput )
			return 0;

		/* ��ʼ����Դ */
		/* �����ж����Դ�������Էְ�*/
		tResPackDesc.pCBParam = fopen("..\\..\\Resource\\Resource.irf", "rb");
		tResPackDesc.pfnRead = ReadResCB;
		tResPackDesc.pfnMap = NULL;
		tResPackDesc.nSize = 0;

		if (tResPackDesc.pCBParam)
		{
			tResPackDesc.pCacheBlockIndex = (ivPUInt8)malloc(ivTTS_CACHE_COUNT + ivTTS_CACHE_EXT);
			tResPackDesc.pCacheBuffer = (ivPUInt8)malloc((ivTTS_CACHE_COUNT + ivTTS_CACHE_EXT)*(ivTTS_CACHE_SIZE));
			tResPackDesc.nCacheBlockSize = ivTTS_CACHE_SIZE;
			tResPackDesc.nCacheBlockCount = ivTTS_CACHE_COUNT;
			tResPackDesc.nCacheBlockExt = ivTTS_CACHE_EXT;
		}
		else
		{
			return 0;
		}

		/* ���� TTS ʵ�� */
		ivReturn = ivTTS_Create(&hTTS, (ivPointer)pHeap, ivTTS_HEAP_SIZE, ivNull, (ivPResPackDesc)&tResPackDesc, (ivSize)1);

		/* ������Ƶ����ص� */
		ivReturn = ivTTS_SetParam(hTTS, ivTTS_PARAM_OUTPUT_CALLBACK, (ivUInt32)OutputCB);

		/* ���������ı�����ҳ */
		ivReturn = ivTTS_SetParam(hTTS, ivTTS_PARAM_INPUT_CODEPAGE, ivTTS_CODEPAGE_GBK);

		/* �������� */
		ivReturn = ivTTS_SetParam(hTTS, ivTTS_PARAM_LANGUAGE, ivTTS_LANGUAGE_CHINESE);	

		/* �������� */
		ivReturn = ivTTS_SetParam(hTTS, ivTTS_PARAM_VOLUME, ivTTS_VOLUME_NORMAL);

		/************************************************************************
			��ʽ�ϳ�
		************************************************************************/
		/* ���÷�����Ϊ XIAOYAN */
		ivReturn = ivTTS_SetParam(hTTS, ivTTS_PARAM_ROLE, ivTTS_ROLE_XIAOYAN);
		ivReturn = ivTTS_SynthText(hTTS, ivText("��ã������ǿƴ�Ѷ�������ϳ�ϵͳ��"), -1);
		ivReturn = ivTTS_SynthText(hTTS, ivText("Hello, this is iFLYTEK TTS system."), -1);
		/* ���÷�����Ϊ TERRY */
		ivReturn = ivTTS_SetParam(hTTS, ivTTS_PARAM_ROLE, ivTTS_ROLE_TERRY);
		ivReturn = ivTTS_SetParam(hTTS, ivTTS_PARAM_LANGUAGE, ivTTS_LANGUAGE_ENGLISH);
		ivReturn = ivTTS_SynthText(hTTS, ivText("Hello, this is iFLYTEK TTS system."), -1);

		/* ���ʼ�� */
		ivReturn = ivTTS_Destroy(hTTS);

		if ( tResPackDesc.pCacheBlockIndex )
		{
			free(tResPackDesc.pCacheBlockIndex);
		}
		if ( tResPackDesc.pCacheBuffer )
		{
			free(tResPackDesc.pCacheBuffer);
		}
		if ( pHeap )
		{
			free(pHeap);
		}
	}
	fclose(tResPackDesc.pCBParam);
	fclose(fpOutput);	
	return 0;
}

