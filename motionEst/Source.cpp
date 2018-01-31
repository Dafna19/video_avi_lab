#include "windows.h"
#include  "vfw.h"
#include <iostream>
#include <fstream>
using namespace std;
#define R 3
#define blockSize 16

BYTE clipping(double x) {
	if (x > 255) x = 255;
	if (x < 0) x = 0;
	return (BYTE)x;
}

//просто делает кадр в удобном triple
void giveFrame(PAVISTREAM stream, RGBTRIPLE *buffer, short **triple, int size, int n, int h, int w) {
	AVIStreamRead(stream, n, 1, buffer, size, NULL, NULL);
	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++) {
			triple[i][j] = clipping(round(0.299*buffer[w*i + j].rgbtRed + 0.587*buffer[w*i + j].rgbtGreen + 0.114*buffer[w*i + j].rgbtBlue));
		}
}

void search(short **base, short **cur, int h, int w, short **diff, int *Dx, int *Dy) {
	int dx, dy;
	for (int x = 0; x < w; x = x + blockSize) {
		for (int y = 0; y < h; y = y + blockSize) {
			double min = MAXINT;

			for (int rx = -R; rx <= R; rx++)
				for (int ry = -R; ry <= R; ry++) { //search
					double L = 0;
					for (int i = 0; i < blockSize; i++)
						for (int j = 0; j < blockSize; j++) {
							if (x + rx + j >= 0 && x + rx + j < w && y + ry + i >= 0 && y + ry + i < h) {
								L += abs(cur[y + i][x + j] - base[y + ry + i][x + rx + j]);
							}
							else 
								L += cur[y + i][x + j];
						}
					L = L / (blockSize*blockSize);
					if (L < min) {
						min = L;
						dx = rx, dy = ry;
						Dx[(w / blockSize)*(y / blockSize) + (x / blockSize)] = dx;//один вектор для всего блока
						Dy[(w / blockSize)*(y / blockSize) + (x / blockSize)] = dy;
						
					}
				}


			for (int i = 0; i < blockSize; i++)
				for (int j = 0; j < blockSize; j++) {
					if (x + j + dx >= 0 && x + j + dx < w && y + i + dy >= 0 && y + i + dy < h)
						diff[y + i][x + j] = clipping(cur[y + i][x + j] - base[y + i + dy][x + j + dx] + 128);
					else
						diff[y + i][x + j] = 128;
				}
		}
	}
}


void main(int argc, char* argv[]) {
	AVIFileInit();
	PAVIFILE file, fileOut;
	PAVISTREAM stream, streamOut;
	AVISTREAMINFO aviInfo;
	BITMAPINFOHEADER bmpInfo;
	long bmpInfoSize = sizeof(bmpInfo);
	RGBTRIPLE *buffer;
	short **triple, **triple2;
	short **diff;
	int h, w;
	int *dx, *dy;

	ofstream file_1;
	file_1.open("Table.txt");
	file_1 << "h_x;h_y\n";

	AVIFileOpen(&file, argv[1], OF_READ, NULL);
	AVIFileGetStream(file, &stream, streamtypeVIDEO, 0);
	AVIStreamInfo(stream, &aviInfo, sizeof(aviInfo));
	AVIStreamReadFormat(stream, 0, &bmpInfo, &bmpInfoSize);
	h = bmpInfo.biHeight, w = bmpInfo.biWidth;

	AVIFileOpen(&fileOut, argv[2], OF_CREATE | OF_WRITE, NULL);
	AVIFileCreateStream(fileOut, &streamOut, &aviInfo);
	AVIStreamSetFormat(streamOut, 0, &bmpInfo, sizeof(bmpInfo));

	buffer = new RGBTRIPLE[bmpInfo.biSizeImage];
	triple = new short *[bmpInfo.biHeight];
	triple2 = new short *[bmpInfo.biHeight];
	diff = new short *[bmpInfo.biHeight];
	for (int i = 0; i < bmpInfo.biHeight; i++) {
		triple[i] = new short[bmpInfo.biWidth];
		triple2[i] = new short[bmpInfo.biWidth];
		diff[i] = new short[bmpInfo.biWidth];
	}
	dx = new int[(w / blockSize)*(h / blockSize)];//столько блоков
	dy = new int[(w / blockSize)*(h / blockSize)];
	double *pdx = new double[2 * R + 1];//а столько возможных значений
	double *pdy = new double[2 * R + 1];
	double blockCount = (w / blockSize)*(h / blockSize);


	giveFrame(stream, buffer, triple, bmpInfo.biSizeImage, 0, bmpInfo.biHeight, bmpInfo.biWidth);//first frame
	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++)
			buffer[w*i + j].rgbtRed = buffer[w*i + j].rgbtBlue = buffer[w*i + j].rgbtGreen = triple[i][j];
	AVIStreamWrite(streamOut, 0, 1, buffer, bmpInfo.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);


	for (int n = 1; n < aviInfo.dwLength; n++) {
		giveFrame(stream, buffer, triple2, bmpInfo.biSizeImage, n, bmpInfo.biHeight, bmpInfo.biWidth);

		for (int i = 0; i < h; i++) //интерполяция
			for (int j = 0; j < w; j++)
				triple[i][j] = (triple[i][j] + triple2[i][j]) / 2;

		search(triple, triple2, h, w, diff, dx, dy);
		
		for (int i = 0; i < h; i++)
			for (int j = 0; j < w; j++)
				buffer[w*i + j].rgbtRed = buffer[w*i + j].rgbtBlue = buffer[w*i + j].rgbtGreen = diff[i][j];

		AVIStreamWrite(streamOut, n, 1, buffer, bmpInfo.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);

		//entropy
		double entx = 0, enty = 0;
		for (int i = 0; i < 2 * R + 1; i++) {
			pdx[i] = 0; pdy[i] = 0;
		}
		for (int i = 0; i < blockCount; i++) {
			pdx[dx[i] + R]++;
			pdy[dy[i] + R]++;
		}
		for (int i = 0; i < 2 * R + 1; i++) {
			if (pdx[i] != 0)
				entx += (pdx[i] / blockCount)*(log2(pdx[i] / blockCount));
			if (pdy[i] != 0)
				enty += (pdy[i] / blockCount)*(log2(pdy[i] / blockCount));

		}
		file_1 <<  -entx << ";" << -enty << endl;
		

		for (int i = 0; i < h; i++)
			for (int j = 0; j < w; j++)
				triple[i][j] = triple2[i][j];
	}


	file_1.close();
	AVIStreamRelease(stream);
	AVIStreamRelease(streamOut);
	AVIFileRelease(file);
	AVIFileRelease(fileOut);
	AVIFileExit();
	free(buffer);
	for (int i = 0; i < bmpInfo.biHeight; i++) {
		free(triple[i]);
		free(triple2[i]);
		free(diff[i]);
	}
	free(triple);
	free(triple2);
	free(diff);
}