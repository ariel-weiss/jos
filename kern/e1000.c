#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>

// LAB 6: Your driver code here

volatile void *e1000addr;
struct e1000_tx_desc txd_arr[E1000_TXDARR_LEN] __attribute__((aligned(4096)));
packet_t txd_bufs[E1000_TXDARR_LEN] __attribute__((aligned(4096)));

/* Reset the TXD array entry corresponding to the given
 * index such that it may be resused for another packet.
 */
void _reset_tdr(int index)
{
    txd_arr[index].buffer_addr = (uint32_t)(PADDR(txd_bufs+index));
    txd_arr[index].cso = 0;
    txd_arr[index].cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
    txd_arr[index].status = 0;
    txd_arr[index].css = 0;
    txd_arr[index].special = 0;
}
int attach_E1000(struct pci_func *pcif)
{
    pci_func_enable(pcif);
    e1000addr = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);


    // Initialize MMIO Region
    // Manual section 14.5 initialization
    *(uint32_t *)(e1000addr+E1000_TDBAL) = (uint32_t)(PADDR(txd_arr)); // Indicates start of descriptor ring buffer
    *(uint32_t *)(e1000addr+E1000_TDBAH) = 0; // Make sure high bits are set to 0
    *(uint16_t *)(e1000addr+E1000_TDLEN) = (uint16_t)(sizeof(struct e1000_tx_desc)*E1000_TXDARR_LEN); // Indicates length of descriptor ring buffer
    *(uint32_t *)(e1000addr+E1000_TDH) = 0;
    *(uint32_t *)(e1000addr+E1000_TDT) = 0;
    *(uint32_t *)(e1000addr+E1000_TCTL) = (E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_CT | E1000_TCTL_COLD);
    *(uint16_t *)(e1000addr+E1000_TIPG) = (uint16_t)(E1000_TIPG_IPGT | E1000_TIPG_IPGR1 | E1000_TIPG_IPGR2);

    // Initialize CMD bits for transmit descriptors
    int i;
    for ( i = 0; i < E1000_TXDARR_LEN; i++){
        _reset_tdr(i);
        txd_arr[i].status = E1000_TXD_STAT_DD;
    }
    return 0;
}

int E1000_transmit(void * data_addr, uint16_t length)
{

    static uint32_t nextindex = 0;
    struct e1000_tx_desc *nextdesc = (&txd_arr[nextindex]);


    if (!(nextdesc->status & E1000_TXD_STAT_DD))
        return -1; // TODO: Error code; no free descriptors

    if (length > E1000_ETH_PACKET_LEN)
        length = E1000_ETH_PACKET_LEN;

    _reset_tdr(nextindex);  // Reset this TDR
    nextdesc->length = length;
    memcpy((txd_bufs+nextindex), data_addr, length);


    nextindex = (nextindex+1)%E1000_TXDARR_LEN;
    *(uint32_t *)(e1000addr+E1000_TDT) = nextindex;
    
    return 0;
}
