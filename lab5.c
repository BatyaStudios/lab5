#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifndef null
#define null NULL
#endif

#define Lab5ErrorReadFile 1
#define Lab5ErrorWriteFile 70
#define Lab5ErrorUnknownFormat 3
#define Lab5ErrorParsingFile 4
#define Lab5ErrorUsage 23

struct Img {
	int Width;
	int Heigth;
	int **Src;
	int **Dst;
};

struct Img **ImgSlices;

int ReadLine(FILE *File, char *Buff)
{
	if (!File) {
		printf("Error: Unable to read file.\n");
		//fclose(File);
		return Lab5ErrorReadFile;
	}
	do {
		if (!fgets(Buff, 128, File)) {
			fclose(File);
			return 2;
		}
	} while (Buff[0] == '#');
	return 0;
}

int ImgWrite(const char *FileName, int ImgSliceCount)
{
	int x;
	int y;
	int i;
	FILE *File = fopen(FileName, "w");

	if (!File) {
		printf("Error: Unable to write file.\n");
		return Lab5ErrorWriteFile;
	}

	fprintf(File, "P3\n");
	fprintf(File, "%d %d\n", ImgSlices[0]->Width,
				ImgSlices[0]->Heigth * ImgSliceCount);
	fprintf(File, "255\n");

	for (i = 0; i < ImgSliceCount; i++) {
		for (y = 0; y < ImgSlices[0]->Heigth; y++) {
			for (x = 0; x < ImgSlices[0]->Width; x++) {
				fprintf(File, "%d\n", ImgSlices[i]->Dst[y][x]);
				fprintf(File, "%d\n", ImgSlices[i]->Dst[y][x]);
				fprintf(File, "%d\n", ImgSlices[i]->Dst[y][x]);
			}
			//printf("%d ", ImgSlices[i]->Src[y][x]);
		}
	}
	fclose(File);
}

int ImgRead(const char *FileName, int ImgSliceCount)
{
	int ImgSliceHeigth;
	char Buff[128];
	int Width;
	int Heigth;
	int Result;
	//int MaxValue;
	int i;
	int x;
	int y;
	int o = 0;
	FILE *File = fopen(FileName, "r");

	Result = ReadLine(File, Buff);

	if (Result)
		return Result;
	if (strcmp(Buff, "P3\n")) {
		printf("Error: Unexpected format.\n");
		fclose(File);
		return Lab5ErrorUnknownFormat;
	}

	Result = ReadLine(File, Buff);
	if (Result)
		return Result;

	if (sscanf(Buff, "%d %d", &Width, &Heigth) != 2)
		return Lab5ErrorParsingFile;

	Result = ReadLine(File, Buff);
	if (Result)
		return Result;
	//sscanf(Buff, "%d", &MaxValue);

	ImgSlices = malloc(sizeof(struct Img *) * ImgSliceCount);

	ImgSliceHeigth = Heigth / ImgSliceCount;

	for (i = 0; i < ImgSliceCount; i++) {
		ImgSlices[i] = malloc(sizeof(struct Img));
		ImgSlices[i]->Width = Width;
		ImgSlices[i]->Heigth = ImgSliceHeigth;
		ImgSlices[i]->Src = malloc(ImgSliceHeigth * sizeof(int *));
		ImgSlices[i]->Dst = malloc(ImgSliceHeigth * sizeof(int *));

		for (y = o; y < (o + ImgSliceHeigth); y++) {
			ImgSlices[i]->Src[y-o] = malloc(Width * sizeof(int));
			ImgSlices[i]->Dst[y-o] = calloc(1, Width * sizeof(int));

			for (x = 0; x < Width; x++) {
				int Pix[3];
				int z;

				for (z = 0; z < 3; z++) {
					Result = ReadLine(File, Buff);
					if (Result)
						return Result;
					//sscanf(Buff, "%d", &Pix[z]);
					Pix[z] = atoi(Buff);
				}
				ImgSlices[i]->Src[y-o][x] = (Pix[0] +
							     Pix[1] +
							     Pix[2]) / 3;
			}
		}
		o += ImgSliceHeigth;
	}
	fclose(File);
};

void ImgPass(int ImgSliceIndex)
{
	int x;
	int y;

	for (y = 1; y < ImgSlices[0]->Heigth-1; y++) {
		for (x = 1; x < ImgSlices[0]->Width-1; x++) {
			int Gx;
			int Gy;

			Gx = (ImgSlices[ImgSliceIndex]->Src[y+1][x-1] +
			      2 * ImgSlices[ImgSliceIndex]->Src[y+1][x] +
			      ImgSlices[ImgSliceIndex]->Src[y+1][x+1]) -
			      (ImgSlices[ImgSliceIndex]->Src[y-1][x-1] +
			      2 * ImgSlices[ImgSliceIndex]->Src[y-1][x] +
			      ImgSlices[ImgSliceIndex]->Src[y-1][x+1]);
			Gy = (ImgSlices[ImgSliceIndex]->Src[y-1][x+1] +
			      2 * ImgSlices[ImgSliceIndex]->Src[y][x+1] +
			      ImgSlices[ImgSliceIndex]->Src[y+1][x+1]) -
			     (ImgSlices[ImgSliceIndex]->Src[y-1][x-1] +
			      2 * ImgSlices[ImgSliceIndex]->Src[y][x-1] +
			      ImgSlices[ImgSliceIndex]->Src[y+1][x-1]);

			ImgSlices[ImgSliceIndex]->Dst[y][x] = sqrt(Gx*Gx +
								   Gy*Gy);
		}
	}
}

int main(int argc, char *argv[])
{
	int i = 0;
	int ImgSliceCount = 1;
	char FileSrc[256] = {0};
	char FileDst[256] = {0};

	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-thd")) {
			if (i+1 < argc)
				ImgSliceCount = atoi(argv[i+1]);
		}
		if (!strcmp(argv[i], "-src")) {
			if (i+1 < argc)
				strcpy(FileSrc, argv[i+1]);
		}
		if (!strcmp(argv[i], "-dst")) {
			if (i+1 < argc)
				strcpy(FileDst, argv[i+1]);
		}
	}

	if (FileSrc[0] == 0 || FileDst[0] == 0 || ImgSliceCount < 1) {
		printf("Usage: -src <path> -dst <path> [-thd <int>]\n");
		return Lab5ErrorUsage;
	}

	ImgRead(FileSrc, ImgSliceCount);

	for (i = 0; i < ImgSliceCount; i++) {
		ImgPass(i);
	}

	ImgWrite(FileDst, ImgSliceCount);
}
