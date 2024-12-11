#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>

typedef char i8;
typedef unsigned char u8;
typedef unsigned short u16;
typedef int i32;
typedef unsigned u32;
typedef unsigned long u64;

#define PRINT_ERROR(cstring) write(STDERR_FILENO, cstring, sizeof(cstring) - 1)

#pragma pack(1)

#define NUM_THREADS 8

typedef struct bmp_header
{
	// Note: header
	i8  signature[2]; // should equal to "BM"
	u32 file_size;
	u32 unused_0;
	u32 data_offset;

	// Note: info header
	u32 info_header_size;
	u32 width; // in px
	u32 height; // in px
	u16 number_of_planes; // should be 1
	u16 bit_per_pixel; // 1, 4, 8, 16, 24 or 32
	u32 compression_type; // should be 0
	u32 compressed_image_size; // should be 0
	// Note: there are more stuff there but it is not important here
}	t_header;

typedef struct file_content
{
	i8*   data;
	u32   size;
} t_content;

typedef struct s_position{
	u32 row;
	u32 col;
} t_position;

typedef struct {
    t_content* content;
    t_header* header;
    u32 start_row;
    u32 end_row;
    t_position result;
    int found;
} 	t_thread_data;


struct file_content   read_entire_file(char* filename)
{
	char* file_data = 0;
	unsigned long	file_size = 0;
	int input_file_fd = open(filename, O_RDONLY);
	if (input_file_fd >= 0)
	{
		struct stat input_file_stat = {0};
		stat(filename, &input_file_stat);
		file_size = input_file_stat.st_size;
		file_data = mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, input_file_fd, 0);
		close(input_file_fd);
	}
	return (struct file_content){file_data, file_size};
}

inline static int is_possible_header(u32 row, u32 col, t_header* header)
{
	if (row + 7 < header->height && col + 7 < header->width)
		return 1;
	return 0;
}

inline static i8 *position_to_pointer(t_content* content, t_position* position)
{
	t_header *header = (t_header*)content->data;
	return content->data + header->data_offset +
		(( header->height - ( position->row + 1 )) * header->width + position->col) * header->bit_per_pixel / 8;
}

int confirm_header( t_content* content, i8* data_address)
{
	i8* curr_address;
	t_header *header = (t_header*)content->data;
	u8 channel_R, channel_G, channel_B;
	for (u32 i = 0; i < 7; i++)
	{
		curr_address = data_address + ( i * header->bit_per_pixel / 8 );
		channel_B = *data_address;
		channel_G = *(data_address + 1);
		channel_R = *(data_address + 2);
		if (channel_B != 127 || channel_G != 188 || channel_R != 217)
			return 0;
	}
	for (u32 i = 0; i < 8; i++)
	{
		curr_address = data_address - ( i * header->width * header->bit_per_pixel / 8 );
		channel_B = *data_address;
		channel_G = *(data_address + 1);
		channel_R = *(data_address + 2);
		if (channel_B != 127 || channel_G != 188 || channel_R != 217)
			return 0;
	}
	return 1;
}

void* search_header(void* arg) {
    t_thread_data* data = (t_thread_data*)arg;
    t_content* content = data->content;
    t_header* header = data->header;
    i8* data_address;
    u8 channel_R, channel_G, channel_B;

    for (u32 row = data->start_row; row < data->end_row; row++) {
        for (u32 col = 0; col < header->width; col++) {
            data_address = position_to_pointer(content, &(t_position){row, col});
            channel_B = *data_address;
            channel_G = *(data_address + 1);
            channel_R = *(data_address + 2);
            if (channel_B == 127 && channel_G == 188 && channel_R == 217 && is_possible_header(row, col, header)) {
                if (confirm_header(content, data_address)) {
                    data->result = (t_position){row, col};
                    data->found = 1;
                    return NULL;
                }
            }
        }
    }
    data->found = 0;
    return NULL;
}

t_position traverse_file(t_content* content) {
    t_header* header = (t_header*)content->data;
    pthread_t threads[NUM_THREADS];
    t_thread_data thread_data[NUM_THREADS];
    u32 rows_per_thread = header->height / NUM_THREADS;
    t_position result = {header->height, header->width};

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].content = content;
        thread_data[i].header = header;
        thread_data[i].start_row = i * rows_per_thread;
        thread_data[i].end_row = (i == NUM_THREADS - 1) ? header->height : (i + 1) * rows_per_thread;
        pthread_create(&threads[i], NULL, search_header, &thread_data[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        if (thread_data[i].found) {
            result = thread_data[i].result;
            break;
        }
    }
    return result;
}

u32	get_msg_len(t_content* content, t_position* position)
{
	i8 *size_address = position_to_pointer(content, &(t_position){position->row, position->col + 7});
	u8 b_len = *size_address;
	u8 r_len = *(size_address + 2);
	return  (u32) b_len + (u32)r_len;
}

void read_message(t_content* content, t_position* position, u16 msg_len)
{
	i8 *msg_address = position_to_pointer(content, &(t_position){position->row + 2, position->col + 2});
	i8 where_am_i = 1;
	while (msg_len--)
	{
		write(STDOUT_FILENO, msg_address, 1);
		msg_address++;
		where_am_i++;
		if (where_am_i == 24)
		{
			position->row += 1;
			msg_address = position_to_pointer(content, &(t_position){position->row + 2, position->col + 2});
			where_am_i = 1;
		}
		else if (where_am_i % 4 == 0)
		{
			msg_address++;
			where_am_i++;
		}
	}
}


int main(int argc, char** argv)
{
	t_position	position;
	u32			msg_len;

	if (argc != 2)
	{
		PRINT_ERROR("Usage: decode <input_filename>\n");
		return 1;
	}
	struct file_content file_content = read_entire_file(argv[1]);
	if (file_content.data == NULL)
	{
		PRINT_ERROR("Failed to read file\n");
		return 1;
	}
	position = traverse_file(&file_content);
	msg_len = get_msg_len(&file_content, &position);
	read_message(&file_content, &position, msg_len);
	return 0;
}
