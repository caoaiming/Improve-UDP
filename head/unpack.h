#ifndef __UNPACK_H_
#define __UNPACK_H_

#define LOCAL_NET_SIZE  1400     /* <= 1500-20-8  local area Internet */
#define INTER_NET_SIZE  544      /* <= 576-20-8 Internet标准MTU 576 */
#define DATA_SIZE       INTER_NET_SIZE - 32 /* send data size 512 */
#define ACK_SIZE        96   

#define SSTH_DEFAULT_SIZE        20
#define WIN_FIRST_SIZE           10
#define WIN_DEFAULT_SIZE         10
#define WIN_INIT_SIZE            1
#define WIN_MIN_SIZE             2
#define WIN_SLOW_START           1

typedef unsigned int uint32_t;

enum Data_type {
    PT_Invalid = 0,
    PT_Urgent,
    PT_Fin,
    PT_Acknowledge,
    PT_Lost,
    PT_Lastack,
    PT_Finish,
    PT_Data,
    PT_Empty,
    PT_Reset    
};

//single udp package context headler infomation.
struct pack_head {
    uint32_t    pack_total;             /*  package total counts  */
    uint32_t    pack_type;              /*  package type          */
    uint32_t    pack_size;              /*  single  package size  */
    uint32_t    Checksum;               /*  CRC checksum          */
    uint32_t    serial_no;              /*  serial number         */
    uint32_t    pack_seq;               /*  package No.           */
    uint32_t    ts;                     /*  time */
    unsigned short  pack_offset;        /*  package offset length */
    char        buf[DATA_SIZE];         /*  package buffer        */
};

typedef struct Packet {
    struct pack_head pack;
    int    flag;
}_pack;

struct Ack {
    uint32_t ack_type;         /*应答类型*/
    uint32_t ack_cnt;          /*应答数量*/
    uint32_t seq[20];          /*应答 seq*/
    uint32_t ts;               /*回射时间戳*/
    uint32_t win;              /*可用缓冲窗口*/
};

struct sliding_win {
    int cnwd;           /*估计发送窗口*/
    int ssthresh;       /*拥塞窗口*/
    int win_size;       /*发送窗口*/
    int win_start;      /*发送分组起始端*/
    int win_end;        /*发送分组终止端*/
    int step_add;       /*窗口增量*/
};

#endif // end _unpack_h_
