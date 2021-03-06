#ifndef JOS_KERN_E1002_H
#define JOS_KERN_E1000_H
#include <kern/pci.h>
#include <kern/sched.h>

extern uint8_t e1000_irq;

// Kernel functions
int E1000_attach(struct pci_func *pcif);
int E1000_transmit(void * data_addr, uint16_t length);
int E1000_receive(void * data_addr, uint16_t *len_store);
uint64_t E1000_get_macaddr();



void e1000_trap_handler(void);

#define E1000_TXDARR_LEN     32 /* Length of the transmit descriptor ring */

/* Transmit Descriptor */
struct e1000_tx_desc {
    uint64_t buffer_addr;       /* Address of the descriptor's data buffer */
    uint16_t length;    /* Data buffer length */
    uint8_t cso;        /* Checksum offset */
    uint8_t cmd;        /* Descriptor control */
    uint8_t status;     /* Descriptor status */
    uint8_t css;        /* Checksum start */
    uint16_t special;
};

/* Transmit Descriptor bit definitions */
#define E1000_TXD_DTYP_D     0x00100000         /* Data Descriptor */
#define E1000_TXD_DTYP_C     0x00000000         /* Context Descriptor */
#define E1000_TXD_POPTS_IXSM (uint8_t)0x01      /* Insert IP checksum */
#define E1000_TXD_POPTS_TXSM (uint8_t)0x02      /* Insert TCP/UDP checksum */
#define E1000_TXD_CMD_EOP    (uint8_t)0x01      /* End of Packet */
#define E1000_TXD_CMD_IFCS   (uint8_t)0x02      /* Insert FCS (Ethernet CRC) */
#define E1000_TXD_CMD_IC     (uint8_t)0x04      /* Insert Checksum */
#define E1000_TXD_CMD_TSE    (uint8_t)0x04      /* TCP Seg enable */
#define E1000_TXD_CMD_RS     (uint8_t)0x08      /* Report Status */
#define E1000_TXD_CMD_RPS    (uint8_t)0x10      /* Report Packet Sent */
#define E1000_TXD_CMD_DEXT   (uint8_t)0x20      /* Descriptor extension (0 = legacy) */
#define E1000_TXD_CMD_VLE    (uint8_t)0x40      /* Add VLAN tag */
#define E1000_TXD_CMD_IDE    (uint8_t)0x80      /* Enable Tidv register */
#define E1000_TXD_STAT_DD    (uint8_t)0x01      /* Descriptor Done */
#define E1000_TXD_STAT_EC    (uint8_t)0x02      /* Excess Collisions */
#define E1000_TXD_STAT_LC    (uint8_t)0x04      /* Late Collisions */
#define E1000_TXD_STAT_TC    (uint8_t)0x04      /* Tx Underrun */
#define E1000_TXD_STAT_TU    (uint8_t)0x08      /* Transmit underrun */
#define E1000_TXD_CMD_TCP    (uint8_t)0x01      /* TCP packet */
#define E1000_TXD_CMD_IP     (uint8_t)0x02      /* IP packet */
/* Transmit Control */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x00000100    /* collision threshold */
#define E1000_TCTL_COLD   0x00040000    /* collision distance */
/* Transmit IPG Register */
#define E1000_TIPG_IPGT   10
#define E1000_TIPG_IPGR1  8<<10
#define E1000_TIPG_IPGR2  6<<20


#define E1000_RXDARR_LEN     128
/* Receive Descriptor */
struct e1000_rx_desc {
    uint64_t buffer_addr; /* Address of the descriptor's data buffer */
    uint16_t length;     /* Length of data DMAed into data buffer */
    uint16_t csum;       /* Packet checksum */
    uint8_t status;      /* Descriptor status */
    uint8_t errors;      /* Descriptor Errors */
    uint16_t special;
};

/* Receive Descriptor Bit Definitions */
/* Receive Control */
#define E1000_RCTL_EN             0x00000002
#define E1000_RCTL_BAM            0x00008000    /* broadcast enable */
#define E1000_RCTL_BSIZE          0x00000000
#define E1000_RCTL_SECRC          0x04000000    /* Strip Ethernet CRC */



/* Receive Descriptor bit definitions */
#define E1000_RXD_STAT_DD       0x01    /* Descriptor Done */
#define E1000_RXD_STAT_EOP      0x02    /* End of Packet */
#define E1000_RXD_STAT_IXSM     0x04    /* Ignore checksum */
#define E1000_RXD_STAT_VP       0x08    /* IEEE VLAN Packet */
#define E1000_RXD_STAT_UDPCS    0x10    /* UDP xsum caculated */
#define E1000_RXD_STAT_TCPCS    0x20    /* TCP xsum calculated */
#define E1000_RXD_STAT_IPCS     0x40    /* IP xsum calculated */
#define E1000_RXD_STAT_PIF      0x80    /* passed in-exact filter */
#define E1000_RXD_STAT_IPIDV    0x200   /* IP identification valid */
#define E1000_RXD_STAT_UDPV     0x400   /* Valid UDP checksum */
#define E1000_RXD_STAT_ACK      0x8000  /* ACK Packet indication */
#define E1000_RXD_ERR_CE        0x01    /* CRC Error */
#define E1000_RXD_ERR_SE        0x02    /* Symbol Error */
#define E1000_RXD_ERR_SEQ       0x04    /* Sequence Error */
#define E1000_RXD_ERR_CXE       0x10    /* Carrier Extension Error */
#define E1000_RXD_ERR_TCPE      0x20    /* TCP/UDP Checksum Error */
#define E1000_RXD_ERR_IPE       0x40    /* IP Checksum Error */
#define E1000_RXD_ERR_RXE       0x80    /* Rx Data Error */



/* EEPROM.EERD Bit Definitions */
#define E1000_EERD_START        0x00000001
#define E1000_EERD_DONE         0x00000010
#define E1000_EERD_MAC_WD0      0x00000000
#define E1000_EERD_MAC_WD1      0x00000100
#define E1000_EERD_MAC_WD2      0x00000200
#define E1000_EERD_DATA_SHIFT   0x10

/* Ethernet Specifications */
#define E1000_ETH_MAC_HIGH      (uint32_t)0x80005634
#define E1000_ETH_MAC_LOW       (uint32_t)0x12005452
#define E1000_ETH_PACKET_LEN    1518

typedef uint8_t packet_t[2048];

/* Register Set. (82543, 82544)
 *
 * Registers are defined to be 32 bits and  should be accessed as 32 bit values.
 * These registers are physically located on the NIC, but are mapped into the
 * host memory address space.
 *
 * RW - register is both readable and writable
 * RO - register is read only
 * WO - register is write only
 * R/clr - register is read only and is cleared when read
 * A - register array
 */

 #define E1000_EERD     0x00014  /* EEPROM Read - RW */


#define E1000_RCTL     0x00100  /* RX Control - RW */
#define E1000_TCTL     0x00400  /* TX Control - RW */
#define E1000_TIPG     0x00410  /* TX Inter-packet gap -RW */
#define E1000_RDBAL    0x02800  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    0x02804  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN    0x02808  /* RX Descriptor Length - RW */
#define E1000_RDH      0x02810  /* RX Descriptor Head - RW */
#define E1000_RDT      0x02818  /* RX Descriptor Tail - RW */
#define E1000_RDTR     0x02820  /* RX Delay Timer - RW */
#define E1000_TDBAL    0x03800  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804
#define E1000_TDLEN    0x03808  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818  /* TX Descripotr Tail - RW */
#define E1000_MTA      0x05200  /* Multicast Table Array - RW Array */
#define E1000_MTA_LEN  0x200    /* Length of the MTA */
#define E1000_RAL      0x05400  /* Receive Address - RW Array */
#define E1000_RAH      0x05404

/* EEPROM.EERD Bit Definitions */
#define E1000_EERD_START        0x00000001
#define E1000_EERD_DONE         0x00000010
#define E1000_EERD_MAC_WD0      0x00000000
#define E1000_EERD_MAC_WD1      0x00000100
#define E1000_EERD_MAC_WD2      0x00000200
#define E1000_EERD_DATA_SHIFT   0x10

/* Ethernet Specifications */
#define E1000_ETH_MAC_HIGH      (uint32_t)0x80005634
#define E1000_ETH_MAC_LOW       (uint32_t)0x12005452
#define E1000_ETH_PACKET_LEN    1518


#define E1000_RXT0	0x00000080
#define E1000_RCTL_LBM_NO       0xffffff3f /* no loopback mode, 6 & 7 bit set to 0 */
#define E1000_RSRPD    0x02C00  /* RX Small Packet Detect - RW */
#define E1000_ICR_SRPD          0x00010000
#define E1000_IMS_SRPD      E1000_ICR_SRPD
#define E1000_ICR_RXO           0x00000040 /* rx overrun */
#define E1000_IMS_RXO       E1000_ICR_RXO       /* rx overrun */
#define E1000_ICR_RXSEQ         0x00000008 /* rx sequence error */
#define E1000_IMS_RXSEQ     E1000_ICR_RXSEQ     /* rx sequence error */
#define E1000_ICR_RXT0          0x00000080 /* rx timer intr (ring 0) */
#define E1000_IMS_RXT0      E1000_ICR_RXT0      /* rx timer intr */
#define E1000_ICR_TXQE          0x00000002 /* Transmit Queue empty */
#define E1000_IMS_TXQE      E1000_ICR_TXQE      /* Transmit Queue empty */

#define E1000_IMS      0x000D0  /* Interrupt Mask Set - RW */
#define E1000_ICR      0x000C0  /* Interrupt Cause Read - R/clr */
#define E1000_ITR      0x000C4  /* Interrupt Throttling Rate - RW */
#define E1000_ICS      0x000C8  /* Interrupt Cause Set - WO */
#define HEAD_SIZE 4

#endif	// JOS_KERN_E1000_H
