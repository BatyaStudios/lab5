#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#ifndef null
#define null NULL
#endif

#ifndef byte
#define byte unsigned char
#endif

#define Lab5ErrorReadFile 1
#define Lab5ErrorUnknownFormat 3
#define Lab5ErrorWriteFile 70
#define Lab5ErrorParsingFile 4
#define Lab5ErrorUsage 23
#define Lab5ErrorTiming 394
#define Lab5ErrorThreading 45

pthread_t *ImgPassThreads;
int *ImgPassThreadsArgs;

struct Img {
	int Width;
	int Heigth;

	byte **Src;
	byte **Dst;
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
				fprintf(File, "%d\n", (int)ImgSlices[i]->
					Dst[y][x]);
				fprintf(File, "%d\n", (int)ImgSlices[i]->
					Dst[y][x]);
				fprintf(File, "%d\n", (int)ImgSlices[i]->
					Dst[y][x]);
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
		ImgSlices[i]->Src = malloc(ImgSliceHeigth * sizeof(byte *));
		ImgSlices[i]->Dst = malloc(ImgSliceHeigth * sizeof(byte *));

		for (y = o; y < (o + ImgSliceHeigth); y++) {
			ImgSlices[i]->Src[y-o] = malloc(Width * sizeof(byte));
			ImgSlices[i]->Dst[y-o] =
				calloc(1, Width * sizeof(byte));

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

void *ImgPass(void *Arg)
{
	int x;
	int y;
	int ImgSliceIndex = *((int *)Arg);

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
	pthread_exit(null);
}

int main(int argc, char *argv[])
{
	int i;
	int ImgSliceCount = 1;
	char FileSrc[256] = {0};
	char FileDst[256] = {0};
	struct timespec ImgPassTimeStart, ImgPassTimeFinish;
	double ImgPassTimeElapsed;

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

	if (ImgRead(FileSrc, ImgSliceCount) == Lab5ErrorReadFile)
		return Lab5ErrorReadFile;

	ImgPassThreads = malloc(sizeof(pthread_t) * ImgSliceCount);
	ImgPassThreadsArgs = malloc(sizeof(int) * ImgSliceCount);

	if (clock_gettime(CLOCK_REALTIME, &ImgPassTimeStart)) {
		printf("Error: Unable to measure time.\n");
		return Lab5ErrorTiming;
	}

	for (i = 0; i < ImgSliceCount; i++) {
		ImgPassThreadsArgs[i] = i;
		if (pthread_create(&ImgPassThreads[i], null, ImgPass,
			       (void *)&ImgPassThreadsArgs[i])) {
			printf("Error: Unable to create Thread.\n");
			return Lab5ErrorThreading;
		}
		//ImgPass(&i);
	}

	for (i = 0; i < ImgSliceCount; i++)
		if (pthread_join(ImgPassThreads[i], null)) {
			printf("Error: Unable to join Thread.\n");
			return Lab5ErrorThreading;
		}

	if (clock_gettime(CLOCK_REALTIME, &ImgPassTimeFinish)) {
		printf("Error: Unable to measure time.\n");
		return Lab5ErrorTiming;
	}

	ImgPassTimeElapsed = ImgPassTimeFinish.tv_sec -
			     ImgPassTimeStart.tv_sec;

	ImgPassTimeElapsed += (ImgPassTimeFinish.tv_nsec -
			       ImgPassTimeStart.tv_nsec) / 1000000000.0;

	printf("Done within: %lfsec.\n", ImgPassTimeElapsed);

	ImgWrite(FileDst, ImgSliceCount);

	return 0;
}
