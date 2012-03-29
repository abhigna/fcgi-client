#include "fcgi_defs.h"
#include "fcgi_header.h"

#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define BUF_SIZE 1024
#define FCGI_SERVER "127.0.0.1"
#define FCGI_PORT "9000"
#define MAXDATASIZE 1000

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
    /*char s[INET6_ADDRSTRLEN], buf[MAXDATASIZE];*/

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(FCGI_SERVER, FCGI_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
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

    /*inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),*/
            /*s, sizeof s);*/
    /*printf("client: connecting to %s\n", s);*/
    freeaddrinfo(servinfo); // all done with this structure
    *sock = sockfd;

    /*if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {*/
        /*perror("recv");*/
        /*exit(1);*/
    /*}*/

    /*buf[numbytes] = '\0';*/

    /*printf("client: received '%s'\n",buf);*/

    /*close(sockfd);*/

    return 0;
}

#define N_NameValue 30
fcgi_name_value nvs[N_NameValue] = {
{"HOME", "/home/abhigna"},
{"TERM", "linux"},
{"PATH", "/opt/bin:/opt/sbin:/sbin:/bin:/usr/sbin:/usr/bin"},
{"PWD", "/home/abhigna"},
{"PHP_FCGI_CHILDREN", "2"},
{"PHP_FCGI_MAX_REQUESTS", "1000"},
{"FCGI_ROLE", "RESPONDER"},
{"SERVER_SOFTWARE", "lighttpd/1.4.29"},
{"SERVER_NAME", "ddwrt.lan"},
{"GATEWAY_INTERFACE", "CGI/1.1"},
{"SERVER_PORT", "8010"},
{"SERVER_ADDR", "127.0.0.1"},
{"REMOTE_PORT", "43485"},
{"REMOTE_ADDR", "127.0.0.1"},
{"SCRIPT_NAME", "/test.php"},
{"PATH_INFO", "no value"},
{"SCRIPT_FILENAME", "/home/abhigna/test.php"},
{"DOCUMENT_ROOT", "/home/abhigna/"},
{"REQUEST_URI", "/test.php"},
{"QUERY_STRING", "no value"},
{"REQUEST_METHOD", "GET"},
{"REDIRECT_STATUS", "200"},
{"SERVER_PROTOCOL", "HTTP/1.1"},
{"HTTP_HOST", "ddwrt.lan:8010"},
{"HTTP_CONNECTION", "keep-alive"},
{"HTTP_USER_AGENT", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.83 Safari/535.11"},
{"HTTP_ACCEPT", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"},
{"HTTP_ACCEPT_LANGUAGE", "en-US,en;q=0.8"},
{"PHP_SELF", "/test.php"},
{"REQUEST_TIME", "1333010004"}
};

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
    while(1){

    if ((nb = recv(sockfd, rbuf, BUF_SIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
        if(nb == 0)
            break;
        for(i = 0; i< nb;i++)
            printf("%c", rbuf[i]);
    }


    /*printf("Received %d|", nb, rbuf);*/
    print_bytes(rbuf, nb);
}

int main(int argv, char **argc){
    int sockfd;
    fcgi_connect(&sockfd);
    simple_session_1(sockfd);
    close(sockfd);
    return 0;
}
