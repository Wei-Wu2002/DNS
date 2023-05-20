#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 53
#define MAX_BUFFER_SIZE 1024

// DNS Header结构体
typedef struct {
    unsigned short id;
    unsigned char rd : 1;
    unsigned char tc : 1;
    unsigned char aa : 1;
    unsigned char opcode : 4;
    unsigned char qr : 1;
    unsigned char rcode : 4;
    unsigned char z : 3;
    unsigned char ra : 1;
    unsigned short qdcount;
    unsigned short ancount;
    unsigned short nscount;
    unsigned short arcount;
} DNSHeader;

// DNS Question结构体
typedef struct {
    unsigned short qtype;
    unsigned short qclass;
} DNSQuestion;

// DNS Resource Record结构体
typedef struct {
    unsigned short type;
    unsigned short class;
    unsigned int ttl;
    unsigned short rdlength;
} DNSResourceRecord;

// 解析DNS回复
void parse_response(char *buffer, int len) {
    DNSHeader *header = (DNSHeader *)buffer;

    // 跳过DNS Header
    char *current = buffer + sizeof(DNSHeader);

    // 解析Question部分
    for (int i = 0; i < ntohs(header->qdcount); i++) {
        while (*current != 0x00) {
            current += strlen(current) + 1;
        }
        current += 5;  // 跳过QTYPE和QCLASS
    }

    // 解析Answer部分
    printf("Answers:\n");
    for (int i = 0; i < ntohs(header->ancount); i++) {
        char domain_name[MAX_BUFFER_SIZE] = "";
        int domain_name_len = 0;
        while (*current != 0x00) {
            int label_len = *current;
            strncat(domain_name, current + 1, label_len);
            strncat(domain_name, ".", 1);
            domain_name_len += label_len + 1;
            current += label_len + 1;
        }
        current++;

        DNSResourceRecord *rr = (DNSResourceRecord *)current;
        unsigned short rdlength = ntohs(rr->rdlength);
        current += sizeof(DNSResourceRecord);

        if (ntohs(rr->type) == 1 && rdlength == 4) {  // A record
            printf("%s IN A ", domain_name);
            for (int j = 0; j < rdlength; j++) {
                printf("%u", (unsigned char)current[j]);
                if (j < rdlength - 1) {
                    printf(".");
                }
            }
            printf("\n");
        } else if (ntohs(rr->type) == 15) {  // MX record
            printf("%s IN MX ", domain_name);
            unsigned short preference = ntohs(*(unsigned short *)current);
            current += sizeof(unsigned short);
            printf("%u ", preference);
            char mx_domain_name[MAX_BUFFER_SIZE] = "";
            int mx_domain_name_len = 0;
            while (*current != 0x00) {
                int label_len = *current;
                strncat(mx_domain_name, current + 1, label_len);
                strncat(mx_domain_name, ".", 1);
                mx_domain_name_len += label_len + 1;
                current += label_len + 1;
            }
            printf("%s\n", mx_domain_name);
            current++;
        } else if (ntohs(rr->type) == 5) {  // CNAME record
            printf("%s IN CNAME ", domain_name);
            char cname[MAX_BUFFER_SIZE] = "";
            int cname_len = 0;
            while (*current != 0x00) {
                int label_len = *current;
                strncat(cname, current + 1, label_len);
                strncat(cname, ".", 1);
                cname_len += label_len + 1;
                current += label_len + 1;
            }
            printf("%s\n", cname);
            current++;
        } else {
            current += rdlength;
        }
    }
}

// 构建DNS请求
void build_dns_request(char *domain, unsigned short qtype, char *buffer) {
    DNSHeader *header = (DNSHeader *)buffer;
    memset(header, 0, sizeof(DNSHeader));
    header->id = htons(getpid());
    header->rd = 1;
    header->qdcount = htons(1);

    char *current = buffer + sizeof(DNSHeader);
    strcpy(current, domain);
    int domain_len = strlen(current);
    current += domain_len + 1;

    DNSQuestion *question = (DNSQuestion *)current;
    question->qtype = htons(qtype);
    question->qclass = htons(1);
}

int main() {
    int sock;
    struct sockaddr_in server_addr;

    // 创建套接字
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("Failed to create socket");
        exit(1);
    }

    // 设置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0) {
        perror("Invalid server address");
        exit(1);
    }

    // 读取用户输入的域名
    char domain[MAX_BUFFER_SIZE];
    printf("Enter domain name: ");
    fgets(domain, sizeof(domain), stdin);
    domain[strcspn(domain, "\n")] = 0;  // 去除换行符

    // 构建DNS请求
    char request_buffer[MAX_BUFFER_SIZE];
    build_dns_request(domain, 1, request_buffer);  // 查询A记录

    // 发送DNS请求
    if (sendto(sock, request_buffer, sizeof(DNSHeader) + strlen(domain) + 2 + sizeof(DNSQuestion),
               0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to send DNS request");
        exit(1);
    }

    // 接收DNS回复
    char response_buffer[MAX_BUFFER_SIZE];
    int server_addr_len = sizeof(server_addr);
    int response_len = recvfrom(sock, response_buffer, sizeof(response_buffer), 0,
                                (struct sockaddr *)&server_addr, (socklen_t *)&server_addr_len);
    if (response_len == -1) {
        perror("Failed to receive DNS response");
        exit(1);
    }

    // 解析DNS回复
    parse_response(response_buffer, response_len);

    // 关闭套接字
    close(sock);

    return 0;
}
