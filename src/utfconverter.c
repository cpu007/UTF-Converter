#include "utfconverter.h"
#include "struct.txt" 

char* filename = 0, *output_file = 0, *file_path = 0;
char host_name[256];
endianness source;
endianness conversion; 
int output_fd = -1, verbosity = 0;
clock_t real_start, real_end, user_start, user_end, sys_start, sys_end;
struct stat file_stats;
long double file_size = 0;
bool utf8_to_utf16 = false;
struct utsname os_info;
struct tms tms_buf;
long int TICKS;
long double ascii_count = 0, surrogate_count = 0, glyph_count = 0;

struct Time_Meas{
	long double read_time_count, convert_time_count, write_time_count;
} real_time, user_time, sys_time;


#define CALC_TIME(expression,time_count) times(&tms_buf);\
			user_start = tms_buf.tms_utime;\
			sys_start = tms_buf.tms_stime;\
			real_start = clock();\
			expression\
			times(&tms_buf);\
			user_end = tms_buf.tms_utime;\
			sys_end = tms_buf.tms_stime;\
			real_end = clock();\
			user_time.time_count += user_end-user_start;\
			sys_time.time_count += sys_end-sys_start;\
			real_time.time_count += real_end-real_start;\



void 
print_verbose_level_1_menu(encoding)
char* encoding;{
	file_path = malloc(PATH_MAX);
	file_path = realpath(filename,file_path);
	if(verbosity){
		printf("\tInput file size: %Lf kb\n",file_size);
		printf("\tInput file path: ");
		printf("%s\n", file_path);
		printf("\tInput file encoding: %s\n", encoding);
		printf("\tOutput encoding: %s\n", (conversion==BIG)?"UTF-16BE":"UTF-16LE");
		printf("\tHostmachine: %s\n", host_name);
		printf("\tOperating System: ");
		printf("%s\n",os_info.sysname);		
	}
	
}

void print_verbose_level_2_menu(){
	if(verbosity == 2){
		real_time.read_time_count /= CLOCKS_PER_SEC;
		real_time.convert_time_count /= CLOCKS_PER_SEC;
		real_time.write_time_count /= CLOCKS_PER_SEC;
		user_time.read_time_count /= TICKS;
		user_time.convert_time_count /= TICKS;
		user_time.write_time_count /= TICKS;
		sys_time.read_time_count /= TICKS;
		sys_time.convert_time_count /= TICKS;
		sys_time.write_time_count /= TICKS;
		printf("\tReading: real=%Lf, user=%Lf, sys=%Lf\n",real_time.read_time_count, user_time.read_time_count, sys_time.read_time_count);
		printf("\tConverting: real=%Lf, user=%Lf, sys=%Lf\n",real_time.convert_time_count, user_time.convert_time_count, sys_time.convert_time_count);
		printf("\tWriting: real=%Lf, user=%Lf, sys=%Lf\n",real_time.write_time_count, user_time.write_time_count, sys_time.write_time_count);
		printf("\tASCII: %Lf%%\n", (ascii_count/glyph_count)*100.);
		printf("\tSurrogates: %Lf%%\n", (surrogate_count/glyph_count)*100.);
		printf("\tGlyphs: %ld\n", (long int)glyph_count);
	}
	
}

int 
main(argc,argv)
int argc;
char** argv;
{
	/* After calling parse_args(), filename and conversion should be set. */
	int fd, rv = 0; 
	unsigned int buf[3] = {0,0,0}, bom_buf[1]; 
	void* memset_return;
	Glyph* glyph; 
	char* encoding_name;
	TICKS = sysconf(_SC_CLK_TCK);
	real_time.read_time_count = 0;
	real_time.convert_time_count = 0;
	real_time.write_time_count = 0;
	user_time.read_time_count = 0;
	user_time.convert_time_count = 0;
	user_time.write_time_count = 0;
	sys_time.read_time_count = 0;
	sys_time.convert_time_count = 0;
	sys_time.write_time_count = 0;
	parse_args(argc, argv);
	fd = open(filename, O_RDONLY);
	if(fd == -1) print_help(NO_FD,EXIT_FAILURE);
	stat(filename, &file_stats);
	gethostname(host_name,256);
	uname(&os_info);
	file_size = file_stats.st_size/1000.;
	glyph = malloc(sizeof(Glyph)); 
    #if (ASM == 1)
		CALC_TIME(if((rv = read(fd, buf, 1)) == 1 && (rv = read(fd, buf+1, 1)) == 1){,read_time_count);
			CALC_TIME(if(buf[0] == 0xff000000 && buf[1] == 0xfe000000) {
				source = LITTLE; /*file is little endian*/
				encoding_name = "UTF-16LE";
			}
			else if(buf[0] == 0xfe000000 && buf[1] == 0xff000000) {
				source = BIG; /*file is big endian*/
				encoding_name = "UTF-16BE";
			}
			else if(read(fd, buf+2, 1) == 1){
			,read_time_count);
				if(buf[0] == 0xEF000000 && buf[1] == 0xBB000000 && buf[2] == 0xBF000000){
					utf8_to_utf16 = true;
					encoding_name = "UTF-8";
				}
			}
			else {
				/*file has no BOM*/
				free(glyph);
				fprintf(stderr, "File has no BOM.\n");
				print_help(fd,EXIT_FAILURE);
			}
			memset_return = memset(glyph, 0, sizeof(Glyph)+1);
			/* Memory write failed, fail quickly from it: */
			if(memset_return == NULL) print_help(fd,EXIT_FAILURE);
		}
		print_verbose_level_1_menu(encoding_name);
		switch(conversion){
			case LITTLE: bom_buf[0] = 0xFFFE0000; break;
			case BIG: bom_buf[0] = 0xFEFF0000; break;
		}
		CALC_TIME(
			if(output_fd == -1) write(STDOUT_FILENO, bom_buf, 2);
			else write(output_fd, bom_buf, 2);,
			write_time_count
		);

		/* Now deal with the rest of the bytes.*/
		CALC_TIME(while((rv = read(fd, buf, 1)) == 1){,read_time_count);
			CALC_TIME(if(utf8_to_utf16 == false && (rv = read(fd, buf+1, 1)) == 1){,read_time_count);
				if(conversion != source) write_glyph(swap_endianness(my_fill_glyph(glyph, buf, source, &fd)));
				else write_glyph(my_fill_glyph(glyph, buf, source, &fd));
			}
			else write_glyph(convert(utf8_my_fill_glyph(glyph,*buf,source,&fd),conversion));
			memset_return = memset(glyph, 0, sizeof(Glyph)+1);
				/* Memory write failed, fail quickly from it: */
				if(memset_return == NULL) print_help(fd,EXIT_FAILURE);
		}
	#else 
	CALC_TIME(if((rv = read(fd, buf, 1)) == 1 && (rv = read(fd, buf+1, 1)) == 1){,read_time_count);
			CALC_TIME(
			if(buf[0] == 0xff && buf[1] == 0xfe) {
				source = LITTLE; /*file is little endian*/
				encoding_name = "UTF-16LE";
			}
			else if(buf[0] == 0xfe && buf[1] == 0xff) {
				source = BIG; /*file is big endian*/
				encoding_name = "UTF-16BE";
			}
			else if(read(fd, buf+2, 1) == 1){
			,read_time_count);
				if(buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF){
					utf8_to_utf16 = true;
					encoding_name = "UTF-8";
				}
			}
			else {
				/*file has no BOM*/
				free(glyph);
				fprintf(stderr, "File has no BOM.\n");
				print_help(fd,EXIT_FAILURE);
			}
			memset_return = memset(glyph, 0, sizeof(Glyph)+1);
			/* Memory write failed, recover from it: */
			if(memset_return == NULL){
				/* tweak write permission on heap memory. */
				asm("movl $8, %esi\n\t"
					"movl $.LC0, %edi\n\t"
					"movl $0, %eax");
				/* Now make the request again. */
				memset(glyph, 0, sizeof(Glyph)+1);
			}
	}
	print_verbose_level_1_menu(encoding_name);
	switch(conversion){
		case LITTLE: bom_buf[0] = 0xFEFF; break;
		case BIG: bom_buf[0] = 0xFFFE; break;
	}
	CALC_TIME(
		if(output_fd == -1) write(STDOUT_FILENO, bom_buf, 2);
		else write(output_fd, bom_buf, 2);
	,write_time_count);

	/* Now deal with the rest of the bytes.*/
	CALC_TIME(while((rv = read(fd, buf, 1)) == 1){,read_time_count);
		CALC_TIME(if(utf8_to_utf16 == false && (rv = read(fd, buf+1, 1)) == 1){,read_time_count);
			if(conversion != source) write_glyph(swap_endianness(my_fill_glyph(glyph, buf, source, &fd)));
			else write_glyph(my_fill_glyph(glyph, buf, source, &fd));
		}
		else write_glyph(convert(utf8_my_fill_glyph(glyph,*buf,source,&fd),conversion));
		memset_return = memset(glyph, 0, sizeof(Glyph)+1);
	        /* Memory write failed, recover from it: */
	        if(memset_return == NULL){
		        /* tweak write permission on heap memory. */
				asm("movl $8, %esi\n\t"
					"movl $.LC0, %edi\n\t"
					"movl $0, %eax");
				/* Now make the request again. */
		        memset(glyph, 0, sizeof(Glyph)+1);
	        }
	}
	#endif
	print_verbose_level_2_menu();
	free(glyph);
	quit_converter(fd,EXIT_SUCCESS);
	return 0;
}

Glyph* swap_endianness(Glyph* glyph) 
{
	/* Use XOR to be more efficient with how we swap values. */
	CALC_TIME(
		glyph->bytes[0] ^= glyph->bytes[1];
		glyph->bytes[1] ^= glyph->bytes[0];
		glyph->bytes[0] ^= glyph->bytes[1];
		if(glyph->surrogate){  /* If a surrogate pair, swap the next two bytes. */
			glyph->bytes[2] ^= glyph->bytes[3];
			glyph->bytes[3] ^= glyph->bytes[2];
			glyph->bytes[2] ^= glyph->bytes[3];
		}
		glyph->end = conversion;
	, convert_time_count);
	return glyph;
}

Glyph* 
my_fill_glyph(glyph,data,end,fd) 
Glyph* glyph;
unsigned int data[2];
endianness end;
int* fd;{
#if (ASM == 1)
	unsigned int halfwordBE = ((data[1]&0xff000000)>>8)|(data[0]&0xff000000), halfwordLE = ((data[0]&0xff000000)>>8)|(data[1]&0xff000000);
	glyph->bytes[0] = data[0]>>24;
	glyph->bytes[1] = data[1]>>24;
	if(glyph->bytes[1] == 0) ++ascii_count;
	if(halfwordLE >= 0xD8000000 && halfwordLE <= 0xDBFF0000){
		CALC_TIME(if(read(*fd, data, 1) == 1 && read(*fd, data+1, 1) == 1){,read_time_count);
		CALC_TIME(
			halfwordLE = ((data[0]&0xff000000)>>8)|(data[1]&0xff000000);
			if((halfwordLE >= 0xDC000000 && halfwordLE <= 0xDFFF0000)){
				glyph->bytes[2] = data[0]>>24;
				glyph->bytes[3] = data[1]>>24;
				glyph->surrogate = true;
				++surrogate_count;
			}
			else {
				glyph->surrogate = false;
				lseek(*fd, -OFFSET, SEEK_CUR);
			},convert_time_count);
		}
	}
	else if(halfwordBE >= 0xD8000000 && halfwordBE <= 0xDBFF0000){
		CALC_TIME(if(read(*fd, data, 1) == 1 && read(*fd, data+1, 1) == 1){,read_time_count);
			CALC_TIME(
				halfwordBE = ((data[1]&0xff000000)>>8)|(data[0]&0xff000000);
				if((halfwordBE >= 0xDC000000 && halfwordBE <= 0xDFFF0000)){
					glyph->bytes[2] = data[0]>>24;
					glyph->bytes[3] = data[1]>>24;
					glyph->surrogate = true;
					++surrogate_count;
				}
				else {
					glyph->surrogate = false;
					lseek(*fd, -OFFSET, SEEK_CUR);
				},convert_time_count);
		}
	}
	else{
		glyph->surrogate = false;
		glyph->bytes[2] = glyph->bytes[3] = 0;
	}
	++glyph_count;
	glyph->end = end;
	return glyph;
#else 
	unsigned int halfwordBE = (data[1]&0xff)|((data[0]&0xff)<<8), halfwordLE = (data[0]&0xff)|((data[1]&0xff)<<8);
	glyph->bytes[0] = data[0];
	glyph->bytes[1] = data[1];
	if(glyph->bytes[1] == 0) ++ascii_count;
	if(source == LITTLE){
		if(halfwordLE >= 0xD800 && halfwordLE <= 0xDBFF){
			CALC_TIME(if(read(*fd, data, 1) == 1 && read(*fd, data+1, 1) == 1){, read_time_count);
				CALC_TIME(
					halfwordLE = (data[0]&0xff)|((data[1]&0xff)<<8);
					if((halfwordLE >= 0xDC00 && halfwordLE <= 0xDFFF)){
						glyph->bytes[2] = data[0];
						glyph->bytes[3] = data[1];
						glyph->surrogate = true;
						++surrogate_count;
					}
					else {
						glyph->surrogate = false;
						lseek(*fd, -OFFSET, SEEK_CUR);
					},convert_time_count);
			}
		}
		else{
			glyph->surrogate = false;
			glyph->bytes[2] = glyph->bytes[3] = 0;
		}
	}
	else {
		if(halfwordBE >= 0xD800 && halfwordBE <= 0xDBFF){
			CALC_TIME(if(read(*fd, data, 1) == 1 && read(*fd, data+1, 1) == 1){,read_time_count);
				CALC_TIME(
					halfwordBE = (data[1]&0xff)|((data[0]&0xff)<<8);
					if((halfwordBE >= 0xDC00 && halfwordBE <= 0xDFFF)){
						glyph->bytes[2] = data[0];
						glyph->bytes[3] = data[1];
						glyph->surrogate = true;
						++surrogate_count;
					}
					else {
						glyph->surrogate = false;
						lseek(*fd, -OFFSET, SEEK_CUR);
					},convert_time_count);
			}
		}
		else{
			glyph->surrogate = false;
			glyph->bytes[2] = glyph->bytes[3] = 0;
		}
	}
	glyph->end = end;
	++glyph_count;
	return glyph;
#endif
}

Glyph* 
utf8_my_fill_glyph(glyph,data,end,fd)
Glyph* glyph;
unsigned int data;
endianness end;
int* fd;{
	int i, mask, hex_digit, num_bytes;
	#if (ASM == 1)
	CALC_TIME(
		if(data&0x80000000){
			for(i = 0; i < 3; i++){
				switch (i){
					case 0: mask = 0xc0000000; break;
					case 1: mask = 0xe0000000; break;
					case 2: mask = 0xf0000000; break;
				}
				hex_digit = data&mask;
				if(hex_digit != mask) break;
			}
		}
		switch(hex_digit){
			case 0xc0000000: num_bytes = 2; break;
			case 0xe0000000: num_bytes = 3; break;
			case 0xf0000000: num_bytes = 4; break;
			default: num_bytes = 1;
		}
		glyph->bytes[0] = glyph->bytes[1] = glyph->bytes[2] = glyph->bytes[3] = 0;
		glyph->bytes[0] = data>>24;
		i = 0;,
		convert_time_count);
	while(num_bytes-->1){
		CALC_TIME(if(read(*fd, &data, 1) == 1){, read_time_count);
			glyph->bytes[++i] = data>>24;
		}
	}
	#else
	CALC_TIME(
		if(data&0x80){
			for(i = 0; i < 3; i++){
				switch (i){
					case 0: mask = 0xc0; break;
					case 1: mask = 0xe0; break;
					case 2: mask = 0xf0; break;
				}
				hex_digit = data&mask;
				if(hex_digit != mask) break;
			}
		}
		switch(hex_digit){
			case 0xc0: num_bytes = 2; break;
			case 0xe0: num_bytes = 3; break;
			case 0xf0: num_bytes = 4; break;
			default: num_bytes = 1;
		}
		glyph->bytes[0] = glyph->bytes[1] = glyph->bytes[2] = glyph->bytes[3] = 0;
		glyph->bytes[0] = data;
		i = 0;,
		convert_time_count);
	while(num_bytes-->1){
		CALC_TIME(if(read(*fd, &data, 1) == 1){, read_time_count);
			glyph->bytes[++i] = data;
		}
	}
	#endif
	glyph->end = end;
	return glyph;
}

Glyph* 
convert(glyph,end)
Glyph* glyph;
endianness end;{
	#if (ASM == 0)
	int hex_digit = 0, num_bytes, i, mask;
	#else
	int num_bytes, i;
	unsigned char hex_digit = 0, mask;
	#endif
	unsigned int code_point = 0, high_surrogate, low_surrogate;
	glyph->end = end;
	CALC_TIME(
	if(glyph->bytes[0]&0x80){
		for(i = 0; i < 3; i++){
			switch (i){
				case 0: mask = 0xc0; break;
				case 1: mask = 0xe0; break;
				case 2: mask = 0xf0; break;
			}
			hex_digit = glyph->bytes[0]&mask;
			if(hex_digit != mask) break;
		}
	}
	switch(hex_digit){
		case 0xc0: num_bytes = 2; break;
		case 0xe0: num_bytes = 3; break;
		case 0xf0: num_bytes = 4; break;
		default: num_bytes = 1;
	}
	switch(num_bytes){
		case 1: code_point = glyph->bytes[0]; ++ascii_count; break;
		case 2: code_point = ((glyph->bytes[0]&0x1f)<<6)|(glyph->bytes[1]&0x3f); break;
		case 3: code_point = ((glyph->bytes[0]&0xf)<<12)|((glyph->bytes[1]&0x3f)<<6)|(glyph->bytes[2]&0x3f);break;
		case 4: code_point = ((glyph->bytes[0]&0x7)<<18)|((glyph->bytes[1]&0x3f)<<12)|((glyph->bytes[2]&0x3f)<<6)|(glyph->bytes[3]&0x3f);break;		
	}
	
	if(0x10000 <= code_point && code_point <= 0x10FFFF){
		++surrogate_count;
		glyph->surrogate = true;
		code_point -= 0x10000;
		low_surrogate = 0x3ff;
		high_surrogate = low_surrogate<<10;
		low_surrogate &= code_point;
		high_surrogate &= code_point;
		high_surrogate >>= 10;
		high_surrogate += 0xD800;
		low_surrogate += 0xDC00;
		if(end == BIG){
			glyph->bytes[1] = high_surrogate&0xFF;
			glyph->bytes[0] = (high_surrogate&0xFF00)>>8;
			glyph->bytes[3] = (low_surrogate&0xFF);
			glyph->bytes[2] = (low_surrogate&0xFF00)>>8;
		}
		else{
			glyph->bytes[0] = high_surrogate&0xFF;
			glyph->bytes[1] = (high_surrogate&0xFF00)>>8;
			glyph->bytes[2] = (low_surrogate&0xFF);
			glyph->bytes[3] = (low_surrogate&0xFF00)>>8;
		}
	}
	else{
		glyph->surrogate = false;
		if(end == BIG){
			glyph->bytes[1] = code_point&0xFF;
			glyph->bytes[0] = (code_point&0xFF00)>>8;
		}
		else{
			glyph->bytes[0] = code_point&0xFF;
			glyph->bytes[1] = (code_point&0xFF00)>>8;
		}
		glyph->bytes[2] = glyph->bytes[3] = 0;
	},convert_time_count);
	++glyph_count;
	return glyph;
}


void 
write_glyph(glyph)
Glyph* glyph; {
	CALC_TIME(
	if(output_fd == -1){
		if(glyph->surrogate) write(STDOUT_FILENO, glyph->bytes, SURROGATE_SIZE);
		else write(STDOUT_FILENO, glyph->bytes, NON_SURROGATE_SIZE);
	}
	else{
		if(glyph->surrogate) write(output_fd, glyph->bytes, SURROGATE_SIZE);
		else write(output_fd, glyph->bytes, NON_SURROGATE_SIZE);
	},write_time_count);
}

void remove_v(s)
char* s;{
  	while((s=strstr(s,"v"))) memmove(s,s+1,1+strlen(s+1));
}

int remove_lone_flag(char* flag){
	int rem_count = 0;
	bool result = true;
	char *orig_flag = flag;
	while (*flag) {
		if(*flag == '-' || *flag == 'v') {
			flag++;
			continue;
		}
		else{
			result = false;
			break;
		}
	}
	flag = orig_flag;
	if(result){
		remove_v(flag);
		if(flag[1] == '\0') flag[0] = ' ';
		rem_count++;
	}
	else{
		while (*flag) {
			if(*flag == 'v') *flag = ' ';
			flag++;
		}
	}
	return rem_count;
}



int parse_verbose_level(argc, argv)
int argc;
char** argv;{
	int option_index, c, rem_ops = 0;	
	while((c = getopt_long(argc, argv, "vuh", long_options, &option_index)) != -1){
		switch(c){ 
			case 'h': 
				print_help(NO_FD,EXIT_SUCCESS);
				break;
			case 'v':
				if(strcmp(argv[optind-1], "-v") == 0) {
					if(verbosity == 0) verbosity = 1;
					else verbosity = 2;
				}
				else {
					verbosity = 2;
				}
				rem_ops += remove_lone_flag(argv[optind-1]);
				break;
			default: break;
		}
	}
	return rem_ops;
}

bool 
is_blank(str)
char* str;{
	bool blank = true;
	while(*str){
		if(*str != ' ') {
			blank = false;
			break;
		}
		str++;
	}
	return blank;
}

void 
remove_blanks(argc, argv)
int argc;
char** argv;{
	while(argc --> 0){
		if(is_blank(*argv)) {
			memmove(argv,argv+1,argc*sizeof(*argv));
		}
		else argv++;
	}
}

void 
parse_args(argc, argv)
int argc;
char** argv;{
	int option_index, c, temp;
	char* endian_convert = NULL;
	temp = parse_verbose_level(argc, argv);
	remove_blanks(argc,argv);
	argc = argc-temp;
	optind = 1;
	if((c = getopt_long(argc, argv, "huv", long_options, &option_index)) != -1){
		switch(c){ 
			case 'h':
				print_help(NO_FD,EXIT_SUCCESS);
				break;
			case 'u':
				endian_convert = argv[optind];
				break;
			case 'U':
				endian_convert = optarg;
				--optind;
			case 'v':
				break;
			default:
				fprintf(stderr, "ERROR: Unrecognized argument.\n");
				print_help(NO_FD,EXIT_FAILURE);
				break;
		}
	}
	if(optind+2 < argc) {
		output_file = strdup(argv[optind+2]);
		output_fd = open(
			output_file,
			O_CREAT|O_WRONLY|O_APPEND,
			S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH
		);
	}
	if(optind+1 < argc) filename = strdup(argv[optind+1]);
	else {
		fprintf(stderr, "Filename not given.\n");
		print_help(NO_FD,EXIT_FAILURE);
	}
	if(endian_convert == NULL){
		fprintf(stderr, "Converson mode not given.\n");
		print_help(NO_FD,EXIT_FAILURE);
	}

	if(strcmp(endian_convert, "16LE") == 0) conversion = LITTLE;
	else if(strcmp(endian_convert, "16BE") == 0) conversion = BIG;
	else {
		printf("Invalid Conversion Scheme\n");
		print_help(NO_FD,EXIT_FAILURE);
	}
}

void print_help(fd,exit_code) 
int fd;
int exit_code;{
	int i;
	for(i = 0; i < USAGE_LENGTH; i++) printf("%s", USAGE[i]);
	quit_converter(fd,exit_code);
}

void 
quit_converter(fd, exit_code)
int fd;
int exit_code;{
	close(STDERR_FILENO);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	if(fd != NO_FD) close(fd);
	if(filename) free(filename);
	if(output_file) free(output_file);
	if(file_path) free(file_path);
	if(output_fd != -1) close(output_fd);
	exit(exit_code);
}
