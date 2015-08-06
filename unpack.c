#include "udp.h"
#include "unpack.h"
#include "udp_rtt.h"



int sock_init(char *ip, struct sockaddr_in *addr)
{
    int sockfd = -1;
    int optval = 1;

    if (!addr)
        return NG;
    bzero(addr, sizeof(struct sockaddr));

    if (ip == NULL)
        addr->sin_addr.s_addr = INADDR_ANY;
    else  
        Inet_aton(ip, &addr->sin_addr);

    addr->sin_family = AF_INET;
        addr->sin_port = htons(8888);
    sockfd = x_socket(AF_INET, SOCK_DGRAM, 0);
    x_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

    x_bind(sockfd, (SA *)addr, sizeof(struct sockaddr));

    return sockfd;
}

void delete(_pack *result, int cnt)
{
    if (result != NULL && cnt != 0) {
        free(result);
    }
    else
        return;
}
#if 0
void display(_pack *res, int cnt)
{
    int i;

    for(i = 0; i < cnt; i++) {
        printf("总包数  ：  %d\n", res[i].pack_total);
        printf("类型    ：  %d\n", res[i].pack_type);
        printf("流水号  ：  %d\n", res[i].serial_no);
        printf("包序号  ：  %d\n", res[i].pack_No);
        printf("偏移长度：  %d\n", res[i].pack_offset);
        printf("包大小  ：  %d\n", res[i].pack_size);
        printf("Checksum：  %u\n", res[i].Checksum);
        printf("----------------------------------------\n");
        printf("buff : \n%s\n", res[i].buf);
        printf("----------------------------------------\n");
        printf("**************************************************\n\n");
    }
}
#endif
void static packet(uint32_t  totle, uint32_t type, int send_size, 
        uint32_t serial, int size_n, const char *buf, _pack *res)
{
    int i;
    int offset, copy_size;
    uint32_t calcu_crc = 0xffffffff;
    char *ss;
    static int No;

    copy_size = DATA_SIZE;

    No++;
    for (i = 0; i < totle; i++) {
        offset = i * copy_size;
        if (i == totle - 1) {
            copy_size = size_n - (totle-1) * copy_size;
        }

        res[i].pack.pack_total  = totle;
        res[i].pack.pack_type   = type;
        res[i].pack.pack_offset = offset;
        //res[i].pack.pack_size   = INTER_NET_SIZE;
        res[i].pack.pack_size   = copy_size;
        res[i].pack.serial_no   = No;
        res[i].pack.Checksum    = 0;
        res[i].pack.pack_seq    = i + 1;

        memset(res[i].pack.buf, '\0', DATA_SIZE);
        memcpy(res[i].pack.buf, buf + offset, copy_size);

        ss = (char *)(res+i);
        res[i].pack.Checksum = make_crc(calcu_crc, ss, INTER_NET_SIZE);
        res[i].flag = NG;
    }
}

void *split_pack(const char *buf, size_t size_n, int *cnt)
{
    int packno = 0;            /* 流水号 */
    int totle  = 0;            /* 发送总个数 */
    int sta_size = DATA_SIZE;  /* 单个数据包的标准大小 */
    _pack *result = NULL;      /* 结构体数组存放拆好包的数据 */

    if (buf == NULL || size_n == 0) {
        result = (_pack *)malloc(sizeof(_pack));
        totle = 1;
        packet(1, PT_Empty, DATA_SIZE, packno, size_n, NULL, result);
    } else {
        totle = size_n / sta_size + !(!(size_n % sta_size));

        result = (_pack *)malloc(sizeof(_pack) * totle);

        packet(totle, PT_Data, DATA_SIZE, packno, size_n, buf, result);
    }
    *cnt = totle;

    return result;
/*
    display(result, totle);
    delete(result, totle);
*/
};


bool check_Allpack(int *no, struct Ack* ack, int totle)
{
    int i, index = 0;
    ack->ack_type = PT_Lost;

    for(i = 0; i < totle && index < 20; i++) {
        if (no[i] == 0) {
            (ack->seq)[index++] = i;
            printf("重发丢失包编号 ： %d", i+1);
        }
    }

    if(index) {            //重发丢失包
        ack->ack_cnt = index;
        return NG;
    }
    printf("接收完毕！\n");

    ack->ack_type = PT_Lastack; //全部接收
    return OK;
}

bool anwser_ack(struct Ack *ack, const struct pack_head *pack, 
        int index, int able_buf, int *no, int *totle)
{
    if (pack->pack_type == PT_Data) {
        printf("普通包...\n");
        ack->ack_type = PT_Acknowledge;
        (ack->seq)[index] = pack->pack_seq;
        ack->ts  = pack->ts;
        ack->ack_cnt = index + 1;
        ack->win = able_buf;
        *totle = pack->pack_total;
    }
    else if (pack->pack_type == PT_Fin) {
        printf("结束包...\n");
        if (check_Allpack(no, ack, *totle) == OK) {
            printf("sucess.\n");
            return OK;
        }
    }
    return NG;
}

void getmsg_info(const struct pack_head *pack, char *buf, int *no)
{
    if (!no[pack->pack_seq - 1]) {
        no[pack->pack_seq - 1] = 1;
        memcpy(buf + pack->pack_offset, pack->buf, pack->pack_size);
    }
}

