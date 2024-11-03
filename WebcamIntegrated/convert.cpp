#include "camera.h"

//-------------------------------------------------------------------
//
// Conversion functions
//
//-------------------------------------------------------------------

__forceinline BYTE Clip(int clr)
{
	return (BYTE)(clr < 0 ? 0 : (clr > 255 ? 255 : clr));
}

__forceinline RGBQUAD ConvertYCrCbToRGB(
	int y,
	int cr,
	int cb
)
{
	RGBQUAD rgbq;

	int c = y - 16;
	int d = cb - 128;
	int e = cr - 128;

	rgbq.rgbRed = Clip((298 * c + 409 * e + 128) >> 8);
	rgbq.rgbGreen = Clip((298 * c - 100 * d - 208 * e + 128) >> 8);
	rgbq.rgbBlue = Clip((298 * c + 516 * d + 128) >> 8);

	return rgbq;
}


//-------------------------------------------------------------------
// TransformImage_RGB24 
//
// RGB-24 to RGB-32
//-------------------------------------------------------------------

void TransformImage_RGB24(
	BYTE* pDest,
	LONG        lDestStride,
	const BYTE* pSrc,
	LONG        lSrcStride,
	DWORD       dwWidthInPixels,
	DWORD       dwHeightInPixels
)
{
	for (DWORD y = 0; y < dwHeightInPixels; y++)
	{
		RGBTRIPLE* pSrcPel = (RGBTRIPLE*)pSrc;
		DWORD* pDestPel = (DWORD*)pDest;

		for (DWORD x = 0; x < dwWidthInPixels; x++)
		{
			pDestPel[x] = D3DCOLOR_XRGB(
				pSrcPel[x].rgbtRed,
				pSrcPel[x].rgbtGreen,
				pSrcPel[x].rgbtBlue
			);
		}

		pSrc += lSrcStride;
		pDest += lDestStride;
	}
}

//-------------------------------------------------------------------
// TransformImage_RGB32
//
// RGB-32 to RGB-32 
//
// Note: This function is needed to copy the image from system
// memory to the Direct3D surface.
//-------------------------------------------------------------------

void TransformImage_RGB32(
	BYTE* pDest,
	LONG        lDestStride,
	const BYTE* pSrc,
	LONG        lSrcStride,
	DWORD       dwWidthInPixels,
	DWORD       dwHeightInPixels
)
{
	MFCopyImage(pDest, lDestStride, pSrc, lSrcStride, dwWidthInPixels * 4, dwHeightInPixels);
}

//-------------------------------------------------------------------
// TransformImage_YUY2 
//
// YUY2 to RGB-32
//-------------------------------------------------------------------

void TransformImage_YUY2(
	BYTE* pDest,
	LONG        lDestStride,
	const BYTE* pSrc,
	LONG        lSrcStride,
	DWORD       dwWidthInPixels,
	DWORD       dwHeightInPixels
)
{
	for (DWORD y = 0; y < dwHeightInPixels; y++)
	{
		RGBQUAD* pDestPel = (RGBQUAD*)pDest;
		WORD* pSrcPel = (WORD*)pSrc;

		for (DWORD x = 0; x < dwWidthInPixels; x += 2)
		{
			// Byte order is U0 Y0 V0 Y1

			int y0 = (int)LOBYTE(pSrcPel[x]);
			int u0 = (int)HIBYTE(pSrcPel[x]);
			int y1 = (int)LOBYTE(pSrcPel[x + 1]);
			int v0 = (int)HIBYTE(pSrcPel[x + 1]);

			pDestPel[x] = ConvertYCrCbToRGB(y0, v0, u0);
			pDestPel[x + 1] = ConvertYCrCbToRGB(y1, v0, u0);
		}

		pSrc += lSrcStride;
		pDest += lDestStride;
	}

}


//-------------------------------------------------------------------
// TransformImage_NV12
//
// NV12 to RGB-32
//-------------------------------------------------------------------

void TransformImage_NV12(
	BYTE* pDst,
	LONG dstStride,
	const BYTE* pSrc,
	LONG srcStride,
	DWORD dwWidthInPixels,
	DWORD dwHeightInPixels
)
{
	const BYTE* lpBitsY = pSrc;
	const BYTE* lpBitsCb = lpBitsY + (dwHeightInPixels * srcStride);;
	const BYTE* lpBitsCr = lpBitsCb + 1;

	for (UINT y = 0; y < dwHeightInPixels; y += 2)
	{
		const BYTE* lpLineY1 = lpBitsY;
		const BYTE* lpLineY2 = lpBitsY + srcStride;
		const BYTE* lpLineCr = lpBitsCr;
		const BYTE* lpLineCb = lpBitsCb;

		LPBYTE lpDibLine1 = pDst;
		LPBYTE lpDibLine2 = pDst + dstStride;

		for (UINT x = 0; x < dwWidthInPixels; x += 2)
		{
			int  y0 = (int)lpLineY1[0];
			int  y1 = (int)lpLineY1[1];
			int  y2 = (int)lpLineY2[0];
			int  y3 = (int)lpLineY2[1];
			int  cb = (int)lpLineCb[0];
			int  cr = (int)lpLineCr[0];

			RGBQUAD r = ConvertYCrCbToRGB(y0, cr, cb);
			lpDibLine1[0] = r.rgbBlue;
			lpDibLine1[1] = r.rgbGreen;
			lpDibLine1[2] = r.rgbRed;
			lpDibLine1[3] = 0; // Alpha

			r = ConvertYCrCbToRGB(y1, cr, cb);
			lpDibLine1[4] = r.rgbBlue;
			lpDibLine1[5] = r.rgbGreen;
			lpDibLine1[6] = r.rgbRed;
			lpDibLine1[7] = 0; // Alpha

			r = ConvertYCrCbToRGB(y2, cr, cb);
			lpDibLine2[0] = r.rgbBlue;
			lpDibLine2[1] = r.rgbGreen;
			lpDibLine2[2] = r.rgbRed;
			lpDibLine2[3] = 0; // Alpha

			r = ConvertYCrCbToRGB(y3, cr, cb);
			lpDibLine2[4] = r.rgbBlue;
			lpDibLine2[5] = r.rgbGreen;
			lpDibLine2[6] = r.rgbRed;
			lpDibLine2[7] = 0; // Alpha

			lpLineY1 += 2;
			lpLineY2 += 2;
			lpLineCr += 2;
			lpLineCb += 2;

			lpDibLine1 += 8;
			lpDibLine2 += 8;
		}

		pDst += (2 * dstStride);
		lpBitsY += (2 * srcStride);
		lpBitsCr += srcStride;
		lpBitsCb += srcStride;
	}
}