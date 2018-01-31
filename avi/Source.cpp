#include "windows.h"
#include  "vfw.h"
#include <iostream>
#include <fstream>
using namespace std;

/* перемешивание кадров из двух видео в одно.
кадр номер i - из первого видео если i mod N = 0,
иначе это кадр из второго
*/
void module(char *name1, char *name2, char *name3) {
	PAVIFILE file, file2, file3;
	PAVISTREAM stream, stream2, stream3;
	AVISTREAMINFO aviInfo, aviInfo2, aviInfo3;
	BITMAPINFOHEADER bmpInfo;
	long bmpInfoSize = sizeof(bmpInfo);
	void *buffer;
	int N = 10; //mod
	int i, j, k;

	AVIFileOpen(&file, name1, OF_READ, NULL);
	AVIFileOpen(&file2, name2, OF_READ, NULL);
	AVIFileOpen(&file3, name3, OF_CREATE | OF_WRITE, NULL);

	AVIFileGetStream(file, &stream, streamtypeVIDEO, 0);
	AVIFileGetStream(file2, &stream2, streamtypeVIDEO, 0);

	AVIStreamInfo(stream, &aviInfo, sizeof(aviInfo));
	AVIStreamInfo(stream2, &aviInfo2, sizeof(aviInfo));

	aviInfo3 = aviInfo;
	for (i = 0, j = 0, k = 0; i < aviInfo.dwLength && j < aviInfo2.dwLength; k++) {
		if (k%N != 0) i++;
		else j++;
	}
	aviInfo3.dwLength = k; //кол-во кадров
	AVIFileCreateStream(file3, &stream3, &aviInfo3); // new video stream	

	AVIStreamReadFormat(stream, 0, &bmpInfo, &bmpInfoSize);
	AVIStreamSetFormat(stream3, 0, &bmpInfo, sizeof(bmpInfo));

	buffer = malloc(bmpInfo.biSizeImage);

	for (i = 0, j = 0, k = 0; k < aviInfo3.dwLength; k++) {
		if (k%N != 0) { // кадр из 1го
			AVIStreamRead(stream, i, 1, buffer, bmpInfo.biSizeImage, NULL, NULL);
			i++;
		}
		else {//кадр из 2го видео
			AVIStreamRead(stream2, j, 1, buffer, bmpInfo.biSizeImage, NULL, NULL);
			j++;
		}
		AVIStreamWrite(stream3, k, 1, buffer, bmpInfo.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);
	}/*
	 if (k < aviInfo3.dwLength && i == aviInfo.dwLength) { //закончилось 1е видео
	 for (; k < aviInfo3.dwLength; k++, j++) {
	 AVIStreamRead(stream2, j, 1, buffer, bmpInfo.biSizeImage, NULL, NULL);
	 AVIStreamWrite(stream3, k, 1, buffer, bmpInfo.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);
	 }
	 }
	 else if (k < aviInfo3.dwLength && j == aviInfo2.dwLength) { //закончилось 2е видео
	 for (; k < aviInfo3.dwLength; k++, i++) {
	 AVIStreamRead(stream, i, 1, buffer, bmpInfo.biSizeImage, NULL, NULL);
	 AVIStreamWrite(stream3, k, 1, buffer, bmpInfo.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);
	 }
	 }*/

	free(buffer);

	AVIStreamRelease(stream);
	AVIStreamRelease(stream2);
	AVIStreamRelease(stream3);
	AVIFileRelease(file);
	AVIFileRelease(file2);
	AVIFileRelease(file3);
}

void reverse(char *name1, char *name2, char *name3) {
	PAVIFILE file, file2, file3;
	PAVISTREAM stream, stream2, stream3;
	AVISTREAMINFO aviInfo, aviInfo2, aviInfo3;
	BITMAPINFOHEADER bmpInfo;
	long bmpInfoSize = sizeof(bmpInfo);
	void *buffer;

	AVIFileOpen(&file, name1, OF_READ, NULL);
	AVIFileOpen(&file2, name2, OF_READ, NULL);
	AVIFileOpen(&file3, name3, OF_CREATE | OF_WRITE, NULL);

	AVIFileGetStream(file, &stream, streamtypeVIDEO, 0);
	AVIFileGetStream(file2, &stream2, streamtypeVIDEO, 0);

	AVIStreamInfo(stream, &aviInfo, sizeof(aviInfo));
	AVIStreamInfo(stream2, &aviInfo2, sizeof(aviInfo));

	aviInfo3 = aviInfo;
	aviInfo3.dwLength = aviInfo.dwLength + aviInfo3.dwLength;
	AVIFileCreateStream(file3, &stream3, &aviInfo3);

	AVIStreamReadFormat(stream, 0, &bmpInfo, &bmpInfoSize);
	AVIStreamSetFormat(stream3, 0, &bmpInfo, sizeof(bmpInfo));

	buffer = malloc(bmpInfo.biSizeImage);
	int k = 0, i;
	for (i = aviInfo.dwLength - 1, k = 0; i >= 0; i--) {
		AVIStreamRead(stream, i, 1, buffer, bmpInfo.biSizeImage, NULL, NULL);
		AVIStreamWrite(stream3, k, 1, buffer, bmpInfo.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);
		k++;
	}
	for (int j = aviInfo2.dwLength - 1; j >= 0; j--, k++) {
		AVIStreamRead(stream2, j, 1, buffer, bmpInfo.biSizeImage, NULL, NULL);
		AVIStreamWrite(stream3, k, 1, buffer, bmpInfo.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);
	}
	free(buffer);

	AVIStreamRelease(stream);
	AVIStreamRelease(stream2);
	AVIStreamRelease(stream3);
	AVIFileRelease(file);
	AVIFileRelease(file2);
	AVIFileRelease(file3);
}

void correlation(char *video, char *outFile) {
	PAVIFILE file;
	PAVISTREAM stream;
	AVISTREAMINFO aviInfo;
	BITMAPINFOHEADER bmpInfo;
	long bmpInfoSize = sizeof(bmpInfo);
	RGBTRIPLE *buffer, *buffer2;

	AVIFileOpen(&file, video, OF_READ, NULL);
	AVIFileGetStream(file, &stream, streamtypeVIDEO, 0);
	AVIStreamInfo(stream, &aviInfo, sizeof(aviInfo));
	AVIStreamReadFormat(stream, 0, &bmpInfo, &bmpInfoSize);
//	cout << "h = " << bmpInfo.biHeight << endl;
//	cout << "w = " << bmpInfo.biWidth << endl;
//	cout << "size image = " << bmpInfo.biSizeImage << endl;
	double size = (double)bmpInfo.biSizeImage;// bmpInfo.biHeight*bmpInfo.biWidth;

	buffer = new RGBTRIPLE[size];
	buffer2 = new RGBTRIPLE[size];
	ofstream out(outFile);
	for (int i = 0; i < aviInfo.dwLength-1; i++) {
		long b;
		AVIStreamRead(stream, i, 1, buffer, size, &b, NULL);
		AVIStreamRead(stream, i+1, 1, buffer2, size, &b, NULL);
		//мат.ожидание
		double matA=0, matB=0, mat=0, corr = 0;
		
		for (int j = 0; j < size; j++) {
			matA += (double)buffer[j].rgbtGreen;
			matB += (double)buffer2[j].rgbtGreen;
		}
		matA = matA / size;
		matB = matB / size;
		//среднекв.отклонение
		double sigma = 0, sigma2 = 0;
		for (int j = 0; j < size; j++) {
			sigma += ((double)buffer[j].rgbtGreen - matA)*((double)buffer[j].rgbtGreen - matA);
			sigma2 += ((double)buffer2[j].rgbtGreen - matB)*((double)buffer2[j].rgbtGreen - matB);
		}
		sigma = sqrt(sigma / (size - 1.0));
		sigma2 = sqrt(sigma2 / (size - 1.0));

		for (int j = 0; j < size; j++) {
			mat += ((double)buffer[j].rgbtGreen - matA)*((double)buffer2[j].rgbtGreen - matB);
		}
		mat = mat / size;
		corr = mat / (sigma*sigma2);
		out << i << ";" << corr << endl;
	}
	out.close();
	delete(buffer);
	delete(buffer2);
}

void saveFrame(char *name, int n) {
	PAVIFILE file;
	PAVISTREAM stream;
	AVISTREAMINFO aviInfo;
	BITMAPINFOHEADER bmpInfo;
	BITMAPFILEHEADER fhdr;
	fhdr.bfType = 0x4d42; // "BM"
	fhdr.bfReserved1 = 0;
	fhdr.bfReserved2 = 0;
	FILE *output;

	long bmpInfoSize = sizeof(bmpInfo);
	RGBTRIPLE *buffer, **triple;
	AVIFileOpen(&file, name, OF_READ, NULL);
	AVIFileGetStream(file, &stream, streamtypeVIDEO, 0);
	AVIStreamInfo(stream, &aviInfo, sizeof(aviInfo));
	AVIStreamReadFormat(stream, 0, &bmpInfo, &bmpInfoSize);

	fhdr.bfSize = sizeof(BITMAPFILEHEADER) + bmpInfo.biSize + bmpInfo.biSizeImage;
	fhdr.bfOffBits = sizeof(BITMAPFILEHEADER) + bmpInfo.biSize;

	buffer = new RGBTRIPLE[bmpInfo.biSizeImage];

	AVIStreamRead(stream, n, 1, buffer, bmpInfo.biSizeImage, NULL, NULL);

	triple = new RGBTRIPLE *[bmpInfo.biHeight];
	for (int i = 0; i < bmpInfo.biHeight; i++) {
		triple[i] = new RGBTRIPLE[bmpInfo.biWidth];
		for (int j = 0; j < bmpInfo.biWidth; j++) {
			triple[i][j] = buffer[bmpInfo.biWidth*i + j];
		}
	}

	fopen_s(&output, "lala.bmp", "wb");
	fwrite(&fhdr, sizeof(fhdr), 1, output);
	fwrite(&bmpInfo, sizeof(bmpInfo), 1, output);
	//fwrite(&buffer, bmpInfo.biSizeImage, 1, output);
	for (int i = 0; i < bmpInfo.biHeight; i++)
		for (int j = 0; j < bmpInfo.biWidth; j++)
			fwrite(&triple[i][j], sizeof(RGBTRIPLE), 1, output);

	fclose(output);
	free(buffer);
	AVIStreamRelease(stream);
	AVIFileRelease(file);
}

void main(int argc, char* argv[]) {
	AVIFileInit();
	module(argv[1], argv[2], "out.avi");
	reverse(argv[2], argv[1], "reverse.avi");
	correlation(argv[1], "video3.csv");
	saveFrame(argv[1], 0);

	AVIFileExit();
}