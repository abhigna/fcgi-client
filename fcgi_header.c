#include "fcgi_defs.h"
#include "fcgi_header.h"

void print_mem(void const *vp, size_t n)
{
    unsigned char const *p = vp;
    size_t i;
    for (i=0; i<n; i++)
        printf("%02X\n", p[i]);
    putchar('\n');
};


void fcgi_header_set_request_id(fcgi_header *h, uint16_t request_id){
    h->request_id_lo = BYTE_0(request_id);
    h->request_id_hi = BYTE_1(request_id);
}

void fcgi_header_set_content_len(fcgi_header *h, uint16_t len){
    h->content_len_lo = BYTE_0(len);
    h->content_len_hi = BYTE_1(len);
}

uint32_t fcgi_header_get_content_len(fcgi_header *h){
    return   (h->content_len_hi << 8) + h->content_len_lo;
}

fcgi_header* create_header(uchar type, uint16_t request_id){
    fcgi_header* tmp = (fcgi_header *) calloc(1,sizeof(fcgi_header));
    tmp->version = FCGI_VERSION_1;
    tmp->type    = type;
    fcgi_header_set_request_id(tmp, request_id);
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

fcgi_record* fcgi_record_create()
{
    fcgi_record* tmp = (fcgi_record *) malloc(sizeof(fcgi_record));
    tmp->header = (fcgi_header *) calloc(sizeof(fcgi_header), 1);
    tmp->next = NULL;
    tmp->state = 0;
    return tmp;
}

int fcgi_process_header(uchar ch,
        fcgi_record* rec){
    fcgi_header *h;
    fcgi_state *state = &(rec->state);
    h = rec->header;

    switch(*state){
        case fcgi_state_version:
            h->version = ch;
            *state = fcgi_state_type;
            break;
        case fcgi_state_type:
            h->type = ch;
            *state = fcgi_state_request_id_hi;
            break;
        case fcgi_state_request_id_hi:
            h->request_id_hi = ch;
            *state = fcgi_state_request_id_lo;
            break;
        case fcgi_state_request_id_lo:
            h->request_id_lo = ch;
            *state = fcgi_state_content_len_hi;
            break;
        case fcgi_state_content_len_hi:
            h->content_len_hi = ch;
            *state = fcgi_state_content_len_lo;
            break;
        case fcgi_state_content_len_lo:
            h->content_len_lo = ch;
            *state = fcgi_state_padding_len;
            break;
        case fcgi_state_padding_len:
            h->padding_len = ch;
            *state = fcgi_state_reserved;
            break;
        case fcgi_state_reserved:
            h->reserved = ch;
            *state = fcgi_state_content_begin;
            break;

        case fcgi_state_content_begin:
        case fcgi_state_content_proc:
        case fcgi_state_padding:
        case fcgi_state_done:
            return FCGI_PROCESS_DONE;
    }

    return FCGI_PROCESS_AGAIN;
}

int fcgi_process_content(uchar **beg_buf, uchar *end_buf,
       fcgi_record *rec){

    size_t tot_len, con_len, cpy_len, offset, nb = end_buf - *beg_buf;
    fcgi_state *state = &(rec->state);
    fcgi_header *h = rec->header;
    offset = rec->offset;

    if ( *state == fcgi_state_padding ){
        *state = fcgi_state_done;
        /*printf("padding %d\n", (int)rec->length - (int)offset + (int)h->padding_len);*/
        *beg_buf += (size_t) ((int)rec->length - (int)offset + (int)h->padding_len);
        return FCGI_PROCESS_DONE;
    }

    con_len = rec->length - offset;
    tot_len = con_len + h->padding_len;

    if(con_len <= nb)
        cpy_len = con_len;
    else
        cpy_len = nb;

    memcpy(rec->content + offset, *beg_buf, cpy_len);

    if(tot_len <= nb)
    {
        rec->offset += tot_len;
        *state = fcgi_state_done;
        *beg_buf += tot_len;
        return FCGI_PROCESS_DONE;
    }
    else if( con_len <= nb )
    {
        /* Have to still skip all or some of padding */
        *state = fcgi_state_padding;
        rec->offset += nb;
        *beg_buf += nb;
        return FCGI_PROCESS_AGAIN;
    }
    else
    {  
        rec->offset += nb;
        *beg_buf += nb;
        return FCGI_PROCESS_AGAIN;
    }
    return 0;
}

int fcgi_process_record(uchar **beg_buf, uchar *end_buf, fcgi_record *rec){

        int rv;
        while(rec->state < fcgi_state_content_begin)
        {
            if((rv = fcgi_process_header(**beg_buf, rec)) == FCGI_PROCESS_ERR)
                    return FCGI_PROCESS_ERR;
            (*beg_buf)++;
            if( *beg_buf == end_buf )
                return FCGI_PROCESS_AGAIN;
        }
        if(rec->state == fcgi_state_content_begin)
        {
           rec->length = fcgi_header_get_content_len(rec->header);
           rec->content = malloc(rec->length);
           rec->state++;
        }

        return fcgi_process_content(beg_buf, end_buf, rec);
}

void fcgi_process_buffer(uchar *beg_buf, uchar *end_buf,
       fcgi_record_list** head){

    fcgi_record* tmp, *h;
    size_t i;
    if(*head == NULL)
        *head = fcgi_record_create();
    h = *head;

    while(1)
    {
        if( h->state == fcgi_state_done )
        {
            tmp = h;
            *head = fcgi_record_create();
            h = *head;
            h->next = tmp;
        }

       if( fcgi_process_record(&beg_buf, end_buf, h) == FCGI_PROCESS_DONE ){
           /*fprintf(stderr, "TYPE%d:LEN:%d\n", h->header->type, h->length);*/
           if(h->header->type == FCGI_STDOUT)
                for(i=0;i < h->length; i++)
                    fprintf(stdout, "%c", ((uchar *)h->content)[i]);
       }

        if ( beg_buf == end_buf )
            return;
    }
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
