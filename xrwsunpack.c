/*
 XRWSunpack - Unpack X Rebirth Workshop .dat files downloaded from Steam
 Developed by Lit (https://github.com/Lighting/XRWSunpack)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#define MAXSIZE 520000
#define XRWS_SIGNATURE "XRWS"
#define XRWS_VERSION 1
#define CONTENT_XML "content.xml"
#define PARSE_DAT ".dat"
#define PARSE_EXTENSIONS "extensions_"
#define PARSE_VERSION 'v'
#define PARSE_VERSION_TOKEN '_'
#define PARSE_WARNING "\nDont change file name after downloading from Steam"

#ifdef __linux__
	#define MAKEDIR(a) mkdir(a, 0775)
	#define SWAPBYTES(a) swab(a)
#else
	#define MAKEDIR(a) _mkdir(a)
	#define SWAPBYTES(a) _swab(a)
#endif


const char *xrwsunpack_header = "XRWSunpack v0.1 (" __DATE__ " " __TIME__ ")";

void terminate(const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "Error: ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	exit(1);
}

void usage(char **argv)
{
	fprintf(stderr, "%s\n", xrwsunpack_header);
	fprintf(stderr, "Usage: %s [option] extension.dat [OUT_DIR]\n", argv[0]);
	fprintf(stderr, "Unpack X Rebirth Workshop (XRWS) .dat files downloaded from Steam\n");
	fprintf(stderr, "Type %s -h or --help for this help screen\n\n", argv[0]);
	fprintf(stderr, "To download files from Steam Workshop use http://steamworkshopdownloader.com\n");
	fprintf(stderr, "Report bugs to <https://github.com/Lighting/XRWSunpack/issues>\n");
}

void unpack(const char *file, const char *out_dir)
{
	FILE *ifd, *ofd;
	char header_sig[4];
	unsigned int header_ver, files_number, files_names_len, files_size, reverse_int;
	unsigned int *files_sizes, files_names_pos;
	char *files_names, *data, *name_start, *name_end;
	unsigned long read_size, len;
	char out_path[FILENAME_MAX*2], out_path2[FILENAME_MAX*2];
	
	//check output directory
	if(*out_dir != '\0' && access(out_dir, W_OK) == -1)
		terminate("No access to directory %s", out_dir);

	//remove prefix path from name of file
	name_start = strrchr(file, '/') + 1;
	//remove prefix text from name of file
	name_end = strstr(name_start, PARSE_EXTENSIONS);
	if(name_end != name_start)
		terminate("Name of file %s must begin with \"%s\"%s", file, PARSE_EXTENSIONS, PARSE_WARNING);
	name_start += sizeof(PARSE_EXTENSIONS) - 1;
	//remove extention and check it
	name_end = strrchr(name_start, '.');
	if(name_end == NULL || strcmp(name_end, PARSE_DAT) != 0)
		terminate("Extension of file %s must be \"%s\"%s", file, PARSE_DAT, PARSE_WARNING);
	strncpy(out_path, name_start, name_end - name_start);
	//remove version from name of file
	name_end = strrchr(out_path, PARSE_VERSION_TOKEN);
	if(name_end == NULL || name_end[1] != PARSE_VERSION)
		terminate("Name of file %s must contain version number%s", file, PARSE_WARNING);
	strncpy(out_path2, out_path, name_end - out_path);

	//open XRWS file for reading
	ifd = fopen(file, "rb");
	if(ifd == NULL)
		terminate("Cannot open XRWS file %s", file);
	
	//read and check header
	fread(&header_sig, 1, sizeof(header_sig), ifd);
	fread(&header_ver, 1, sizeof(header_ver), ifd);
	printf("%d", header_ver);
//	header.files_number = __bswap_32(header.files_number);
//	header.files_names_len = __bswap_32(header.files_names_len);
//	header.files_size = __bswap_32(header.files_size);
	if(strncmp(header.sig, XRWS_SIGNATURE, sizeof(header.sig)) != 0)
		terminate("%s is not a XRWS file", file);
	if(header.ver != XRWS_VERSION)
		terminate("%s have unsupported XRWS version %u", file, header.ver);
	
	//read sizes of files
	files_sizes = malloc(header.files_number * 4);
	memset(files_sizes, 0, sizeof(files_sizes));
	fread(files_sizes, header.files_number, 4, ifd);
	//convert integers
//	for(unsigned long counter = 0; counter < header.files_number; counter++)
//		files_sizes[counter] = __bswap_32(files_sizes[counter]);
	
	//read names of files
	files_names = malloc(header.files_names_len);
	memset(files_names, 0, header.files_names_len);
	fread(files_names, 1, header.files_names_len, ifd);
	
	//create extention subdirectory
	if(*out_dir != '\0')
		sprintf(out_path, "%s/%s", out_dir, out_path2);
	else
		strcpy(out_path, out_path2);
	if(MAKEDIR(out_path) == 0)
		printf("Create directory %s\n", out_path);
	else
		terminate("Connot create directory %s", out_path);
	
	//create files
	data = malloc(MAXSIZE);
	files_names_pos = 0;
	for(unsigned long counter = 0; counter < header.files_number; counter++)
	{
		sprintf(out_path2, "%s/%s", out_path, files_names + files_names_pos);
		ofd = fopen(out_path2, "wb");
		if(ofd == NULL)
			terminate("Cannot create file %s", out_path2);

		while((read_size = (files_sizes[counter] - ftell(ofd) > MAXSIZE) ? MAXSIZE : (files_sizes[counter] - ftell(ofd))) > 0)
		{
			len = fread(data, 1, read_size, ifd);
			fwrite(data, 1, len, ofd);
		}
		
		fclose(ofd);
		printf("File %s unpacked\n", out_path2);
		files_names_pos += strlen(files_names + files_names_pos) + 1;
	}
	
	//check data size with header
	if(ftell(ifd) != (header.files_size + sizeof(header) + sizeof(files_sizes) + header.files_names_len))
		terminate("File %s corrupted", file);
	
	//create content.xml
	sprintf(out_path2, "%s/%s", out_path, CONTENT_XML);
	ofd = fopen(out_path2, "wb");
	if(ofd == NULL)
		terminate("Cannot create file %s", out_path2);
	while((len = fread(data, 1, MAXSIZE, ifd)) > 0)
		fwrite(data, 1, len, ofd);
	fclose(ofd);
	printf("File %s created\n", out_path2);

	fclose(ifd);
	free(data);
	free(files_sizes);
	free(files_names);
}

int main(int argc, char **argv)
{
	if(argc < 2)
	{
		usage(argv);
		terminate("\nXRWS file not found");
	}
	
	if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
		usage(argv);
	else
		unpack(argv[1], (argc > 2) ? argv[2] : "");

	return 0;
}
