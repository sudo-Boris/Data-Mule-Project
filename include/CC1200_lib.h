#ifndef CC1200_LIB_H
#define CC1200_LIB_H

#include <SPIv1.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include "hashTable.h"
#include "helper_functions.h"

#define CALRX           0
#define CAL             1
#define CALTX           2
#define CALIDLE         3
#define INITTX          4
#define INITRX          5
#define LOW_RSSI_THRES  35
#define HIGH_RSSI_THRES 65
#define BROADCAST       0xFF
/**
    @brief Initializes cc1200 with correct register settings
    and input mode
*/
int cc1200_init();

/** 
    @brief Waits untill the status change is fullfilled
*/
void wait_cmd(CC1200_STATES cmd);

/**
    @brief Takes input frequency_index and correctly wirtes
    correspondig frequency into the respective registers
*/
void change_mode(uint8_t calibrate);

/**
 * NOT USED!
 * @brief Prints the current frequency in MHz
*/
void print_freq();

/**
    @brief Receiver funcionality.

    Receives stream of packets with variable length
    as long as the FIFO isn't empty
*/
void receive_packets_variable();

/**
    @brief Adds msg for target mote (dst) to backpack
    @param msg message to send to target
    @param dest ID of mote that the message was designated to
*/
void update_backpack(char* msg, uint8_t dest);

/**
    @brief Transmitting end of synchronization protocol.

    Sends own, unflagged backpack messages, message by message
    and waits for acknowledgement
    - if IN_ARCHIVE answer, move respective message to archive
    - else continue
*/
void send_mule_backpack(uint8_t src);

/**
    @brief Blocking the transmission from any other node besides
    the one he is currently synching his backpack with
    @return Dataframe from relevant node

*/
dataframe block_transmission();

/**
    @brief Receiving end of synchronization protocol.

    Waits for backpack content from transmitting end and responds 
    respective with acknowledgement
    - if message already in archive, send MULE_ACK with IN_ARCHIVE flag
    - if in own backpack, continue
    - if not in archive and not in own backpack, add to own backpack
*/
void receive_mule_backpack();

/** 
    @brief Function that calls the iniatialization of the backpack
    synchronizatin between two mules.

    @param initiator which mule begins with the sending
    @param src ID of mule with which to synchronize backpacks with
*/
void sync_backpacks(bool initiator, uint8_t src);

/**
    @brief Sends out data frame - used for protocol execution
    @param frame - frame to send
*/
void send_frame(dataframe frame);

/**
    @brief Takes message to send to target, constructs frame and sends it
    @param dst target ID
    @param msg message to send
*/
void send_msg(uint8_t dst, char* msg);

/**
    @brief Creates search msg and broadcasts it
*/
void discover_nodes();

/**
    @brief Waits for the TX fifo to empty out before
    continueing execution
*/
void wait_fifo_empty();

/**  NOT USED!
    @brief Drops packet of length pkt_length.
    @param pkt_length length of packet to drop
*/
void drop_pkt(int pkt_length);

/**  NOT USED!
    @brief Checks whether the Packet is directed to us
    If yes: keep the packet
    else remove it from fifo
    @param addr Address byte in packet
    @param pkt_length How many bytes to flush from fifo
    @return bool if packet is for us or not
*/ 
bool check_addr(uint8_t addr, uint8_t pkt_length);

/**
    @brief Reads all bytes out of FIFO to construct dataframe
    @return constructed dataframe from FIFO
*/
dataframe decode_message();

/**
    @brief Waits until message is recieved (until there is a byte in the FIFO)
    and the RSSI value is healthy
*/
void wait_for_reception();

/**  NOT USED!
    @brief Receiver funcionality.

    Save the values from FS_CHP, FS_CV02, FS_CV04 registers for each frequency
    to enable Fast Frequency Hoping
*/
void freq_calibrationRX();

/**  NOT USED!
    @brief Transmitter funcionality.

    Save the values from FS_CHP, FS_CV02, FS_CV04 registers for each freq
    to enable Fast Frequency Hoping
*/
void freq_calibrationTX();

/**
    @brief Searches through entire backpack and looks if there is one message
    @return true when one message exists - else false
*/
bool existing_messages();

/**
    @brief Splits string at delimiter
    @param a_str string to split
    @param a_delim character at which the string is split
    @return string array with all substrings
*/
char** split_String(char* a_str, const char a_delim);

void compare_regs();
void print_regs();

// node in the area with it's direction
extern int nodes_area[MAX_NODES];
extern int ffhRX[3][3];
extern int ffhTX[3][3];

extern int freq_index;
extern int nodes_cnt;
extern uint8_t NODE_ID;

extern llist* backpack[MAX_NODES];

extern dataframe wait_pkt;

// checklist of nodes to be updated
// index corresponds to ID, value (0, 1) corresponds to updated or not
extern int checklist[MAX_NODES];

#endif // CC1200_LIB_H