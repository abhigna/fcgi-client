#include "fcgi_defs.h"
#include "fcgi_header.h"

#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define BUF_SIZE 5000
#define FCGI_SERVER "127.0.0.1"
#define FCGI_PORT "9000"
#define MAXDATASIZE 1000

#define N_NameValue 27
fcgi_name_value nvs[N_NameValue] = {
{"SCRIPT_FILENAME", "/home/abhigna/test.php"},
{"SCRIPT_NAME", "/test.php"},
{"DOCUMENT_ROOT", "/home/abhigna/"},
{"REQUEST_URI", "/test.php"},
{"PHP_SELF", "/test.php"},
{"TERM", "linux"},
{"PATH", ""},
{"PHP_FCGI_CHILDREN", "2"},
{"PHP_FCGI_MAX_REQUESTS", "1000"},
{"FCGI_ROLE", "RESPONDER"},
{"SERVER_SOFTWARE", "lighttpd/1.4.29"},
{"SERVER_NAME", "SimpleServer"},
{"GATEWAY_INTERFACE", "CGI/1.1"},
{"SERVER_PORT", FCGI_PORT},
{"SERVER_ADDR", FCGI_SERVER},
{"REMOTE_PORT", ""},
{"REMOTE_ADDR", "127.0.0.1"},
{"PATH_INFO", "no value"},
{"QUERY_STRING", "no value"},
{"REQUEST_METHOD", "GET"},
{"REDIRECT_STATUS", "200"},
{"SERVER_PROTOCOL", "HTTP/1.1"},
{"HTTP_HOST", "localhost:9000"},
{"HTTP_CONNECTION", "keep-alive"},
{"HTTP_USER_AGENT", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.83 Safari/535.11"},
{"HTTP_ACCEPT", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"},
{"HTTP_ACCEPT_LANGUAGE", "en-US,en;q=0.8"},
};

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int fcgi_connect(int *sock){
    int sockfd;//, numbytes;
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(FCGI_SERVER, FCGI_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    freeaddrinfo(servinfo);
    *sock = sockfd;
    return 0;
}


void simple_session_1(int sockfd){
    uint16_t req_id = 1;
    uint16_t len=0;
    int nb, i;
    unsigned char *p, *buf, *rbuf;
    fcgi_header* head;
    fcgi_begin_request* begin_req = create_begin_request(req_id);

    rbuf = malloc(BUF_SIZE);
    buf  = malloc(BUF_SIZE);
    p = buf;
    serialize(p, begin_req->header, sizeof(fcgi_header));
    p += sizeof(fcgi_header);
    serialize(p, begin_req->body, sizeof(fcgi_begin_request_body));
    p += sizeof(fcgi_begin_request_body);

    /* Sending fcgi_params */
    head = create_header(FCGI_PARAMS, req_id);

    len = 0;
    /* print_bytes(buf, p-buf); */
    for(i = 0; i< N_NameValue; i++) {
        nb = serialize_name_value(p, &nvs[i]);
        len += nb;
    }

    head->content_len_lo = BYTE_0(len);
    head->content_len_hi = BYTE_1(len);


    serialize(p, head, sizeof(fcgi_header));
    p += sizeof(fcgi_header);

    for(i = 0; i< N_NameValue; i++) {
        nb = serialize_name_value(p, &nvs[i]);
        p += nb;
    }

    head->content_len_lo = 0;
    head->content_len_hi = 0;

    serialize(p, head, sizeof(fcgi_header));
    p += sizeof(fcgi_header);

    /*printf("Total bytes sending %ld", p-buf);*/

    /*print_bytes(buf, p-buf);*/

    if (send(sockfd, buf, p-buf, 0) == -1){
            perror("send");
            close(sockfd);
            return;
    }
    fcgi_record_list *rlst = NULL, *rec;

    while(1){
        if ((nb = recv(sockfd, rbuf, BUF_SIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }
        if(nb == 0)
            break;
        fcgi_process_buffer(rbuf, rbuf+(size_t)nb, &rlst);

    }

    for(rec=rlst; rec!=NULL; rec=rec->next)
    {
        /*if(rec->header->type == FCGI_STDOUT)*/
            /*printf("PADD<%d>", rec->header->padding_len);*/
            /*printf("%d\n", rec->length);*/
            /*for(i=0;i < rec->length; i++) fprintf(stdout, "%c", ((uchar *)rec->content)[i]);*/

    }
}

int main(int argc, char **argv){
    int sockfd;
    if( argc != 2){
        printf("Usage: %s <Absolute path of file>\n", argv[0]);
        return 0;
    }
    else
        nvs[0].value = argv[1];
    fcgi_connect(&sockfd);
    simple_session_1(sockfd);
    close(sockfd);
    return 0;
}
