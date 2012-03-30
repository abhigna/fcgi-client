#ifndef FCGI_HEADER_H
#define FCGI_HEADER_H
#include "fcgi_defs.h"

/* Bytes from LSB to MSB 0..3 */
#define BYTE_0(x) ((x) & 0xff)
#define BYTE_1(x) ((x)>>8 & 0xff)
#define BYTE_2(x) ((x)>>16 & 0xff)
#define BYTE_3(x) ((x)>>24 & 0xff)

typedef unsigned char uchar;
typedef enum{
    fcgi_state_version = 0,
    fcgi_state_type,
    fcgi_state_request_id_hi,
    fcgi_state_request_id_lo,
    fcgi_state_content_len_hi,
    fcgi_state_content_len_lo,
    fcgi_state_padding_len,
    fcgi_state_reserved,
    fcgi_state_content_begin,
    fcgi_state_content_proc,
    fcgi_state_padding,
    fcgi_state_done
} fcgi_state;

typedef struct fcgi_record_{
    fcgi_header* header;
    void *content;
    size_t offset, length;
    fcgi_state state;
    struct fcgi_record_* next;
} fcgi_record;

typedef fcgi_record fcgi_record_list;
fcgi_record* fcgi_record_create();

#define FCGI_PROCESS_AGAIN 1
#define FCGI_PROCESS_DONE 2
#define FCGI_PROCESS_ERR 3

fcgi_header* create_header(unsigned char type,uint16_t request_id);
fcgi_begin_request* create_begin_request(uint16_t request_id);
void serialize(uchar* buffer, void *st, size_t size);

uint32_t serialize_name_value(uchar* buffer, fcgi_name_value* nv);
void print_bytes(uchar *buf, int n);

#define PRINT_OPAQUE_STRUCT(p)  print_mem((p), sizeof(*(p)))
void print_mem(void const *vp, size_t n);

void fcgi_process_buffer(uchar *beg_buf,uchar *end_buf,
        fcgi_record_list** head);


#endif
