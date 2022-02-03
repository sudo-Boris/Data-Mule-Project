#include "CC1200_lib.h"
#include <assert.h>

static uint8_t sync_id = 0;
static int resend = 0;
int cc1200_init(){
    int adr;
    int val;
    int i = 0;
    cc1200_cmd(SIDLE);
    sleep(1);    
    int j;
    wait_pkt.payload = (char*) malloc(sizeof(char) * 50);
   
    // -------------- init TX --------------
    for(i=0; i < MAX_REG; i++)
        cc1200_reg_write(RegSettingsTX[i].adr, RegSettingsTX[i].val);
    for(i=0; i < MAX_EXT_REG; i++)
        cc1200_reg_write(ExtRegSettingsTX[i].adr, ExtRegSettingsTX[i].val);
    change_mode(INITTX);
    cc1200_cmd(SIDLE);
    wait_cmd(IDLE);

    // -------------- init RX --------------
    for(i=0; i < MAX_REG; i++)
        cc1200_reg_write(RegSettings[i].adr, RegSettings[i].val);
    for(i=0; i < MAX_EXT_REG; i++)
        cc1200_reg_write(ExtRegSettings[i].adr, ExtRegSettings[i].val);
    change_mode(INITRX);
    cc1200_cmd(SIDLE);
    wait_cmd(IDLE);

    cc1200_reg_write(FS_DIG0, 0x50);
    cc1200_reg_write(SETTLING_CFG, 0x03);
    // exit with default state RX
    cc1200_cmd(SRX);
    wait_cmd(RX);
    return 0;
}

void wait_cmd(CC1200_STATES cmd){
    int status = get_status_cc1200();
    cc1200_cmd(SNOP);
    while(status != cmd){
        status = get_status_cc1200();
	cc1200_cmd(SNOP);
    }
}

void change_mode(uint8_t calibrate){
    cc1200_cmd(SIDLE);
    wait_cmd(IDLE);
 
    if(calibrate == CAL){
        cc1200_cmd(SCAL);
        wait_cmd(CALLIBRATE);
    }else if (calibrate == CALRX || calibrate == INITRX){

        cc1200_cmd(SRX);
        wait_cmd(RX);
    }else if(calibrate == CALTX || calibrate == INITTX){

        cc1200_cmd(STX);
        wait_cmd(TX);
    }

}

void print_freq(){
    int f0,f1,f2;
    cc1200_reg_read(FREQ0, &f0);
    cc1200_reg_read(FREQ1, &f1);
    cc1200_reg_read(FREQ2, &f2);
    f2 = (f2 << 16) | (f1 << 8) | f0;
    printf("RF: %i MHz\n", f2);
}

void receive_packets_variable(){  
    int pkt_length, c, bytes_in_FIFO;
    usleep(1000);
    cc1200_reg_read(NUM_RXBYTES, &bytes_in_FIFO);

    while(bytes_in_FIFO > 0){
        // Construct packet that is received ------------------
        dataframe pkt = decode_message();

        dataframe answer;
        uint8_t meta = pkt.meta_data;
        switch (MSG_TYPE(pkt.meta_data)){
            case SEARCH:{
                // Construct packet to answer searching node    
                answer.pkt_length = TOTAL_SIZE(1);
                answer.addr = pkt.src;
                answer.src = NODE_ID;
                answer.payload = (char*) malloc(sizeof(char) + 1);
                strcpy(answer.payload, "0");                

                if(IS_MULE){
                    /*
                        Tell other mule that you are a mule and get ready for
                        backpack synchronisation - you will beginn wich recieving
                    */
                    sync_id = pkt.src;
                    answer.meta_data = (MULE_BP | SYNC_READY);
                    send_frame(answer);
                    change_mode(CALRX);
                    free(answer.payload);
                    sync_backpacks(false, pkt.src);
                }else{
                    answer.meta_data = DISCOVERED;
                    send_frame(answer);
                    free(answer.payload);
                }
                
                // return to RX
                change_mode(CALRX);
                
            }break;

            case MULE_BP:{
                /* Found other mule - initiate backpack synchronization */
                #ifdef EVAL
                    printf("[INFO]: Found mule %i - initiating backpack synchronization \n", pkt.src);
                #endif
                if(MULE_TYPE(meta) == SYNC_READY){
                    sync_id = pkt.src;
                    sync_backpacks(true, pkt.src);
                }
            }break;

            case DISCOVERED:{
                #ifdef EVAL
                    printf("[INFO]: Discovered mote with ID: %i\n", pkt.src);            
                #endif
                nodes_area[nodes_cnt] = pkt.src;
                /* Only able to track 20 surrounding nodes */
                (nodes_cnt > MAX_NODES) ? (nodes_cnt = 0) : nodes_cnt++ ;
            }break;

            case ACK:{
                /* 
                    After mote updated, move msg from backpack to archive
                */ 
                char* str = (char*) malloc(strlen(pkt.payload) + 4);
                itoa(pkt.src, str);
                strcat(str,",");
                char* tmp = llist_pop(backpack[pkt.src]);
                strcat(str, tmp);
                free(tmp);
                ht_insert(archive, str, true);
                #ifdef EVAL
                    printf("[INFO]: Received ACK, mote %i is updated | %s\n", pkt.src, str);
                #endif
            }break;
            default:{
                #ifdef EVAL
                    printf("[INFO]: Received message: %s\n", pkt.payload);
                #endif
                // Construct packet to answer searching node    
                answer.pkt_length = TOTAL_SIZE(1);
                answer.addr = pkt.src;
                answer.meta_data = ACK;
                answer.src = NODE_ID;
                answer.payload = (char*) malloc(sizeof(char) + 1);
                strcpy(answer.payload, "0");
                // ! --> send_frame finishes in TX
                #ifdef EVAL
                    printf("[INFO]: Sending ACK, I (ID: %i) am updated\n", NODE_ID);
                #endif
                send_frame(answer);
                free(answer.payload);
                // return to RX
                change_mode(CALRX);
            }break;
        }
        
        cc1200_reg_read(NUM_RXBYTES, &bytes_in_FIFO);
    }
}

void update_backpack(char* msg, uint8_t dest){
    llist_push(backpack[dest], msg); 
}

void send_mule_backpack(uint8_t src){
    int resend = 0;
    // Construct pkt
    dataframe pkt;
    pkt.addr = src;
    pkt.meta_data = MULE_BP;
    pkt.src = NODE_ID;
    int RX_LEN = 0;
    int i;
    dataframe load;
    for(i = 0; i < MAX_NODES; i++){
        node* head = *backpack[i];
        if(!head) continue;
        while(head && head->data != NULL && !(head->checked)){
            pkt.payload = (char*) malloc(strlen(head->data) + 3 + 1); // data_len + max of 2 bytes for ID + ","
            itoa(i, pkt.payload);
            strcat(pkt.payload, ",");
            strcat(pkt.payload, head->data);
            pkt.pkt_length = TOTAL_SIZE(strlen(pkt.payload));

            #ifdef EVAL
                printf("[INFO: SYNC_BP] Payload to send to mule for sync: %s\n", pkt.payload);
            #endif
            uint8_t error = FAULT_CRC;

            while(MSG_TYPE(error) == FAULT_CRC){
                send_frame(pkt);
                change_mode(CALRX);
                load = block_transmission();
                error = load.meta_data;    
            }

            free(pkt.payload);

            /* wait for ACK */

            uint8_t type = MULE_TYPE(load.meta_data);

            if (type == IN_ARCHIVE){
                /* drop own pkt in BP and add to archive */
                char** tokens = split_String(load.payload, ',');
                if (tokens){
                    int BP_ID = atoi(*(tokens));
                    char* BP_data = *(tokens + 1);
                    free(tokens);
                    /* move msg to archive */
                    ht_insert(archive, load.payload, true);
                    llist_delete_data(backpack[BP_ID], BP_data);
                }
            } 
            #ifdef EVAL
                printf("[INFO: SYNC_BP] Received BP_ACK from node %i \n", load.addr);
            #endif
            free(load.payload);

            head = head->next;
        }
    }
    
    /* done with own backpack */
    pkt.pkt_length = TOTAL_SIZE(1);
    pkt.meta_data = MULE_BP | BP_DONE;
    pkt.payload = (char*) malloc(sizeof(char) + 1);
    strcpy(pkt.payload, "0");
    send_frame(pkt);
    change_mode(CALRX);
    free(pkt.payload);
    load = block_transmission();
    if((load.meta_data & MULE_ACK) == MULE_ACK){
        #ifdef EVAL
            printf("[INFO: SYNC_BP] Finish transmitting backpack to mule %i \n", load.src);
        #endif
    }
}

dataframe block_transmission(){
    dataframe load;
    load.src = 100;
    while(load.src != sync_id){
        wait_for_reception();
        load = decode_message();
    }
    return load;
}

void receive_mule_backpack(){
    dataframe pkt, answer;
    while(1){
        pkt = block_transmission();

        if((pkt.meta_data & BP_DONE) == BP_DONE){
            #ifdef EVAL
                printf("[INFO: SYNC_BP] Finished reception from Mule %i \n", pkt.src);
            #endif
            answer.meta_data = MULE_ACK;
            answer.src = NODE_ID;
            answer.addr = pkt.src;
            answer.payload = (char*) malloc(sizeof(char) + 1);
            answer.pkt_length = TOTAL_SIZE(1);
            strcpy(answer.payload, "0");
            send_frame(answer);
            free(answer.payload);
            change_mode(CALRX);
            usleep(100);
            return;
        }
        #ifdef EVAL
            printf("[INFO: SYNC_BP] Received message | %s\n",pkt.payload);
        #endif
        char** tokens = (char**) split_String(pkt.payload, ',');
        // printf("Not here\n");
        int BP_ID;
        char* BP_data;
        if (tokens){
            BP_ID = atoi(*(tokens));
            BP_data = *(tokens + 1);
            free(tokens);
        }

        /*
            look in Archive for data 
            - if there, send MULE_ACK with IN_ARCHIVE flag
            - if in own BP -> everything easy
            - if not in archive - and not in own BP -> add to own BP 
        */
        if(ht_search(archive, pkt.payload)){
            answer.addr = pkt.src;
            answer.meta_data = (MULE_ACK | IN_ARCHIVE);
            answer.src = NODE_ID;
            answer.pkt_length = pkt.pkt_length;
            answer.payload = (char*) malloc(PAYLOAD_SIZE(pkt.pkt_length) + 1);
            strcpy(answer.payload, pkt.payload);
            send_frame(answer);
            free(answer.payload);
        }else if(llist_find(backpack[BP_ID], BP_data)){
            llist_update_flag(backpack[BP_ID], BP_data, true);
            answer.addr = pkt.src;
            answer.meta_data = MULE_ACK;
            answer.src = NODE_ID;
            answer.pkt_length = TOTAL_SIZE(1);
            answer.payload = (char*) malloc(sizeof(char) + 1);
            strcpy(answer.payload, "0");
            send_frame(answer);
            free(answer.payload);
        }else{
            llist_push(backpack[BP_ID], BP_data);
            llist_update_flag(backpack[BP_ID], BP_data, true);
            answer.addr = pkt.src;
            answer.meta_data = MULE_ACK;
            answer.src = NODE_ID;
            answer.pkt_length = TOTAL_SIZE(1);
            answer.payload = (char*) malloc(sizeof(char) + 1);
            strcpy(answer.payload, "0");
            send_frame(answer);
            free(answer.payload);
        }
        change_mode(CALRX);
    }
}

void sync_backpacks(bool initiator, uint8_t src){
    clock_t timer = clock();

    if(initiator){
        send_mule_backpack(src);
        receive_mule_backpack();
    }else{
        receive_mule_backpack();
        send_mule_backpack(src);
    }
    printf("\nBP synchronization took %lf | resend %i\n", (double) (clock() - timer) / CLOCKS_PER_SEC, resend);

        
    /* After sync is done, reset optimisation flags */
    int i;
    for(i = 0; i < MAX_NODES; i++){
        node* head = *backpack[i];
        if(!head) continue;
        while(head && head->data != NULL){
            head->checked = false;
            head = head->next;
        }
    }

}

void send_frame(dataframe frame){
    static int i = 0;
    // Change mode to TX if necessary
    usleep(100);
    change_mode(CALTX);
    usleep(100);
    
    cc1200_reg_write(FIFO, frame.pkt_length);
    cc1200_reg_write(FIFO, frame.addr);
    cc1200_reg_write(FIFO, frame.meta_data);
    cc1200_reg_write(FIFO, frame.src);
    #ifdef DEBUG
        printf("SEND: Len %i | Addr %i | Src %i | Meta %i | Pay_len %i\n", frame.pkt_length, frame.addr,
        frame.src, frame.meta_data, strlen(frame.payload));
    #endif
    do{
        cc1200_reg_write(FIFO, *frame.payload);
    }while(*frame.payload++);
    wait_fifo_empty();
}

dataframe decode_message(){
    dataframe load;
    int c;
    int crc = 0;
    while(!CHECK_CRC(crc)){
        usleep(5000);
        cc1200_reg_read(FIFO, (int*) &load.pkt_length);
        cc1200_reg_read(FIFO, (int*) &load.addr);
        cc1200_reg_read(FIFO, (int*) &load.meta_data);
        cc1200_reg_read(FIFO, (int*) &load.src);

        char msg[PAYLOAD_SIZE(load.pkt_length)];
        int i;
        for(i=0; i < PAYLOAD_SIZE(load.pkt_length); i++){
            cc1200_reg_read(FIFO, &c);
            msg[i] =  c;
        }
        msg[i] = '\0';
        load.payload = (char*) malloc(strlen(msg) + 1);
        strcpy(load.payload, msg);

        cc1200_reg_read(FIFO, &c);
        cc1200_reg_read(FIFO, &crc);

        #ifdef DEBUG
            printf("\n-----------------------------------\n");
            printf("Length: %i\n", load.pkt_length);
            printf("Addr: %i\n", load.addr);
            printf("Meta: %i\n", load.meta_data);
            printf("Src: %i\n", load.src);
            printf("Msg: %s \n", load.payload);
            printf("RSSI: %i\n", c);
            printf("CRC ok ? %s \n", CHECK_CRC(crc) ? "ok" : "nah");
            printf("-----------------------------------\n");
        #endif

        if(!CHECK_CRC(crc)){
            resend++;
            cc1200_cmd(SFRX);
            change_mode(CALRX);
            #ifdef DEBUG
                printf("[ERROR]: CRC was faulty! Ask for resend! \n");
            #endif
            dataframe answer;
            answer.addr = load.src;
            answer.meta_data = FAULT_CRC;
            answer.src = NODE_ID;
            answer.pkt_length = TOTAL_SIZE(1);
            answer.payload = (char*) malloc(sizeof(char) + 1);
            strcpy(answer.payload, "0");
            send_frame(answer);
            change_mode(CALRX);
            free(answer.payload);
            wait_for_reception();
        }
    }
    return load;
}

void wait_for_reception(){
    int RX_LEN, LSRSSI, MSRSSI;
    signed int RSSI;
    cc1200_reg_read(NUM_RXBYTES, &RX_LEN);
    cc1200_reg_read(RSSI1, &MSRSSI);
    cc1200_reg_read(RSSI0, &LSRSSI);
    RSSI = (MSRSSI & 0xFF0) | (LSRSSI & 0xF); 
    while( !RX_LEN || RSSI < LOW_RSSI_THRES || RSSI > HIGH_RSSI_THRES){
        cc1200_reg_read(RSSI1, &MSRSSI);
        cc1200_reg_read(RSSI0, &LSRSSI);
        cc1200_reg_read(NUM_RXBYTES, &RX_LEN);
        RSSI = (MSRSSI & 0xFF0) | (LSRSSI & 0xF); 
    }

}

void send_msg(uint8_t dst, char* msg){
    dataframe pkt;
    pkt.pkt_length = TOTAL_SIZE(strlen(msg));
    pkt.addr = dst;
    pkt.src = NODE_ID;
    pkt.meta_data = DATA;
    pkt.payload = (char*) malloc(strlen(msg) + 1 );
    strcpy(pkt.payload, msg);
    send_frame(pkt);
    free(pkt.payload);
}

void discover_nodes(){
    memset(nodes_area, 0, sizeof(nodes_area));
    // Construct packet to be sent to destination
    dataframe pkt;
    pkt.pkt_length = TOTAL_SIZE(1);
    pkt.addr = BROADCAST;
    pkt.meta_data = SEARCH;
    pkt.src = NODE_ID;
    pkt.payload = (char*) malloc(sizeof(char) + 1);
    strcpy(pkt.payload, "0");
    send_frame(pkt);
    free(pkt.payload);
}

void wait_fifo_empty(){
    int len;
    cc1200_reg_read(NUM_TXBYTES, &len);
    while(len > 0) cc1200_reg_read(NUM_TXBYTES, &len);
    usleep(500);

}

void drop_pkt(int pkt_length){
    int tmp = 0;
    while(pkt_length){
        cc1200_reg_read(FIFO, &tmp);
        pkt_length--;
    }
}

void freq_calibrationRX(){
    cc1200_reg_read(FS_CHP , &ffhRX[0][0]);
    cc1200_reg_read(FS_VCO2, &ffhRX[0][1]);
    cc1200_reg_read(FS_VCO4, &ffhRX[0][2]);
    freq_index = 1;
    change_mode(INITRX);

    cc1200_reg_read(FS_CHP , &ffhRX[1][0]);
    cc1200_reg_read(FS_VCO2, &ffhRX[1][1]);
    cc1200_reg_read(FS_VCO4, &ffhRX[1][2]);
    freq_index = 2;
    change_mode(INITRX);

    cc1200_reg_read(FS_CHP , &ffhRX[2][0]);
    cc1200_reg_read(FS_VCO2, &ffhRX[2][1]);
    cc1200_reg_read(FS_VCO4, &ffhRX[2][2]);
    freq_index = 0;

}

void freq_calibrationTX(){
    cc1200_reg_read(FS_CHP , &ffhTX[0][0]);
    cc1200_reg_read(FS_VCO2, &ffhTX[0][1]);
    cc1200_reg_read(FS_VCO4, &ffhTX[0][2]);
    freq_index = 1;
    change_mode(INITTX);

    cc1200_reg_read(FS_CHP , &ffhTX[1][0]);
    cc1200_reg_read(FS_VCO2, &ffhTX[1][1]);
    cc1200_reg_read(FS_VCO4, &ffhTX[1][2]);
    freq_index = 2;
    change_mode(INITTX);

    cc1200_reg_read(FS_CHP , &ffhTX[2][0]);
    cc1200_reg_read(FS_VCO2, &ffhTX[2][1]);
    cc1200_reg_read(FS_VCO4, &ffhTX[2][2]);
    freq_index = 0;

}

bool existing_messages(){
    int i =0;
    for(; i < MAX_NODES; i++){
        node* head = *backpack[i];
        if(head && head->data != NULL )
            return true;
    }
    return false;
}

bool check_addr(uint8_t addr, uint8_t pkt_length){
    if(addr != NODE_ID && addr != BROADCAST){
        printf("Flushing toilet\n");
        drop_pkt(pkt_length+1);
        return false;
    }
    return true;
}

char** split_String(char* a_str, const char a_delim){
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;
    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }
    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = (char**) malloc(sizeof(char*) * count);
    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);
        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

void compare_regs(){
    int i=0;
    for(; i < MAX_REG; i++){
        if(RegSettings[i].val != RegSettingsTX[i].val){
            printf("DIFF at addr 0x%x: 0x%x | 0x%x\n", RegSettings[i].adr, RegSettings[i].val, RegSettingsTX[i].val);
        }
    }

    for(i = 0; i < MAX_EXT_REG; i++){
        if(ExtRegSettings[i].val != ExtRegSettingsTX[i].val){
            printf("DIFF at addr 0x%x: 0x%x | 0x%x\n", ExtRegSettings[i].adr, ExtRegSettings[i].val, ExtRegSettingsTX[i].val);
        }
    }
}

void print_regs(){
    int addr = 0;
    int val;
    for(; addr <= MAX_REG; addr++){
        cc1200_reg_read(addr, &val);
        printf("INFO: Addr: 0x%x | val: 0x%x\n", addr, val);
    }
    for(addr = 0; addr <= 0x39; addr++){
        cc1200_reg_read(addr | EXT_ADR, &val);
        printf("INFO: Addr: 0x%x | val: 0x%x\n", addr | EXT_ADR, val);
    }
    for(addr = 0x64; addr <= 0xA2; addr++){
        cc1200_reg_read(addr | EXT_ADR, &val);
        printf("INFO: Addr: 0x%x | val: 0x%x\n", addr | EXT_ADR, val);
    }
    for(addr = 0xD2; addr <= 0xDA; addr++){
        cc1200_reg_read(addr | EXT_ADR, &val);
        printf("INFO: Addr: 0x%x | val: 0x%x\n", addr | EXT_ADR, val);
    }
}

// IDs of nodes in Area
int nodes_area[MAX_NODES];

int spectrum[3] = {850, 870, 900};
int ffhRX[3][3];
int ffhTX[3][3];


REG_TYPE RegSettings[MAX_REG] =
{
    {IOCFG3         , 0x06 }, // GPIO3 IO Pin Configuration
    {IOCFG2         , 0x06 }, // GPIO2 IO Pin Configuration
    {IOCFG1         , 0x30 }, // GPIO1 IO Pin Configuration
    {IOCFG0         , 0x3c }, // GPIO0 IO Pin Configuration
    {SYNC3          , 0x93 }, // Sync Word Configuration [31:24]
    {SYNC2          , 0x0b }, // Sync Word Configuration [23:16]
    {SYNC1          , 0x51 }, // Sync Word Configuration [15:8]
    {SYNC0          , 0xde }, // Sync Word Configuration [7:0]
    {SYNC_CFG1      , 0xa9 }, // Sync Word Detection Configuration Reg. 1
    {SYNC_CFG0      , 0x03 }, // Sync Word Detection Configuration Reg. 0
    {DEVIATION_M    , 0x06 }, // Frequency Deviation Configuration
    {MODCFG_DEV_E   , 0x0b }, // Modulation Format and Frequency Deviation Configur..
    {DCFILT_CFG     , 0x4c }, // Digital DC Removal Configuration
    {PREAMBLE_CFG1  , 0x34 }, // Preamble Length Configuration Reg. 1 (reset);
    {PREAMBLE_CFG0  , 0x8a }, // Preamble Detection Configuration Reg. 0
    {IQIC           , 0xc8 }, // Digital Image Channel Compensation Configuration
    {CHAN_BW        , 0x10 }, // Channel Filter Configuration
    {MDMCFG1        , 0x42 }, // General Modem Parameter Configuration Reg. 1
    {MDMCFG0        , 0x05 }, // General Modem Parameter Configuration Reg. 0
    {SYMBOL_RATE2   , 0x8f }, // Symbol Rate Configuration Exponent and Mantissa [1..
    {SYMBOL_RATE1   , 0x75 }, // Symbol Rate Configuration Mantissa [15:8]
    {SYMBOL_RATE0   , 0x10 }, // Symbol Rate Configuration Mantissa [7:0]
    {AGC_REF        , 0x27 }, // AGC Reference Level Configuration
    {AGC_CS_THR     , 0xee }, // Carrier Sense Threshold Configuration
    {AGC_GAIN_ADJUST, 0x00 }, // RSSI Offset Configuration
    {AGC_CFG3       , 0xb1 }, // Automatic Gain Control Configuration Reg. 3
    {AGC_CFG2       , 0x20 }, // Automatic Gain Control Configuration Reg. 2
    {AGC_CFG1       , 0x11 }, // Automatic Gain Control Configuration Reg. 1
    {AGC_CFG0       , 0x94 }, // Automatic Gain Control Configuration Reg. 0
    {FIFO_CFG       , 0x00 }, // FIFO Configuration
    {DEV_ADDR       , 0x45 }, // Device Address Configuration
    {SETTLING_CFG   , 0x0b }, // Frequency Synthesizer Calibration and Settling Con..
    {FS_CFG         , 0x12 }, // Frequency Synthesizer Configuration
    {WOR_CFG1       , 0x08 }, // eWOR Configuration Reg. 1
    {WOR_CFG0       , 0x21 }, // eWOR Configuration Reg. 0
    {WOR_EVENT0_MSB , 0x00 }, // Event 0 Configuration MSB
    {WOR_EVENT0_LSB , 0x00 }, // Event 0 Configuration LSB
    {RXDCM_TIME     , 0x00 }, // RX Duty Cycle Mode Configuration
    {PKT_CFG2       , 0x00 }, // Packet Configuration Reg. 2 (on reset)
    {PKT_CFG1       , 0x03 }, // Packet Configuration Reg. 1 (on reset)
    {PKT_CFG0       , 0x20 }, // Packet Configuration Reg. 0 (on reset)
    {RFEND_CFG1     , 0x3f }, // RFEND Configuration Reg. 1
    {RFEND_CFG0     , 0x20 }, // RFEND Configuration Reg. 0
    {PA_CFG1        , 0x7f }, // Power Amplifier Configuration Reg. 1
    {PA_CFG0        , 0x56 }, // Power Amplifier Configuration Reg. 0
    {ASK_CFG        , 0x0f }, // ASK Configuration
    {PKT_LEN        , 0xFF }  // Packet Length Configuration
};
REG_TYPE ExtRegSettings[MAX_EXT_REG] =
{
    {IF_MIX_CFG      , 0x1c },  // IF Mix Configuration
    {FREQOFF_CFG     , 0x20 },  // Frequency Offset Correction Configuration (on Reset)
    {TOC_CFG         , 0x03 },  // Timing Offset Correction Configuration (reset value)
    {MARC_SPARE      , 0x00 },  // MARC Spare
    {ECG_CFG         , 0x00 },  // External Clock Frequency Configuration
    {MDMCFG2         , 0x02 },  // General Modem Parameter Configuration Reg. 2
    {EXT_CTRL        , 0x01 },  // External Control Configuration
    {RCCAL_FINE      , 0x00 },  // RC Oscillator Calibration Fine
    {RCCAL_COARSE    , 0x00 },  // RC Oscillator Calibration Coarse
    {RCCAL_OFFSET    , 0x00 },  // RC Oscillator Calibration Clock Offset
    {FREQOFF1        , 0x00 },  // Frequency Offset MSB
    {FREQOFF0        , 0x00 },  // Frequency Offset LSB
    {FREQ2           , 0x56 },  // Frequency Configuration [23:16]
    {FREQ1           , 0xcc },  // Frequency Configuration [15:8]
    {FREQ0           , 0xcc },  // Frequency Configuration [7:0]
    {IF_ADC2         , 0x02 },  // Analog to Digital Converter Configuration Reg. 2
    {IF_ADC1         , 0xee },  // Analog to Digital Converter Configuration Reg. 1
    {IF_ADC0         , 0x10 },  // Analog to Digital Converter Configuration Reg. 0
    {FS_DIG1         , 0x07 },  // Frequency Synthesizer Digital Reg. 1
    {FS_DIG0         , 0xaf },  // Frequency Synthesizer Digital Reg. 0
    {FS_CAL3         , 0x00 },  // Frequency Synthesizer Calibration Reg. 3
    {FS_CAL2         , 0x20 },  // Frequency Synthesizer Calibration Reg. 2
    {FS_CAL1         , 0x40 },  // Frequency Synthesizer Calibration Reg. 1
    {FS_CAL0         , 0x0e },  // Frequency Synthesizer Calibration Reg. 0
    {FS_CHP          , 0x28 },  // Frequency Synthesizer Charge Pump Configuration
    {FS_DIVTWO       , 0x03 },  // Frequency Synthesizer Divide by 2
    {FS_DSM1         , 0x00 },  // FS Digital Synthesizer Module Configuration Reg. 1
    {FS_DSM0         , 0x33 },  // FS Digital Synthesizer Module Configuration Reg. 0
    {FS_DVC1         , 0xff },  // Frequency Synthesizer Divider Chain Configuration ..
    {FS_DVC0         , 0x17 },  // Frequency Synthesizer Divider Chain Configuration ..
    {FS_LBI          , 0x00 },  // Frequency Synthesizer Local Bias Configuration
    {FS_PFD          , 0x00 },  // Frequency Synthesizer Phase Frequency Detector Con..
    {FS_PRE          , 0x6e },  // Frequency Synthesizer Prescaler Configuration
    {FS_REG_DIV_CML  , 0x1c },  // Frequency Synthesizer Divider Regulator Configurat..
    {FS_SPARE        , 0xac },  // Frequency Synthesizer Spare
    {FS_VCO4         , 0x14 },  // FS Voltage Controlled Oscillator Configuration Reg..
    {FS_VCO3         , 0x00 },  // FS Voltage Controlled Oscillator Configuration Reg..
    {FS_VCO2         , 0x00 },  // FS Voltage Controlled Oscillator Configuration Reg..
    {FS_VCO1         , 0x00 },  // FS Voltage Controlled Oscillator Configuration Reg..
    {FS_VCO0         , 0xb5 },  // FS Voltage Controlled Oscillator Configuration Reg..
    {GBIAS6          , 0x00 },  // Global Bias Configuration Reg. 6
    {GBIAS5          , 0x02 },  // Global Bias Configuration Reg. 5
    {GBIAS4          , 0x00 },  // Global Bias Configuration Reg. 4
    {GBIAS3          , 0x00 },  // Global Bias Configuration Reg. 3
    {GBIAS2          , 0x10 },  // Global Bias Configuration Reg. 2
    {GBIAS1          , 0x00 },  // Global Bias Configuration Reg. 1
    {GBIAS0          , 0x00 },  // Global Bias Configuration Reg. 0
    {IFAMP           , 0x09 },  // Intermediate Frequency Amplifier Configuration
    {LNA             , 0x01 },  // Low Noise Amplifier Configuration
    {RXMIX           , 0x01 },  // RX Mixer Configuration
    {XOSC5           , 0x0e },  // Crystal Oscillator Configuration Reg. 5
    {XOSC4           , 0xa0 },  // Crystal Oscillator Configuration Reg. 4
    {XOSC3           , 0x03 },  // Crystal Oscillator Configuration Reg. 3
    {XOSC2           , 0x04 },  // Crystal Oscillator Configuration Reg. 2
    {XOSC1           , 0x03 },  // Crystal Oscillator Configuration Reg. 1
    {XOSC0           , 0x00 },  // Crystal Oscillator Configuration Reg. 0
    {ANALOG_SPARE    , 0x00 },  // Analog Spare
    {PA_CFG3         , 0x00 },  // Power Amplifier Configuration Reg. 3
    {WOR_TIME1       , 0x00 },  // eWOR Timer Counter Value MSB
    {WOR_TIME0       , 0x00 },  // eWOR Timer Counter Value LSB
    {WOR_CAPTURE1    , 0x00 },  // eWOR Timer Capture Value MSB
    {WOR_CAPTURE0    , 0x00 },  // eWOR Timer Capture Value LSB
    {BIST            , 0x00 },  // MARC Built-In Self-Test
    {DCFILTOFFSET_I1 , 0x00 },  // DC Filter Offset I MSB
    {DCFILTOFFSET_I0 , 0x00 },  // DC Filter Offset I LSB
    {DCFILTOFFSET_Q1 , 0x00 },  // DC Filter Offset Q MSB
    {DCFILTOFFSET_Q0 , 0x00 },  // DC Filter Offset Q LSB
    {IQIE_I1         , 0x00 },  // IQ Imbalance Value I MSB
    {IQIE_I0         , 0x00 },  // IQ Imbalance Value I LSB
    {IQIE_Q1         , 0x00 },  // IQ Imbalance Value Q MSB
    {IQIE_Q0         , 0x00 },  // IQ Imbalance Value Q LSB
    {RSSI1           , 0x80 },  // Received Signal Strength Indicator Reg. 1
    {RSSI0           , 0x00 },  // Received Signal Strength Indicator Reg.0
    {MARCSTATE       , 0x41 },  // MARC State
    {LQI_VAL         , 0x00 },  // Link Quality Indicator Value
    {PQT_SYNC_ERR    , 0xff },  // Preamble and Sync Word Error
    {DEM_STATUS      , 0x00 },  // Demodulator Status
    {FREQOFF_EST1    , 0x00 },  // Frequency Offset Estimate MSB
    {FREQOFF_EST0    , 0x00 },  // Frequency Offset Estimate LSB
    {AGC_GAIN3       , 0x00 },  // Automatic Gain Control Reg. 3
    {AGC_GAIN2       , 0xd1 },  // Automatic Gain Control Reg. 2
    {AGC_GAIN1       , 0x00 },  // Automatic Gain Control Reg. 1
    {AGC_GAIN0       , 0x3f },  // Automatic Gain Control Reg. 0
    {CFM_RX_DATA_OUT , 0x00 },  // Custom Frequency Modulation RX Data
    {CFM_TX_DATA_IN  , 0x00 },  // Custom Frequency Modulation TX Data
    {ASK_SOFT_RX_DATA, 0x30 },  //:ASK Soft Decision Output
    {RNDGEN          , 0x7f },  // Random Number Generator Value
    {MAGN2           , 0x00 },  // Signal Magnitude after CORDIC [16]
    {MAGN1           , 0x00 },  // Signal Magnitude after CORDIC [15:8]
    {MAGN0           , 0x00 },  // Signal Magnitude after CORDIC [7:0]
    {ANG1            , 0x00 },  // Signal Angular after CORDIC [9:8]
    {ANG0            , 0x00 },  // Signal Angular after CORDIC [7:0]
    {CHFILT_I2       , 0x02 },  // Channel Filter Data Real Part [16]
    {CHFILT_I1       , 0x00 },  // Channel Filter Data Real Part [15:8]
    {CHFILT_I0       , 0x00 },  // Channel Filter Data Real Part [7:0]
    {CHFILT_Q2       , 0x00 },  // Channel Filter Data Imaginary Part [16]
    {CHFILT_Q1       , 0x00 },  // Channel Filter Data Imaginary Part [15:8]
    {CHFILT_Q0       , 0x00 },  // Channel Filter Data Imaginary Part [7:0]
    {GPIO_STATUS     , 0x00 },  // General Purpose Input/Output Status
    {FSCAL_CTRL      , 0x01 },  // Frequency Synthesizer Calibration Control
    {PHASE_ADJUST    , 0x00 },  // Frequency Synthesizer Phase Adjust
    {PARTNUMBER      , 0x20 },  // Part Number
    {PARTVERSION     , 0x11 },  // Part Revision
    {SERIAL_STATUS   , 0x00 },  // Serial Status
    {MODEM_STATUS1   , 0x10 },  // Modem Status Reg. 1
    {MODEM_STATUS0   , 0x00 },  // Modem Status Reg. 0
    {MARC_STATUS1    , 0x00 },  // MARC Status Reg. 1
    {MARC_STATUS0    , 0x00 },  // MARC Status Reg. 0
    {PA_IFAMP_TEST   , 0x00 },  // Power Amplifier Intermediate Frequency Amplifier T..
    {FSRF_TEST       , 0x00 },  // Frequency Synthesizer Test
    {PRE_TEST        , 0x00 },  // Frequency Synthesizer Prescaler Test
    {PRE_OVR         , 0x00 },  // Frequency Synthesizer Prescaler Override
    {ADC_TEST        , 0x00 },  // Analog to Digital Converter Test
    {DVC_TEST        , 0x0b },  // Digital Divider Chain Test
    {ATEST           , 0x40 },  // Analog Test
    {ATEST_LVDS      , 0x00 },  // Analog Test LVDS
    {ATEST_MODE      , 0x00 },  // Analog Test Mode
    {XOSC_TEST1      , 0x3c },  // Crystal Oscillator Test Reg. 1
    {XOSC_TEST0      , 0x00 },  // Crystal Oscillator Test Reg. 0
    {AES             , 0x00 },  // AES
    {MDM_TEST        , 0x00 },  // MODEM Test
    {RXFIRST         , 0x00 },  // RX FIFO Pointer First Entry
    {TXFIRST         , 0x00 },  // TX FIFO Pointer First Entry
    {RXLAST          , 0x00 },  // RX FIFO Pointer Last Entry
    {TXLAST          , 0x00 },  // TX FIFO Pointer Last Entry
    {NUM_TXBYTES     , 0x00 },  // TX FIFO Status
    {NUM_RXBYTES     , 0x00 },  // RX FIFO Status
    {FIFO_NUM_TXBYTES, 0x0f },  //:TX FIFO Status
    {FIFO_NUM_RXBYTES, 0x00 },  //:RX FIFO Status
    {RXFIFO_PRE_BUF  , 0x00 }  // RX FIFO Status
};


REG_TYPE RegSettingsTX[MAX_REG] =
{
    {IOCFG3         , 0x06 }, // GPIO3 IO Pin Configuration
    {IOCFG2         , 0x06 }, // GPIO2 IO Pin Configuration
    {IOCFG1         , 0x30 }, // GPIO1 IO Pin Configuration
    {IOCFG0         , 0x3c }, // GPIO0 IO Pin Configuration
    {SYNC3          , 0x93 }, // Sync Word Configuration [31:24]
    {SYNC2          , 0x0b }, // Sync Word Configuration [23:16]
    {SYNC1          , 0x51 }, // Sync Word Configuration [15:8]
    {SYNC0          , 0xde }, // Sync Word Configuration [7:0]
    {SYNC_CFG1      , 0xa9 }, // Sync Word Detection Configuration Reg. 1
    {SYNC_CFG0      , 0x03 }, // Sync Word Detection Configuration Reg. 0
    {DEVIATION_M    , 0x06 }, // Frequency Deviation Configuration
    {MODCFG_DEV_E   , 0x0b }, // Modulation Format and Frequency Deviation Configur..
    {DCFILT_CFG     , 0x4c }, // Digital DC Removal Configuration
    {PREAMBLE_CFG1  , 0x34 }, // Preamble Length Configuration Reg. 1 (reset);
    {PREAMBLE_CFG0  , 0x8a }, // Preamble Detection Configuration Reg. 0
    {IQIC           , 0xc8 }, // Digital Image Channel Compensation Configuration
    {CHAN_BW        , 0x10 }, // Channel Filter Configuration
    {MDMCFG1        , 0x42 }, // General Modem Parameter Configuration Reg. 1
    {MDMCFG0        , 0x05 }, // General Modem Parameter Configuration Reg. 0
    {SYMBOL_RATE2   , 0x8f }, // Symbol Rate Configuration Exponent and Mantissa [1..
    {SYMBOL_RATE1   , 0x75 }, // Symbol Rate Configuration Mantissa [15:8]
    {SYMBOL_RATE0   , 0x10 }, // Symbol Rate Configuration Mantissa [7:0]
    {AGC_REF        , 0x27 }, // AGC Reference Level Configuration
    {AGC_CS_THR     , 0xee }, // Carrier Sense Threshold Configuration
    {AGC_GAIN_ADJUST, 0x00 }, // RSSI Offset Configuration
    {AGC_CFG3       , 0xb1 }, // Automatic Gain Control Configuration Reg. 3
    {AGC_CFG2       , 0x20 }, // Automatic Gain Control Configuration Reg. 2
    {AGC_CFG1       , 0x11 }, // Automatic Gain Control Configuration Reg. 1
    {AGC_CFG0       , 0x94 }, // Automatic Gain Control Configuration Reg. 0
    {FIFO_CFG       , 0x00 }, // FIFO Configuration
    {DEV_ADDR       , 0x45 }, // Device Address Configuration
    {SETTLING_CFG   , 0x0b }, // Frequency Synthesizer Calibration and Settling Con..
    {FS_CFG         , 0x12 }, // Frequency Synthesizer Configuration
    {WOR_CFG1       , 0x08 }, // eWOR Configuration Reg. 1
    {WOR_CFG0       , 0x21 }, // eWOR Configuration Reg. 0
    {WOR_EVENT0_MSB , 0x00 }, // Event 0 Configuration MSB
    {WOR_EVENT0_LSB , 0x00 }, // Event 0 Configuration LSB
    {RXDCM_TIME     , 0x00 }, // RX Duty Cycle Mode Configuration
    {PKT_CFG2       , 0x00 }, // Packet Configuration Reg. 2 (on reset)
    {PKT_CFG1       , 0x03 }, // Packet Configuration Reg. 1 (on reset)
    {PKT_CFG0       , 0x20 }, // Packet Configuration Reg. 0 (on reset)
    {RFEND_CFG1     , 0x3f }, // RFEND Configuration Reg. 1
    {RFEND_CFG0     , 0x20 }, // RFEND Configuration Reg. 0
    {PA_CFG1        , 0x7f }, // Power Amplifier Configuration Reg. 1
    {PA_CFG0        , 0x56 }, // Power Amplifier Configuration Reg. 0
    {ASK_CFG        , 0x0f }, // ASK Configuration
    {PKT_LEN        , 0xFF }  // Packet Length Configuration
};
REG_TYPE ExtRegSettingsTX[MAX_EXT_REG] =
{
    {IF_MIX_CFG      , 0x1c },  // IF Mix Configuration
    {FREQOFF_CFG     , 0x20 },  // Frequency Offset Correction Configuration (on Reset)
    {TOC_CFG         , 0x03 },  // Timing Offset Correction Configuration (reset value)
    {MARC_SPARE      , 0x00 },  // MARC Spare
    {ECG_CFG         , 0x00 },  // External Clock Frequency Configuration
    {MDMCFG2         , 0x02 },  // General Modem Parameter Configuration Reg. 2
    {EXT_CTRL        , 0x01 },  // External Control Configuration
    {RCCAL_FINE      , 0x00 },  // RC Oscillator Calibration Fine
    {RCCAL_COARSE    , 0x00 },  // RC Oscillator Calibration Coarse
    {RCCAL_OFFSET    , 0x00 },  // RC Oscillator Calibration Clock Offset
    {FREQOFF1        , 0x00 },  // Frequency Offset MSB
    {FREQOFF0        , 0x00 },  // Frequency Offset LSB
    {FREQ2           , 0x55 },  // Frequency Configuration [23:16]
    {FREQ1           , 0x00 },  // Frequency Configuration [15:8]
    {FREQ0           , 0x00 },  // Frequency Configuration [7:0]
    {IF_ADC2         , 0x02 },  // Analog to Digital Converter Configuration Reg. 2
    {IF_ADC1         , 0xee },  // Analog to Digital Converter Configuration Reg. 1
    {IF_ADC0         , 0x10 },  // Analog to Digital Converter Configuration Reg. 0
    {FS_DIG1         , 0x04 },  // Frequency Synthesizer Digital Reg. 1
    {FS_DIG0         , 0x50 },  // Frequency Synthesizer Digital Reg. 0
    {FS_CAL3         , 0x00 },  // Frequency Synthesizer Calibration Reg. 3
    {FS_CAL2         , 0x20 },  // Frequency Synthesizer Calibration Reg. 2
    {FS_CAL1         , 0x40 },  // Frequency Synthesizer Calibration Reg. 1
    {FS_CAL0         , 0x0e },  // Frequency Synthesizer Calibration Reg. 0
    {FS_CHP          , 0x28 },  // Frequency Synthesizer Charge Pump Configuration
    {FS_DIVTWO       , 0x03 },  // Frequency Synthesizer Divide by 2
    {FS_DSM1         , 0x00 },  // FS Digital Synthesizer Module Configuration Reg. 1
    {FS_DSM0         , 0x33 },  // FS Digital Synthesizer Module Configuration Reg. 0
    {FS_DVC1         , 0xf7 },  // Frequency Synthesizer Divider Chain Configuration ..
    {FS_DVC0         , 0x0f },  // Frequency Synthesizer Divider Chain Configuration ..
    {FS_LBI          , 0x00 },  // Frequency Synthesizer Local Bias Configuration
    {FS_PFD          , 0x00 },  // Frequency Synthesizer Phase Frequency Detector Con..
    {FS_PRE          , 0x6e },  // Frequency Synthesizer Prescaler Configuration
    {FS_REG_DIV_CML  , 0x1c },  // Frequency Synthesizer Divider Regulator Configurat..
    {FS_SPARE        , 0xac },  // Frequency Synthesizer Spare
    {FS_VCO4         , 0x14 },  // FS Voltage Controlled Oscillator Configuration Reg..
    {FS_VCO3         , 0x00 },  // FS Voltage Controlled Oscillator Configuration Reg..
    {FS_VCO2         , 0x00 },  // FS Voltage Controlled Oscillator Configuration Reg..
    {FS_VCO1         , 0x00 },  // FS Voltage Controlled Oscillator Configuration Reg..
    {FS_VCO0         , 0xb5 },  // FS Voltage Controlled Oscillator Configuration Reg..
    {GBIAS6          , 0x00 },  // Global Bias Configuration Reg. 6
    {GBIAS5          , 0x02 },  // Global Bias Configuration Reg. 5
    {GBIAS4          , 0x00 },  // Global Bias Configuration Reg. 4
    {GBIAS3          , 0x00 },  // Global Bias Configuration Reg. 3
    {GBIAS2          , 0x10 },  // Global Bias Configuration Reg. 2
    {GBIAS1          , 0x00 },  // Global Bias Configuration Reg. 1
    {GBIAS0          , 0x00 },  // Global Bias Configuration Reg. 0
    {IFAMP           , 0x09 },  // Intermediate Frequency Amplifier Configuration
    {LNA             , 0x01 },  // Low Noise Amplifier Configuration
    {RXMIX           , 0x01 },  // RX Mixer Configuration
    {XOSC5           , 0x0e },  // Crystal Oscillator Configuration Reg. 5
    {XOSC4           , 0xa0 },  // Crystal Oscillator Configuration Reg. 4
    {XOSC3           , 0x03 },  // Crystal Oscillator Configuration Reg. 3
    {XOSC2           , 0x04 },  // Crystal Oscillator Configuration Reg. 2
    {XOSC1           , 0x03 },  // Crystal Oscillator Configuration Reg. 1
    {XOSC0           , 0x00 },  // Crystal Oscillator Configuration Reg. 0
    {ANALOG_SPARE    , 0x00 },  // Analog Spare
    {PA_CFG3         , 0x00 },  // Power Amplifier Configuration Reg. 3
    {WOR_TIME1       , 0x00 },  // eWOR Timer Counter Value MSB
    {WOR_TIME0       , 0x00 },  // eWOR Timer Counter Value LSB
    {WOR_CAPTURE1    , 0x00 },  // eWOR Timer Capture Value MSB
    {WOR_CAPTURE0    , 0x00 },  // eWOR Timer Capture Value LSB
    {BIST            , 0x00 },  // MARC Built-In Self-Test
    {DCFILTOFFSET_I1 , 0x00 },  // DC Filter Offset I MSB
    {DCFILTOFFSET_I0 , 0x00 },  // DC Filter Offset I LSB
    {DCFILTOFFSET_Q1 , 0x00 },  // DC Filter Offset Q MSB
    {DCFILTOFFSET_Q0 , 0x00 },  // DC Filter Offset Q LSB
    {IQIE_I1         , 0x00 },  // IQ Imbalance Value I MSB
    {IQIE_I0         , 0x00 },  // IQ Imbalance Value I LSB
    {IQIE_Q1         , 0x00 },  // IQ Imbalance Value Q MSB
    {IQIE_Q0         , 0x00 },  // IQ Imbalance Value Q LSB
    {RSSI1           , 0x80 },  // Received Signal Strength Indicator Reg. 1
    {RSSI0           , 0x00 },  // Received Signal Strength Indicator Reg.0
    {MARCSTATE       , 0x41 },  // MARC State
    {LQI_VAL         , 0x00 },  // Link Quality Indicator Value
    {PQT_SYNC_ERR    , 0xff },  // Preamble and Sync Word Error
    {DEM_STATUS      , 0x00 },  // Demodulator Status
    {FREQOFF_EST1    , 0x00 },  // Frequency Offset Estimate MSB
    {FREQOFF_EST0    , 0x00 },  // Frequency Offset Estimate LSB
    {AGC_GAIN3       , 0x00 },  // Automatic Gain Control Reg. 3
    {AGC_GAIN2       , 0xd1 },  // Automatic Gain Control Reg. 2
    {AGC_GAIN1       , 0x00 },  // Automatic Gain Control Reg. 1
    {AGC_GAIN0       , 0x3f },  // Automatic Gain Control Reg. 0
    {CFM_RX_DATA_OUT , 0x00 },  // Custom Frequency Modulation RX Data
    {CFM_TX_DATA_IN  , 0x00 },  // Custom Frequency Modulation TX Data
    {ASK_SOFT_RX_DATA, 0x30 },  //:ASK Soft Decision Output
    {RNDGEN          , 0x7f },  // Random Number Generator Value
    {MAGN2           , 0x00 },  // Signal Magnitude after CORDIC [16]
    {MAGN1           , 0x00 },  // Signal Magnitude after CORDIC [15:8]
    {MAGN0           , 0x00 },  // Signal Magnitude after CORDIC [7:0]
    {ANG1            , 0x00 },  // Signal Angular after CORDIC [9:8]
    {ANG0            , 0x00 },  // Signal Angular after CORDIC [7:0]
    {CHFILT_I2       , 0x02 },  // Channel Filter Data Real Part [16]
    {CHFILT_I1       , 0x00 },  // Channel Filter Data Real Part [15:8]
    {CHFILT_I0       , 0x00 },  // Channel Filter Data Real Part [7:0]
    {CHFILT_Q2       , 0x00 },  // Channel Filter Data Imaginary Part [16]
    {CHFILT_Q1       , 0x00 },  // Channel Filter Data Imaginary Part [15:8]
    {CHFILT_Q0       , 0x00 },  // Channel Filter Data Imaginary Part [7:0]
    {GPIO_STATUS     , 0x00 },  // General Purpose Input/Output Status
    {FSCAL_CTRL      , 0x01 },  // Frequency Synthesizer Calibration Control
    {PHASE_ADJUST    , 0x00 },  // Frequency Synthesizer Phase Adjust
    {PARTNUMBER      , 0x20 },  // Part Number
    {PARTVERSION     , 0x11 },  // Part Revision
    {SERIAL_STATUS   , 0x00 },  // Serial Status
    {MODEM_STATUS1   , 0x10 },  // Modem Status Reg. 1
    {MODEM_STATUS0   , 0x00 },  // Modem Status Reg. 0
    {MARC_STATUS1    , 0x00 },  // MARC Status Reg. 1
    {MARC_STATUS0    , 0x00 },  // MARC Status Reg. 0
    {PA_IFAMP_TEST   , 0x00 },  // Power Amplifier Intermediate Frequency Amplifier T..
    {FSRF_TEST       , 0x00 },  // Frequency Synthesizer Test
    {PRE_TEST        , 0x00 },  // Frequency Synthesizer Prescaler Test
    {PRE_OVR         , 0x00 },  // Frequency Synthesizer Prescaler Override
    {ADC_TEST        , 0x00 },  // Analog to Digital Converter Test
    {DVC_TEST        , 0x0b },  // Digital Divider Chain Test
    {ATEST           , 0x40 },  // Analog Test
    {ATEST_LVDS      , 0x00 },  // Analog Test LVDS
    {ATEST_MODE      , 0x00 },  // Analog Test Mode
    {XOSC_TEST1      , 0x3c },  // Crystal Oscillator Test Reg. 1
    {XOSC_TEST0      , 0x00 },  // Crystal Oscillator Test Reg. 0
    {AES             , 0x00 },  // AES
    {MDM_TEST        , 0x00 },  // MODEM Test
    {RXFIRST         , 0x00 },  // RX FIFO Pointer First Entry
    {TXFIRST         , 0x00 },  // TX FIFO Pointer First Entry
    {RXLAST          , 0x00 },  // RX FIFO Pointer Last Entry
    {TXLAST          , 0x00 },  // TX FIFO Pointer Last Entry
    {NUM_TXBYTES     , 0x00 },  // TX FIFO Status
    {NUM_RXBYTES     , 0x00 },  // RX FIFO Status
    {FIFO_NUM_TXBYTES, 0x0f },  //:TX FIFO Status
    {FIFO_NUM_RXBYTES, 0x00 },  //:RX FIFO Status
    {RXFIFO_PRE_BUF  , 0x00 }  // RX FIFO Status
};
