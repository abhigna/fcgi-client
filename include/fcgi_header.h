#ifndef FCGI_HEADER_H
#define FCGI_HEADER_H

#include "fcgi_defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* Bytes from LSB to MSB 0..3 */
#define BYTE_0(x) ((x) & 0xff)
#define BYTE_1(x) ((x)>>8 & 0xff)
#define BYTE_2(x) ((x)>>16 & 0xff)
#define BYTE_3(x) ((x)>>24 & 0xff)

typedef unsigned char uchar;

fcgi_header* create_header(unsigned char type,uint16_t request_id);

fcgi_begin_request* create_begin_request(uint16_t request_id);

void serialize(uchar* buffer, void *st, size_t size);


typedef struct node_{
    void *data;
    struct node_* next;
} Node;

Node* create_node(void *data);

void prepend_to_list(Node* head, Node* n);
uint32_t serialize_name_value(uchar* buffer, fcgi_name_value* nv);
void print_bytes(uchar *buf, int n);

#define PRINT_OPAQUE_STRUCT(p)  print_mem((p), sizeof(*(p)))
void print_mem(void const *vp, size_t n);

#endif
