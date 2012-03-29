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
#define PRINT_OPAQUE_STRUCT(p)  print_mem((p), sizeof(*(p)))

void print_mem(void const *vp, size_t n)
{
    unsigned char const *p = vp;
    size_t i;
    for (i=0; i<n; i++)
        printf("%02x\n", p[i]);
    putchar('\n');
};

typedef unsigned char uchar;

fcgi_header* create_header(uchar type, uint16_t request_id){
    fcgi_header* tmp = (fcgi_header *) calloc(1,sizeof(fcgi_header));
    tmp->version = FCGI_VERSION_1;
    tmp->type    = type;
    tmp->request_id_lo = BYTE_0(request_id);
    tmp->request_id_hi = BYTE_1(request_id);
    return tmp;
}

fcgi_begin_request* create_begin_request(uint16_t request_id){
    fcgi_begin_request *h = (fcgi_begin_request *) malloc(sizeof(fcgi_begin_request));
    h->header = create_header(FCGI_BEGIN_REQUEST, request_id);
    h->body   = calloc(1, sizeof(fcgi_begin_request_body));
    h->body->role_lo = FCGI_RESPONDER;
    h->header->content_len_lo = sizeof(fcgi_begin_request_body);
    return h;
}

void serialize(uchar* buffer, void *st, size_t size){

    /*unsigned char const *p = st;*/
    /*size_t i;*/
    /*for(i=0; i < size; i++){*/
        /**buffer++ = *p++;*/
        /*[>printf("|%02X|", *(p-1));<]*/
    /*}*/
        
   memcpy(buffer, st, size);
}


typedef struct node_{
    void *data;
    struct node_* next;
} Node;

Node* create_node(void *data){
    Node *n = (Node *) malloc( sizeof(Node));
    n->data = data;
    n->next = NULL;
    return n;
}

void prepend_to_list(Node* head, Node* n){
    Node *tmp = head;
    head = n;
    head->next = tmp;
}


uint32_t serialize_name_value(uchar* buffer, fcgi_name_value* nv){
    uchar *p = buffer;
    uint32_t nl, vl;
    nl = strlen(nv->name);
    vl = strlen(nv->value);

    if( nl < 128 )
        *p++ = BYTE_0(nl);
    else
    {
        *p++ = BYTE_0(nl);
        *p++ = BYTE_1(nl);
        *p++ = BYTE_2(nl);
        *p++ = BYTE_3(nl);
    }

    if ( vl < 128 )
        *p++ = BYTE_0(vl);
    else
    {
        *p++ = BYTE_0(vl);
        *p++ = BYTE_1(vl);
        *p++ = BYTE_2(vl);
        *p++ = BYTE_3(vl);
    }

    memcpy(p, nv->name, nl);
    p+=nl;
    memcpy(p, nv->value, vl);
    p+= vl;

    return p - buffer;
}

void print_bytes(uchar *buf, int n){
    int i;
    printf("{");
    for(i=0;i<n;i++)
        printf("%02X.", buf[i]);
    printf("}\n");
}
