#include "CC1200_lib.h"

#pragma exit spi_shutdown

HashTable* archive;
int freq_index = 0;
int nodes_cnt = 0;
llist* backpack[MAX_NODES];
uint8_t NODE_ID;
dataframe wait_pkt;
bool IS_MULE;
uint32_t bitmap = 0;

int main(int argc, char* argv[]){
    if(argc != 3){
        fprintf(stderr, "Only give ID and if its mule \n");
        exit(0);
    }
    
    NODE_ID = (uint8_t) atoi(argv[1]);
    !strcmp(argv[2], "true") ? (IS_MULE = true) : (IS_MULE = false);
    printf("[INIT]: Node ID is: %u and node type is %s\n", NODE_ID, IS_MULE ? "Mule" : "Mote");
    archive = create_table(CAPACITY);

    int i = 0;
    for(; i < MAX_NODES; i++)
        backpack[i] = llist_create(NULL);
    i = 0;
    if(spi_init()){
        printf("[ERROR]: Init failed\n");
        return -1;
    }
    cc1200_cmd(SRES);
    cc1200_init();

    fd_set rdfs;
    struct timeval tv;
    FD_ZERO(&rdfs);
    FD_SET(0, &rdfs);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
   
    int RX_LEN;
    size_t msg_size = 50;
    size_t checklist_size = 20;
    char* msg = (char*) malloc(msg_size * sizeof(char));
    time_t t_old_disc = time(NULL);
    time_t t_old_check = time(NULL);
    change_mode(CALRX);
    if(IS_MULE) printf("Please enter the target mote ID and the respective message.\nFormat: ID message\n");
    #ifdef DEBUGGER_MULE
    if(NODE_ID == 8){
        uint8_t i, j;
        for(i = 0; i < MAX_NODES; i++){
            for(j = 0; j < MSG_NUM / MAX_NODES; j++){
                char* msg = (char*) malloc(3);
                itoa(j, msg);
                update_backpack(msg, (uint8_t) i);
                free(msg);
            }
        }
    }
    #endif
    #ifdef DEBUGGER_MOTE
        if(NODE_ID == 8){
            uint8_t j;
            for(j = 0; j < MSG_NUM; j++){
                    char* msg = (char*) malloc(4);
                    itoa(j, msg);
                    update_backpack(msg, 5);
                    free(msg);
                }
        }
    #endif

    while(1){
        FD_ZERO(&rdfs);
        FD_SET(0, &rdfs);
        cc1200_reg_read(NUM_RXBYTES, &RX_LEN);

        if(RX_LEN > 0){
            // signal detected -> perform tasks with message
            receive_packets_variable();
	    }

        /* check after deadline if every node on checklist updated */
        if(difftime(time(NULL), t_old_check) > DEADLINE){
            i = 0;
            for(; i < MAX_NODES; i++){
                node* head = *backpack[i];
                if(head && head->data != NULL)
                    fprintf(stderr, "[ERROR]: NODE %i WAS NOT UPDATED\r\n PLEASE DETERMINE ERROR AND DO A MANUAL UPDATE \n", i);
            }
            t_old_check = time(NULL);
        }

        if(existing_messages()){
            if(difftime(time(NULL), t_old_disc) > DISCOVERY_PERIOD){
                #ifdef EVAL
                    printf("[INFO]: Discover message broadcasted, next message is sent in %i seconds\n \n", DISCOVERY_PERIOD);
                #endif
                discover_nodes();
                change_mode(CALRX);
                t_old_disc = time(NULL);
            }
            if(nodes_cnt > 0){
                // go through discovered nodes (backwards) and update those who aren't already
                int index = 0;
                clock_t start = clock();
                while(!full_set(bitmap, nodes_cnt)){
                    int dst = nodes_area[index];
                    node* head = *backpack[dst];
                    if(head && head->data != NULL){
                        #ifdef EVAL
                            printf("[INFO]: Sending %s to mote %i \n", head->data, dst);
                        #endif
                        send_msg(dst, (char*) head->data);
                        change_mode(CALRX);
                        wait_for_reception();
                        receive_packets_variable();
                    }else{
                        bitmap = set_bit(bitmap, index);
                    }
                    index < (nodes_cnt - 1) ? index++ : index == 0; 
                }
                printf("Took %lf secs to send 200 messages \n", (double) (clock() - start) / CLOCKS_PER_SEC);
                nodes_cnt = 0;
            }
        }
        
        /* 
            check if user has input message to send into stdin
            if so, read msg and add to backpack
        */
        if(select(1, &rdfs, NULL, NULL, &tv)){
            memset(msg, 0, msg_size);
            getline(&msg, &msg_size, stdin);

            char** tokens = (char**) split_String(msg, ' ');
            int target_ID;
            char* data;
            if (tokens){
                target_ID = atoi(*(tokens));
                data = *(tokens + 1);
                free(tokens);
            }
            if(target_ID == 255){
                for(i = 0; i < MAX_NODES; i++){
                    update_backpack(data, i);        
                }
            }else if(target_ID >= MAX_NODES){
                fprintf(stderr, "[ERROR]: Please enter valid node ID \n");
            }else
                update_backpack(data, target_ID);
        
        }else if(select(1, &rdfs, NULL, NULL, &tv) == -1){
            perror("[ERROR]: select()\n");
        }
        
        tv.tv_sec = 0;
        tv.tv_usec = 0;
    }

    free(msg);
    free(wait_pkt.payload);

    spi_shutdown();
}
