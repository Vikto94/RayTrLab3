#ifndef IMAGE_H_INCLUDED
#define IMAGE_H_INCLUDED
#include "utils.h"

enum toneType { X_IS_B_IN_1_dib_255, X_IS_255_IN_1_dib_B, X_IS_LOG_255_B };

class Image {
public:
	Image(unt w, unt h, toneType = X_IS_B_IN_1_dib_255);
	void setPixelColor(unt x, unt y, Color color);
	void saveImage(const char *filename);
private:
	unt m_width, m_height;
	unc* m_img;
	unt m_fSz;
	toneType m_tone;

};

#endif