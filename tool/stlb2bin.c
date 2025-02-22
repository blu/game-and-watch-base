/*
   Convert a binary STL file to a raw binary suitable for in-place use

   The STL file is expected to contain a TRIANGLE LIST; the resulting
   file contains the same triangle list, with coorinates converted
   to int16_t after multiplication by a scaling factor. The output
   format is as follows:

   uint16_t face_count

   int16_t face0.vertex0.x
   int16_t face0.vertex0.y
   int16_t face0.vertex0.z

   int16_t face0.vertex1.x
   int16_t face0.vertex1.y
   int16_t face0.vertex1.z
   ..
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

void print_usage(char **argv)
{
	fprintf(stderr, "usage: %s stl_file scale_factor\n", argv[0]);
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		print_usage(argv);
		return -1;
	}

	const char *fname_in = argv[1];
	float factor;

	if (1 != sscanf(argv[2], "%f", &factor)) {
		print_usage(argv);
		return -2;
	}

	const char *fname_ext_bin = ".bin";
	const size_t fnamein_len = strlen(fname_in);
	const size_t fextbin_len = strlen(fname_ext_bin);
	char *fname_out = alloca(fnamein_len + fextbin_len + 1);
	memcpy(fname_out, fname_in, fnamein_len);
	memcpy(fname_out + fnamein_len, fname_ext_bin, fextbin_len + 1);

	FILE *fin = fopen(fname_in, "rb");

	if (!fin) {
		fprintf(stderr, "error: reading input file %s\n", fname_in);
		return -3;
	}

	FILE *fout = fopen(fname_out, "wb");

	if (!fout) {
		fclose(fin);
		fprintf(stderr, "error: writing to output %s\n", fname_out);
		return -4;
	}

	int ret = 0;
	uint32_t fcount = 0;
	uint32_t vcount = 0;

	/* seek past input header */
	if (fseek(fin, 80, SEEK_CUR)) {
		fprintf(stderr, "error: seeking past header\n");
		ret = -6;
		goto err;
	}

	if (1 != fread(&fcount, sizeof(fcount), 1, fin)) {
		fprintf(stderr, "error: reading face count\n");
		ret = -6;
		goto err;
	}

	if (fcount > (uint16_t) -1U) {
		fprintf(stderr, "error: face count overflow\n");
		ret = -8;
		goto err;
	}

	if (1 != fwrite(&fcount, sizeof(uint16_t), 1, fout)) {
		fprintf(stderr, "error: writing to output\n");
		ret = -7;
		goto err;
	}

	for (uint32_t fi = 0; fi < fcount; fi++) {
		struct {
			float x, y, z;
		} in[3];
		struct {
			int16_t x, y, z;
		} out[3];

		/* seek past triangle normal */
		if (fseek(fin, sizeof(float) * 3, SEEK_CUR)) {
			fprintf(stderr, "error: seeking past normal for tri %u\n", fi);
			ret = -6;
			break;
		}

		if (1 != fread(&in, sizeof(in), 1, fin)) {
			fprintf(stderr, "error: reading vertices for tri %u\n", fi);
			ret = -6;
			break;
		}
		for (int vi = 0; vi < 3; vi++) {
			out[vi].x = (int16_t) roundf(in[vi].x * factor);
			out[vi].y = (int16_t) roundf(in[vi].y * factor);
			out[vi].z = (int16_t) roundf(in[vi].z * factor);
		}
		if (1 != fwrite(&out, sizeof(out), 1, fout)) {
			fprintf(stderr, "error: writing to output for vertices %u-%u\n", vcount, vcount + 3);
			ret = -7;
			break;
		}

		/* seek past triangle attribute record */
		if (fseek(fin, 2, SEEK_CUR)) {
			fprintf(stderr, "error: seeking past normal for tri %u\n", fi);
			ret = -6;
			break;
		}

		vcount += 3;
	}

err:
	fclose(fout);
	fclose(fin);

	if (ret >= 0)
		printf("faces: %u\nverts: %u\n", fcount, vcount);

	return ret;
}
