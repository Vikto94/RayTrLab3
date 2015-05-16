#include "image.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void Image::setPixelColor(unt x, unt y, Color color)
{
	m_img[(x + y * m_width) * 3 + 0] = color.blue;
	m_img[(x + y * m_width) * 3 + 1] = color.green;
	m_img[(x + y * m_width) * 3 + 2] = color.red;
}

float toneMappingOperator(float col, long double x, toneType tp)
{
	switch (tp)
	{
	case X_IS_B_IN_1_dib_255:
		return log((long double)col) / log(x);
	case X_IS_255_IN_1_dib_B:
		return pow(x, (long double)col);
	case X_IS_LOG_255_B:
		return pow((long double)col, x);
	}
}

long double toneMappingCalcX(float col, toneType tp)
{
	switch (tp)
	{
	case X_IS_B_IN_1_dib_255:
		return pow((long double)col, 1 / (long double)255);
	case X_IS_255_IN_1_dib_B:
		return pow((long double)255, 1/(long double)col);
	case X_IS_LOG_255_B:
		return log((long double)255) / log((long double)col);
	}
}

void Image::saveImage(const char *filename)
{
	unsigned char *img = (unsigned char *)malloc(3 * m_width * m_height);
	memset(img, 0, sizeof(img));
	int m_fSz = 54 + 3 * m_width * m_height;
	for (unt i = 0; i < m_width; i++)
	{
		for (unt j = 0; j < m_height; j++)
		{
			int x = i;
			int y = (m_height - 1) - j;
			img[(x + y * m_width) * 3] = 0;
			img[(x + y * m_width) * 3 + 1] = 0;
			img[(x + y * m_width) * 3 + 2] = 0;
		}
	};

	glm::vec3 min(FLT_MAX), max(FLT_MIN);
	for (unt i = 0; i < m_width; i++)
	{
		for (unt j = 0; j < m_height; j++)
		{
			int x = i;
			int y = (m_height - 1) - j;
			if (m_img[(x + y * m_width) * 3] < min.b)
				min.b = m_img[(x + y * m_width) * 3];
			else if (m_img[(x + y * m_width) * 3] > max.b)
				max.b = m_img[(x + y * m_width) * 3];
			if (m_img[(x + y * m_width) * 3 + 1] < min.g)
				min.g = m_img[(x + y * m_width) * 3 + 1];
			else if (m_img[(x + y * m_width) * 3 + 1] > max.g)
				max.g = m_img[(x + y * m_width) * 3 + 1];
			if (m_img[(x + y * m_width) * 3 + 2] < min.r)
				min.r = m_img[(x + y * m_width) * 3 + 2];
			else if (m_img[(x + y * m_width) * 3 + 2] > max.r)
				max.r = m_img[(x + y * m_width) * 3 + 2];
		}
	};
	long double x_r = toneMappingCalcX(max.r - min.r, m_tone), x_g = toneMappingCalcX(max.g - min.g, m_tone), x_b = toneMappingCalcX(max.b - min.b, m_tone);

	for (unt i = 0; i < m_width; i++)
	{
		for (unt j = 0; j < m_height; j++)
		{
			int x = i;
			int y = (m_height - 1) - j;
			img[(x + y * m_width) * 3 + 0] = (unsigned char)toneMappingOperator(m_img[(x + y * m_width) * 3 + 0] - min.b, x_b, m_tone);
			img[(x + y * m_width) * 3 + 1] = (unsigned char)toneMappingOperator(m_img[(x + y * m_width) * 3 + 1] - min.g, x_g, m_tone);
			img[(x + y * m_width) * 3 + 2] = (unsigned char)toneMappingOperator(m_img[(x + y * m_width) * 3 + 2] - min.r, x_r, m_tone);
		}
	};
	
	FILE *f;
	f = fopen(filename, "wb");

	unsigned char bmpFileHdr[14] = { 'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0 };
	unsigned char bmpInfoHdr[40] = { 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0 };
	unsigned char bmpPad[3] = { 0, 0, 0 };

	bmpFileHdr[2] = (unsigned char)(m_fSz);
	bmpFileHdr[3] = (unsigned char)(m_fSz >> 8);
	bmpFileHdr[4] = (unsigned char)(m_fSz >> 16);
	bmpFileHdr[5] = (unsigned char)(m_fSz >> 24);

	bmpInfoHdr[4] = (unsigned char)(m_width);
	bmpInfoHdr[5] = (unsigned char)(m_width >> 8);
	bmpInfoHdr[6] = (unsigned char)(m_width >> 16);
	bmpInfoHdr[7] = (unsigned char)(m_width >> 24);
	bmpInfoHdr[8] = (unsigned char)(m_height);
	bmpInfoHdr[9] = (unsigned char)(m_height >> 8);
	bmpInfoHdr[10] = (unsigned char)(m_height >> 16);
	bmpInfoHdr[11] = (unsigned char)(m_height >> 24);

	fwrite(bmpFileHdr, 1, 14, f);
	fwrite(bmpInfoHdr, 1, 40, f);
	for (unt i = 0; i < m_height; i++) {
		fwrite(img + (m_width * (m_height - i - 1) * 3), 3, m_width, f);
		fwrite(bmpPad, 1, (4 - (m_width * 3) % 4) % 4, f);
	}
	fclose(f);
}

Image::Image(unt w, unt h, toneType tp) :m_width(w), m_height(h), m_tone(tp)
{
	m_img = (unc *)malloc(3 * w * h * sizeof (unc));
	memset(m_img, 0, sizeof(m_img));
	int m_fSz = 54 + 3 * w * h;
	for (unt i = 0; i < w; i++)
	{
		for (unt j = 0; j < h; j++)
		{
			int x = i;
			int y = (m_height - 1) - j;
			m_img[(x + y * w) * 3] = 0;
			m_img[(x + y * w) * 3 + 1] = 0;
			m_img[(x + y * w) * 3 + 2] = 0;
		}
	}
}