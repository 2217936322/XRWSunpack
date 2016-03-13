/*
 XRWSunpack - Unpack X Rebirth Workshop .dat files downloaded from Steam
 Developed by Lit (https://github.com/Lighting/XRWSunpack)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>

#define MAXSIZE 520000
#define XRWS_SIGNATURE "XRWS"
#define XRWS_VERSION 1
#define CONTENT_XML "content.xml"

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
	fprintf(stderr, "Usage: %s [option] .dat_file [OUT_DIR]\n", argv[0]);
	fprintf(stderr, "Unpack X Rebirth Workshop (XRWS) .dat files downloaded from Steam\n");
	fprintf(stderr, "Type %s -h or --help for this help screen\n\n", argv[0]);
	fprintf(stderr, "To download files from Steam Workshop use http://steamworkshopdownloader.com\n");
	fprintf(stderr, "Report bugs to <https://github.com/Lighting/XRWSunpack/issues>\n");
}

void unpack(const char *file, const char *out_dir)
{
	FILE *ifd, *ofd;
	struct header_struct {
		char sig[4];
		unsigned int ver;
		unsigned int files_number;
		unsigned int files_names_len;
		unsigned int files_size;
	} header;
	unsigned int *files_sizes, files_names_pos;
	char *files_names, *data, *point;
	unsigned long read_size, len;
	char out_path[FILENAME_MAX*2], out_path2[FILENAME_MAX*2];
	
	if(*out_dir != '\0' && access(out_dir, W_OK) == -1)
		terminate("No access to directory %s", out_dir);
	
	if((point = strrchr(file, '.')) != NULL)
		if(strcmp(point, ".dat") != 0)
			terminate("File %s must contain .dat extension", file);

	//open XRWS file for reading
	ifd = fopen(file, "rb");
	if(ifd == NULL)
		terminate("Cannot open XRWS file %s", file);
	
	//read and check header
	fread(&header, 1, sizeof(header), ifd);
	//convert integers
	header.ver = ntohl(header.ver);
	header.files_number = ntohl(header.files_number);
	header.files_names_len = ntohl(header.files_names_len);
	header.files_size = ntohl(header.files_size);
	if(strncmp(header.sig, XRWS_SIGNATURE, sizeof(header.sig)) != 0)
		terminate("%s is not a XRWS file", file);
	if(header.ver != XRWS_VERSION)
		terminate("%s have unsupported XRWS version %u", file, header.ver);
	
	//read sizes of files
	files_sizes = malloc(header.files_number * 4);
	memset(files_sizes, 0, sizeof(files_sizes));
	fread(files_sizes, header.files_number, 4, ifd);
	//convert integers
	for(unsigned long counter = 0; counter < header.files_number; counter++)
		files_sizes[counter] = ntohl(files_sizes[counter]);
	
	//read names of files
	files_names = malloc(header.files_names_len);
	memset(files_names, 0, header.files_names_len);
	fread(files_names, 1, header.files_names_len, ifd);
	
	//create extention subdirectory
	strncpy(out_path, file, point - file);
	if(*out_dir != '\0')
		sprintf(out_path2, "%s/%s", out_dir, out_path);
	else
		strcpy(out_path2, out_path);
	if(mkdir(out_path2, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
		printf("Create directory %s\n", out_path2);
	else
		terminate("Connot create directory %s", out_path2);
	
	//create files
	data = malloc(MAXSIZE);
	files_names_pos = 0;
	for(unsigned long counter = 0; counter < header.files_number; counter++)
	{
		sprintf(out_path, "%s/%s", out_path2, files_names + files_names_pos);
		ofd = fopen(out_path, "wb");
		if(ofd == NULL)
			terminate("Cannot create file %s", out_path);

		while((read_size = (files_sizes[counter] - ftell(ofd) > MAXSIZE) ? MAXSIZE : (files_sizes[counter] - ftell(ofd))) > 0)
		{
			len = fread(data, 1, read_size, ifd);
			fwrite(data, 1, len, ofd);
		}
		
		fclose(ofd);
		printf("File %s unpacked\n", out_path);
		files_names_pos += strlen(files_names + files_names_pos) + 1;
	}
	
	printf("%d %d\n", ftell(ifd), header.files_size + sizeof(header) + sizeof(files_sizes) + sizeof(files_names));
	if(ftell(ifd) != (header.files_size + sizeof(header) + sizeof(files_sizes) + sizeof(files_names)))
		terminate("File %s corrupted", file);
	
	sprintf(out_path, "%s/%s", out_path2, CONTENT_XML);
	ofd = fopen(out_path, "wb");
	if(ofd == NULL)
		terminate("Cannot create file %s", out_path);
	
	while((len = fread(data, 1, MAXSIZE, ifd)) > 0)
		fwrite(data, 1, len, ofd);

	fclose(ofd);
	printf("File %s created\n", out_path);

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
