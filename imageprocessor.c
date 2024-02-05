#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

// Typedef:
typedef enum {IDENTITY, INFO, HFLIP, VFLIP, GRAYSCALE} operation_t; 

typedef struct {
	short file_signature;			// 0x00
	int sizeof_bmpfile_inbytes; 	// 0x02
	int pixeldata_offset_inbytes;	// 0x0A	
	int sizeof_header_inbytes;		// 0x0E
	int countof_width_inpixels;		// 0x12
	int countof_height_inpixels;	// 0x16	
	int compression_method_number; 	// 0x1E
	short sizeof_pixel_inbits;		// 0x1C
} header_extract_t;

typedef struct {
	unsigned int red;
	unsigned int green;
	unsigned int blue;
	unsigned int alpha;
} bitmask_t;

// Constants:
const int BI_BITFIELDS = 3;
const int SIZEOF_BYTE_INBITS = 8;
const double GRAYSCALE_FACTOR_RED = 0.299;
const double GRAYSCALE_FACTOR_GREEN = 0.587;
const double GRAYSCALE_FACTOR_BLUE = 0.114;
const short FILE_SIGNATURE_INDICATING_BITMAP = 0x4D42;
const int SIZEOF_HEADER_RAWBYTES_INBYTES = 54;
const int HEADER_SIZE_INDICATING_RGB_BITMASK = 0x28;
const int HEADER_SIZE_INDICATING_RGBA_BITMASK = 0x6C;
const int SIZEOF_RGB_BITMASK_RAWBYTES_INBYTES = 12;
const int SIZEOF_RGBA_BITMASK_RAWBYTES_INBYTES = 68;
const int SIZEOF_MAX_PADDING_INBYTES = 4;
const int SIZEOF_COLORTABLE_ENTRY_INBYTES = 4;
const int COLORTABLE_BPP_CUTOFF_INBITS = 8;
//											RED			GREEN		BLUE		ALPHA										
const bitmask_t COLORTABLE_BITMASK		= {0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000};
const bitmask_t DEFAULT_BITMASK_16BIT 	= {0x00007C00, 0x000003E0, 0x0000001F, 0x00008000}; //default bitmask will have the alpha, just for completeness and to ensure that the "alpha bits" of the image aren't modified when they don't need to.
const bitmask_t DEFAULT_BITMASK_24BIT 	= {0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000};
const bitmask_t DEFAULT_BITMASK_32BIT 	= {0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000}; //default bitmask will have the alpha, just for completeness and to ensure that the "alpha bits" of the image aren't modified when they don't need to.

// Global Variables: 
/*
Poilcy for gobal variables is as follows:
- READ: any function may read the value of a global variable.
- MODIFY: only main() may modify the value of a global variable.
*/
operation_t operation;
bitmask_t bitmask;

char *header_rawbytes; 
char *bitmask_rawbytes; // NOTE: the different versions of the bitmapinfoheader have different lengths for the infoheader. BUT, since they all expand on the prior, this script will leave the infoheader as a constant 54 bytes and put all new fields in the bitmask array 
char *colortable_rawbytes; // some formats may have no colortable
char *pixeldata_rawbytes; 
int sizeof_bitmask_rawbytes_inbytes;
int sizeof_colortable_rawbytes_inbytes;
int sizeof_pixeldata_rawbytes_inbytes;

int file_signature;
int sizeof_bmpfile_inbytes;
int pixeldata_offset_inbytes;
int sizeof_header_inbytes;
int countof_width_inpixels;
int countof_height_inpixels;
int compression_method_number;
int sizeof_pixel_inbits;
int countof_colortable_entries;

int sizeof_pixel_inbytes;
int countof_width_inbytes; // excluding padding
int countof_padding_inbytes; // since the BMP scan line has a width in pixels and padding is added to arrive at byte multiples of 4, there must be a consistent number of padding bytes for each line; bytes are greedily interperted as pixels until there are not enough bytes left per the bpp. 
int countof_total_width_inbytes; // including padding. AKA stride, see here: https://learn.microsoft.com/en-us/windows/win32/medfound/image-stride. 

int condition_has_colortable;
int condition_has_bitmask;
int condition_has_rgb_bitmask;
int condition_has_rgba_bitmask;

// Functions:

// 1. Read
int read_arg_to_operation(operation_t *operation_return, int argc, char **argv);
int read_stdin_to(char *rawbytes_return, int countof_items_to_read_inbytes);

// 2. Parse
int parse_header_rawbytes_to_header_extract(header_extract_t *header_extract_return); // also verify if is a valid BMP given the first byte of the header
int parse_bitmask_rawbytes_to_bitmask(bitmask_t *bitmask_return); // though custom bitmasks will not be asked, it is still useful to just parse the bitmask given that, espeically for 16-bpp, there is a distinct 5:6:5 and 1:5:5:5 format which is differentiated by bitmask 

// 3. Operate
// 3.1 Operate - INFO
// nothing to operate, just output to stdout

// 3.2 Operate - HFLIP/VFLIP
int flip_pixel_indices_row(char *pixeldata_rawbytes_return); 
int flip_pixel_indices_collumn(char *pixeldata_rawbytes_return);
int generate_pixel_indices(int *pixel_indices, int sizeof_pixel_indices_inbytes, int pixel_index_increment_inbytes);
int generate_pixel_indices_collumn(int *pixel_indices);
int copy_inversed_pixel_indices_to(char *arr, int *pixel_indices, int pixel_index_increment_inbytesxel_indices_inbytes, int iteration_increment_value, int iteration_stop_value);

// 3.3 Operate - GRAYSCALE
int scale_gray_colortable(char *colortable_rawbytes_return); // will only be used for palletized/indexed bpp-values, in our case 8-bit 
int scale_gray_pixeldata(char *pixeldata_rawbytes_return); // the struct returned from 'parse_bitmask_rawbytes_to_bitmask()' will be used to apply the grayscale effect to the appropriate bits
int scale_gray_element(unsigned char* element_source_pntr, unsigned char* element_dest_pntr, int sizeof_element_inbytes);
int compact_value_from_bitmask(unsigned int element_inbits, unsigned int bitmask);
int expand_value_from_bitmask (unsigned int value_inbits, unsigned int bitmask);
int calculate_number_of_bits_in_bitmask(unsigned int bitmask);

// 4. Output
int write_to_stdout(char *rawbytes, int countof_items_to_write_inbytes);
int write_info_to_stdout();
int write_bmp_to_stdout();

// temp
int temp_write_to_stdout(char * arr) {
	int return_count = fwrite(arr, 1, 196662, stdout);
	// printf("return_count =%d", return_count);
	return return_count;
}

int main(int argc, char **argv) {

	operation_t *operation_return = (operation_t *) malloc(sizeof(operation_t));
	if (read_arg_to_operation(operation_return, argc, argv)) {
		return 1;
	} 
	operation = *operation_return;

	#ifdef _WIN32 //neccesary so that reading from stdin on windows works properly
	_setmode( _fileno( stdin ), _O_BINARY );
	_setmode( _fileno( stdout ), _O_BINARY );
	#endif
	
	char *header_rawbytes_return = (char *) malloc(SIZEOF_HEADER_RAWBYTES_INBYTES);
	if (read_stdin_to(header_rawbytes_return, SIZEOF_HEADER_RAWBYTES_INBYTES) != 0) {
		return 1;
	}
	header_rawbytes = header_rawbytes_return;
	
	header_extract_t *header_extract_return = (header_extract_t *) malloc(sizeof(header_extract_t));
	if (parse_header_rawbytes_to_header_extract(header_extract_return) != 0) {
		return 1;
	}
	file_signature = header_extract_return->file_signature;
	sizeof_bmpfile_inbytes = header_extract_return->sizeof_bmpfile_inbytes;
	pixeldata_offset_inbytes = header_extract_return->pixeldata_offset_inbytes;
	compression_method_number = header_extract_return->compression_method_number;
	sizeof_header_inbytes = header_extract_return->sizeof_header_inbytes;
	countof_width_inpixels = header_extract_return->countof_width_inpixels;
	countof_height_inpixels = header_extract_return->countof_height_inpixels;
	sizeof_pixel_inbits = header_extract_return->sizeof_pixel_inbits;

	sizeof_pixel_inbytes = sizeof_pixel_inbits/SIZEOF_BYTE_INBITS;
	countof_width_inbytes = countof_width_inpixels * sizeof_pixel_inbytes;
	int countof_excess_inbytes = countof_width_inbytes % SIZEOF_MAX_PADDING_INBYTES; // modulo "chops off" the excess chunk of the width beyond a padding interval
	if (countof_excess_inbytes > 0) countof_padding_inbytes = (SIZEOF_MAX_PADDING_INBYTES - countof_excess_inbytes); // pad the difference between this excess and the next padding interval
	countof_total_width_inbytes = countof_width_inbytes + countof_padding_inbytes;
	
	condition_has_bitmask = (compression_method_number == BI_BITFIELDS); 
	condition_has_colortable = (sizeof_pixel_inbits <= COLORTABLE_BPP_CUTOFF_INBITS);

	if (condition_has_bitmask) {
		condition_has_rgb_bitmask = (sizeof_header_inbytes == HEADER_SIZE_INDICATING_RGB_BITMASK);
		condition_has_rgba_bitmask = (sizeof_header_inbytes == HEADER_SIZE_INDICATING_RGBA_BITMASK);
		
		if (condition_has_rgb_bitmask) sizeof_bitmask_rawbytes_inbytes = SIZEOF_RGB_BITMASK_RAWBYTES_INBYTES;
		if (condition_has_rgba_bitmask) sizeof_bitmask_rawbytes_inbytes = SIZEOF_RGBA_BITMASK_RAWBYTES_INBYTES;

		char *bitmask_rawbytes_return = (char *) malloc(sizeof_bitmask_rawbytes_inbytes);		
		if (read_stdin_to(bitmask_rawbytes_return, sizeof_bitmask_rawbytes_inbytes)) {
			return 1;
		}
		bitmask_rawbytes = bitmask_rawbytes_return;

		bitmask_t *bitmask_return = (bitmask_t *) malloc(sizeof(bitmask_t));
		if (parse_bitmask_rawbytes_to_bitmask(bitmask_return) != 0) {
			return 1;
		}  		
		bitmask = *bitmask_return;

	} else {
		if (sizeof_pixel_inbits <= 8) bitmask = COLORTABLE_BITMASK; // this bitmask is assumed to be the standard RGBA format
		if (sizeof_pixel_inbits == 16) bitmask = DEFAULT_BITMASK_16BIT;
		if (sizeof_pixel_inbits == 24) bitmask = DEFAULT_BITMASK_24BIT;
		if (sizeof_pixel_inbits == 32) bitmask = DEFAULT_BITMASK_32BIT;
	}

	if (condition_has_colortable) {
		countof_colortable_entries = (1 << sizeof_pixel_inbits); // alternative form for 2^sizeof_pixel_inbits, more efficient to bit shift.
		sizeof_colortable_rawbytes_inbytes = countof_colortable_entries * SIZEOF_COLORTABLE_ENTRY_INBYTES;

		char *colortable_rawbytes_return = (char *) malloc(sizeof_colortable_rawbytes_inbytes);		
		if (read_stdin_to(colortable_rawbytes_return, sizeof_colortable_rawbytes_inbytes)) {
			fprintf(stderr, "[ERROR] Unable to read stdin as BMP colortable\n");
			return 1;
		} 		
		colortable_rawbytes = colortable_rawbytes_return;
	}

	sizeof_pixeldata_rawbytes_inbytes = sizeof_bmpfile_inbytes - (SIZEOF_HEADER_RAWBYTES_INBYTES + sizeof_bitmask_rawbytes_inbytes + sizeof_colortable_rawbytes_inbytes);
	if (sizeof_pixeldata_rawbytes_inbytes != countof_total_width_inbytes * countof_height_inpixels) {
		fprintf(stderr, "[WARN] sizeof_pixeldata_rawbytes_inbytes != countof_total_width_inbytes * countof_height_inpixels\n");
	}

	char *pixeldata_rawbytes_return = (char *) malloc(sizeof_pixeldata_rawbytes_inbytes);		
	if (read_stdin_to(pixeldata_rawbytes_return, sizeof_pixeldata_rawbytes_inbytes)) {
		fprintf(stderr, "[ERROR] Unable to read stdin as BMP pixeldata\n");
		return 1;
	} 		
	pixeldata_rawbytes = pixeldata_rawbytes_return;


	if (operation == IDENTITY) {
		if (write_bmp_to_stdout() != 0) {
			fprintf(stderr, "[ERROR] Unable to write BMP to stdout\n");
			return 1;
		}
		return 0;
	}
	
	if (operation == INFO) {
		write_info_to_stdout();
		return 0;
	}
	
	if (operation == HFLIP || operation == VFLIP) {
		char *pixeldata_rawbytes_return = (char *) malloc(sizeof_pixeldata_rawbytes_inbytes); // get a new block of memory allocated, following the policy that the global 'pixeldata_rawbytes' can only be modified in main().
		if (operation == HFLIP) {
			flip_pixel_indices_row(pixeldata_rawbytes_return);
		} 
		if (operation == VFLIP) {
			flip_pixel_indices_collumn(pixeldata_rawbytes_return);
		};
		pixeldata_rawbytes = pixeldata_rawbytes_return;

		if (write_bmp_to_stdout() != 0) {
			fprintf(stderr, "[ERROR] Unable to write BMP to stdout\n");
			return 1;
		}
		return 0;
	}

	if (operation == GRAYSCALE) {
		if (condition_has_colortable) {
			char *colortable_rawbytes_return = (char *) malloc(sizeof_colortable_rawbytes_inbytes);
			scale_gray_colortable(colortable_rawbytes_return);;
			colortable_rawbytes = colortable_rawbytes_return;
		} else { 
			char *pixeldata_rawbytes_return = (char *) malloc(sizeof_pixeldata_rawbytes_inbytes);
			scale_gray_pixeldata(pixeldata_rawbytes_return);;
			pixeldata_rawbytes = pixeldata_rawbytes_return;
		}

		if (write_bmp_to_stdout() != 0) {
			fprintf(stderr, "[ERROR] Unable to write BMP to stdout\n");
			return 1;
		}
		return 0;
	}

	return 1;
} 

int read_arg_to_operation(operation_t *operation_return, int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "[ERROR] Invalid number of operations specified: argc=%d\n",argc);
		return 1;
	}

	char* arg_1 = argv[1];
		if (strcmp("IDENTITY", arg_1) == 0) *operation_return = IDENTITY;
		else if (strcmp("INFO", arg_1) == 0) *operation_return = INFO;
		else if (strcmp("HFLIP", arg_1) == 0) *operation_return = HFLIP;
		else if (strcmp("VFLIP", arg_1) == 0) *operation_return = VFLIP;
		else if (strcmp("GRAYSCALE", arg_1) == 0) *operation_return = GRAYSCALE;
		else {
			fprintf(stderr, "[ERROR] Invalid operations specified: argv[1]=%s\n",argv[1]);
			return 1;
		}
	return 0;
}

int read_stdin_to(char *rawbytes_return, int countof_items_to_read_inbytes) {
	int countof_items_read_inbytes = fread(rawbytes_return, 1, countof_items_to_read_inbytes, stdin);
	if (countof_items_read_inbytes != countof_items_to_read_inbytes) {
		fprintf(stderr, "[ERROR] (read_stdin_to): countof_items_to_read_inbytes=%d, countof_items_read_inbytes=%d\n", countof_items_to_read_inbytes, countof_items_read_inbytes);
		return 1;
	} 
	if (ferror(stdin)) {
		fprintf(stderr, "[ERROR] (read_stdin_to): error occured while reading from stdin");
		return 1;
	}
	return 0; 
}

int parse_header_rawbytes_to_header_extract(header_extract_t *header_extract_return) {
	header_extract_return->file_signature = *(typeof(file_signature) *)(header_rawbytes + 0x00);
	header_extract_return->sizeof_bmpfile_inbytes = *(typeof(sizeof_bmpfile_inbytes) *)(header_rawbytes + 0x02);
	header_extract_return->pixeldata_offset_inbytes = *(typeof(pixeldata_offset_inbytes) *)(header_rawbytes + 0x0A);
	header_extract_return->sizeof_header_inbytes = *(typeof(sizeof_header_inbytes) *)(header_rawbytes + 0x0E);
	header_extract_return->countof_width_inpixels = *(typeof(countof_width_inpixels) *)(header_rawbytes + 0x12);
	header_extract_return->countof_height_inpixels = *(typeof(countof_height_inpixels) *)(header_rawbytes + 0x16);
	header_extract_return->sizeof_pixel_inbits = *(typeof(sizeof_pixel_inbits) *)(header_rawbytes + 0x1C);
	header_extract_return->compression_method_number = *(typeof(compression_method_number) *)(header_rawbytes + 0x1E);
	
	if (header_extract_return->file_signature != FILE_SIGNATURE_INDICATING_BITMAP) {
		fprintf(stderr, "[ERROR] (parse_header_rawbytes_to_header_extract): file_signature[%d] != FILE_SIGNATURE_INDICATING_BITMAP[%d]\n", header_extract_return->file_signature, FILE_SIGNATURE_INDICATING_BITMAP);		
		return 1;
	}

	return 0;
}

int parse_bitmask_rawbytes_to_bitmask(bitmask_t *bitmask_return) {
	bitmask_return->red = *(unsigned int *)(bitmask_rawbytes + 0x00);
	bitmask_return->green = *(unsigned int *)(bitmask_rawbytes + 0x04);
	bitmask_return->blue = *(unsigned int *)(bitmask_rawbytes + 0x08);
	if (condition_has_rgba_bitmask) bitmask_return->alpha = *(unsigned int *)(bitmask_rawbytes + 0x12);
	return 0;
}

int flip_pixel_indices_row(char *pixeldata_rawbytes_return) {
	int sizeof_pixel_indices_inbytes = countof_width_inpixels;
	int pixel_indices[sizeof_pixel_indices_inbytes];
	generate_pixel_indices(pixel_indices, sizeof_pixel_indices_inbytes, sizeof_pixel_inbytes);
	int iteration_increment_value = countof_total_width_inbytes; // the 'increment' from row x to row x+1 is to add the total length of a row to each index
	int iteration_stop_value = countof_height_inpixels; // each row is flipped until the number of rows is reached, this number of rows being equal to the height
	copy_inversed_pixel_indices_to(pixeldata_rawbytes_return, pixel_indices, sizeof_pixel_indices_inbytes, iteration_increment_value, iteration_stop_value);
	return 0;
} 

int flip_pixel_indices_collumn(char *pixeldata_rawbytes_return) {
	int sizeof_pixel_indices_inbytes = countof_height_inpixels;
	int pixel_indices[sizeof_pixel_indices_inbytes];
	generate_pixel_indices(pixel_indices, sizeof_pixel_indices_inbytes, countof_total_width_inbytes);
	int iteration_increment_value = sizeof_pixel_inbytes; // the 'increment' from collumn x to collumn x+1 is to add the size of each pixel in bytes to get over to the next pixel column
	int iteration_stop_value = countof_width_inpixels; // each col is flipped until the number of cols is reached, this number of cols being equal to the width
	copy_inversed_pixel_indices_to(pixeldata_rawbytes_return, pixel_indices, sizeof_pixel_indices_inbytes, iteration_increment_value, iteration_stop_value);	
	return 0;
}

int generate_pixel_indices(int *pixel_indices, int sizeof_pixel_indices_inbytes, int pixel_index_increment_inbytes) {
	for (int next_pixel_index = 0; next_pixel_index < sizeof_pixel_indices_inbytes; next_pixel_index++) {
		int next_pixel_index_inbytes = next_pixel_index * pixel_index_increment_inbytes;
		pixel_indices[next_pixel_index] = next_pixel_index_inbytes;
	}
	return 0;
}

int copy_inversed_pixel_indices_to(char *pixeldata_copy, int *pixel_indices, int sizeof_pixel_indices_inbytes, int iteration_increment_value, int iteration_stop_value) {
	for (int iteration = 0; iteration < iteration_stop_value; iteration++) {
		int iteration_offset_value = iteration * iteration_increment_value;
		
		int indexof_front_pixel_indices = 0;
		int indexof_back_pixel_indices = sizeof_pixel_indices_inbytes - 1;

		while (indexof_front_pixel_indices <= indexof_back_pixel_indices) {
			int indexof_front_pixeldata = pixel_indices[indexof_front_pixel_indices] + iteration_offset_value;
			int indexof_back_pixeldata = pixel_indices[indexof_back_pixel_indices] + iteration_offset_value;
			
			for (int pixel_byte = 0; pixel_byte < sizeof_pixel_inbytes; pixel_byte++) {
				pixeldata_copy[indexof_front_pixeldata + pixel_byte] = pixeldata_rawbytes[indexof_back_pixeldata + pixel_byte];
				pixeldata_copy[indexof_back_pixeldata + pixel_byte] = pixeldata_rawbytes[indexof_front_pixeldata + pixel_byte];
			}

			indexof_front_pixel_indices++;
			indexof_back_pixel_indices--;
		}
	}
	return 0;
}

int scale_gray_colortable(char *colortable_rawbytes_return) {
	for (int colortable_entry = 0; colortable_entry < countof_colortable_entries; colortable_entry++) {
		int indexof_colortable_entry = colortable_entry * SIZEOF_COLORTABLE_ENTRY_INBYTES;
		scale_gray_element((unsigned char*)colortable_rawbytes + indexof_colortable_entry, (unsigned char*)colortable_rawbytes_return + indexof_colortable_entry, SIZEOF_COLORTABLE_ENTRY_INBYTES);		
	}
	return 0;
}

int scale_gray_pixeldata(char *pixeldata_rawbytes_return) {
	for (int row = 0; row < countof_height_inpixels; row++) {
		int row_offset_inbytes = row * countof_total_width_inbytes;
		for (int col = 0; col < countof_width_inpixels; col++) {
			int pixel_index = row_offset_inbytes + (col * sizeof_pixel_inbytes);
			scale_gray_element((unsigned char*)pixeldata_rawbytes + pixel_index, (unsigned char*)pixeldata_rawbytes_return + pixel_index, sizeof_pixel_inbytes);
		}
	}

	return 0;
}

int scale_gray_element(unsigned char* element_source_pntr, unsigned char* element_dest_pntr, int sizeof_element_inbytes) {
	unsigned int element_source_inbits = 0;
	for (int element_source_byte = 0; element_source_byte < sizeof_element_inbytes; element_source_byte++) {
		int element_source_byte_shift = element_source_byte * SIZEOF_BYTE_INBITS;
		element_source_inbits += element_source_pntr[element_source_byte] << element_source_byte_shift;
	}	
	
	unsigned int value_red = compact_value_from_bitmask(element_source_inbits, bitmask.red);
	unsigned int value_green = compact_value_from_bitmask(element_source_inbits, bitmask.green);
	unsigned int value_blue = compact_value_from_bitmask(element_source_inbits, bitmask.blue);

	double max_value_red = (1 << calculate_number_of_bits_in_bitmask(bitmask.red)) - 1;
	double max_value_green = (1 << calculate_number_of_bits_in_bitmask(bitmask.green)) - 1;
	double max_value_blue = (1 << calculate_number_of_bits_in_bitmask(bitmask.blue)) - 1;

	double normalized_value_red = value_red / max_value_red;
	double normalized_value_green = value_green / max_value_green;
	double normalized_value_blue = value_blue / max_value_blue;

	 double normalized_value_gray = (
		GRAYSCALE_FACTOR_RED * normalized_value_red + 
		GRAYSCALE_FACTOR_GREEN * normalized_value_green + 
		GRAYSCALE_FACTOR_BLUE * normalized_value_blue);
	
	unsigned int value_gray_red = normalized_value_gray * max_value_red;
	unsigned int value_gray_green = normalized_value_gray * max_value_green;
	unsigned int value_gray_blue = normalized_value_gray * max_value_blue;
	
	unsigned int element_scaled = element_source_inbits & bitmask.alpha; // if there exist alpha bits, those shuold be preserved

	element_scaled += expand_value_from_bitmask(value_gray_red, bitmask.red);
	element_scaled += expand_value_from_bitmask(value_gray_green, bitmask.green);
	element_scaled += expand_value_from_bitmask(value_gray_blue, bitmask.blue);
	
	for (int element_dest_byte = 0; element_dest_byte < sizeof_element_inbytes; element_dest_byte++) {
		int element_dest_byte_shift = element_dest_byte * SIZEOF_BYTE_INBITS;
		element_dest_pntr[element_dest_byte] = (unsigned char) (element_scaled >> element_dest_byte_shift);
	}	

	return 0;
}

int calculate_number_of_bits_in_bitmask(unsigned int bitmask) {
	int number_of_bits = 0;
	while (bitmask != 0) {
		number_of_bits += (bitmask & 1);
		bitmask = bitmask >> 1;
	}
	return number_of_bits;
}

int compact_value_from_bitmask(unsigned int element_inbits, unsigned int bitmask) {
	unsigned int value = 0;
	int element_shift = 0;

	while (bitmask != 0) {
		int condition_rightmost_bit_in_bitmask_is_1 = bitmask & 1;
		if (condition_rightmost_bit_in_bitmask_is_1) {
			unsigned int rightmost_bit_in_element = element_inbits & 1;
			unsigned int to_add = rightmost_bit_in_element << element_shift;
			value += to_add;
			element_shift++;
		}
		bitmask = bitmask >> 1;
		element_inbits = element_inbits >> 1;
	}
	return value;
}

int expand_value_from_bitmask (unsigned int value_inbits, unsigned int bitmask) {
	unsigned int element_component = 0;
	int value_shift = 0;

	while (bitmask != 0) {
		int condition_rightmost_bit_in_bitmask_is_1 = bitmask & 1;
		if (condition_rightmost_bit_in_bitmask_is_1) {
			unsigned int rightmost_bit_in_value = value_inbits & 1;
			unsigned int to_add = rightmost_bit_in_value << value_shift;
			element_component += to_add;
			value_inbits = value_inbits >> 1;
		}
		bitmask = bitmask >> 1;
		value_shift++;
	}
	return element_component;
}

int write_info_to_stdout() {
	printf("Pixel width: %d\n", countof_width_inpixels);
	printf("Pixel height: %d\n", countof_height_inpixels);
	printf("Bit depth: %d\n", sizeof_pixel_inbits);
	printf("Row width (bytes): %d\n", countof_total_width_inbytes);
	printf("Bytes per pixel: %d\n", sizeof_pixel_inbytes);
	printf("Data offset: %d\n", pixeldata_offset_inbytes);
	return 0;
}

int write_bmp_to_stdout() {
	int countof_errors = 0;
	countof_errors += write_to_stdout(header_rawbytes, SIZEOF_HEADER_RAWBYTES_INBYTES);
	if (condition_has_bitmask) countof_errors += write_to_stdout(bitmask_rawbytes, sizeof_bitmask_rawbytes_inbytes);
	if (condition_has_colortable) countof_errors += write_to_stdout(colortable_rawbytes, sizeof_colortable_rawbytes_inbytes);
	countof_errors += write_to_stdout(pixeldata_rawbytes, sizeof_pixeldata_rawbytes_inbytes);	
	return countof_errors;
} 

int write_to_stdout(char *rawbytes, int countof_items_to_write_inbytes) {

	int countof_items_written_inbytes = fwrite(rawbytes, 1, countof_items_to_write_inbytes, stdout);
	if (countof_items_written_inbytes != countof_items_to_write_inbytes) {
		fprintf(stderr, "[ERROR] (write_to_stdout): countof_items_to_write_inbytes=%d, countof_items_written_inbytes=%d\n", countof_items_to_write_inbytes, countof_items_written_inbytes);
		return 1;
	} 
	if (ferror(stdin)) {
		fprintf(stderr, "[ERROR] (write_to_stdout): error occured while reading from stdin");
		return 1;
	}
	return 0; 
	
}

char* intToBinaryString(unsigned int num) {
    // Calculate the number of bits needed
    int numBits = sizeof(unsigned int) * 8; // Assuming 8 bits per byte

    // Allocate memory for the binary string (including null terminator)
    char* binaryString = (char*)malloc(numBits + 1);

    if (binaryString == NULL) {
        // Memory allocation failed
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Start from the most significant bit
    for (int i = numBits - 1; i >= 0; i--) {
        // Use bitwise AND to check the value of the current bit
        if (num & (1u << i)) {
            binaryString[numBits - 1 - i] = '1';
        } else {
            binaryString[numBits - 1 - i] = '0';
        }
    }

    // Add null terminator
    binaryString[numBits] = '\0';

    return binaryString;
}
