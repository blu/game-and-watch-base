#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

void print_usage(char **argv)
{
	fprintf(stderr, "usage: %s stl_file scale_factor\n", argv[0]);
}

int seekto(FILE *f, const char *str)
{
	const size_t len = strlen(str);
	size_t pos = 0;

	char buf[1];

	while (1 == fread(buf, 1, 1, f)) {
		if (buf[0] == str[pos]) {
			if (++pos == len)
				return 1;
		} else
			pos = 0;
	}

	if (!feof(f)) {
		fprintf(stderr, "error: reading from input\n");
		return -5;
	}

	return 0;
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
	unsigned fcount = 0;
	unsigned vcount = 0;

	while ((ret = seekto(fin, "facet")) > 0) {
		struct {
			float x, y, z;
		} in;
		struct {
			int16_t x, y, z;
		} out;
		int polyn = 3;

		do {
			if ((ret = seekto(fin, "vertex ")) != 1)
				break;
			if (3 != fscanf(fin, "%f %f %f", &in.x, &in.y, &in.z)) {
				fprintf(stderr, "error: reading vertex %u\n", vcount);
				ret = -6;
				break;
			}
			out.x = (int16_t) roundf(in.x * factor);
			out.y = (int16_t) roundf(in.y * factor);
			out.z = (int16_t) roundf(in.z * factor);
			if (1 != fwrite(&out, sizeof(out), 1, fout)) {
				fprintf(stderr, "error: writing to output for vertex %u\n", vcount);
				break;
			}
			vcount++;
		}
		while (--polyn);

		if (polyn)
			break;

		if ((ret = seekto(fin, "endfacet")) < 0) {
			fprintf(stderr, "error: seeking closure for facet %u\n", fcount);
			ret = -6;
			break;
		}
		fcount++;
	}

	fclose(fout);
	fclose(fin);

	if (ret >= 0)
		printf("faces: %u\nverts: %u\n", fcount, vcount);

	return ret;
}
