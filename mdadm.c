#include "mdadm.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int jbod_operation(uint32_t op, uint8_t *blockID);
static bool is_mounted = false;

uint32_t createOpCode(uint32_t command, uint32_t diskID, uint32_t reserved,
                      uint32_t blockID) {
  uint32_t opCode = 0;
  command = command << 26;
  diskID = diskID << 22;
  reserved = reserved << 8;
  opCode = opCode | command | diskID | reserved | blockID;
  return opCode;
}

int mdadm_mount(void) {
  if (is_mounted) {
    return -1;
  }
  uint32_t op = createOpCode(JBOD_MOUNT, 0, 0, 0);
  jbod_operation(op, NULL);
  is_mounted = true;
  return 1;
}

int mdadm_unmount(void) {
  if (!is_mounted) {
    return -1;
  }
  uint32_t op = createOpCode(JBOD_UNMOUNT, 0, 0, 0);
  jbod_operation(op, NULL);
  is_mounted = false;
  return 1;
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) {
  printf("Debug: Starting read operation\n");
  printf("Debug: Initial addr = %u, len = %u\n", addr, len);

  if (!is_mounted) {
    printf("Debug: System is not mounted\n");
    return -1;
  }

  if (addr + len > JBOD_NUM_DISKS * JBOD_DISK_SIZE ||
      (buf == NULL && len != 0)) {
    printf("Debug: Invalid parameters (out-of-bound address or NULL buffer "
           "with non-zero length)\n");
    return -1;
  }

  if (len > 1024) {
    printf("Debug: I/O size larger than 1024 bytes\n");
    return -1;
  }

  if (len == 0) {
    printf("Debug: 0-length read\n");
    return 0;
  }

  uint32_t total_bytes_read = 0;

  while (len > 0) {
    uint32_t disk = addr / JBOD_DISK_SIZE;
    uint32_t block = (addr % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE;
    uint32_t offset = addr % JBOD_BLOCK_SIZE;
    uint32_t bytes_to_read = JBOD_BLOCK_SIZE - offset;

    if (bytes_to_read > len) {
      bytes_to_read = len;
    }

    printf("Debug: Reading from disk = %u, block = %u, offset = %u\n", disk,
           block, offset);

    uint32_t op_seek_disk = createOpCode(JBOD_SEEK_TO_DISK, disk, 0, 0);
    uint32_t op_seek_block = createOpCode(JBOD_SEEK_TO_BLOCK, 0, 0, block);
    int seek_disk_result = jbod_operation(op_seek_disk, NULL);
    int seek_block_result = jbod_operation(op_seek_block, NULL);

    if (seek_disk_result != 0 || seek_block_result != 0) {
      printf("Debug: Seek operation failed\n");
      return -1;
    }

    printf("Debug: seek_disk_result = %d, seek_block_result = %d\n",
           seek_disk_result, seek_block_result);

    uint8_t read_block[JBOD_BLOCK_SIZE];
    uint32_t op_read_block = createOpCode(JBOD_READ_BLOCK, 0, 0, 0);
    int read_block_result = jbod_operation(op_read_block, read_block);

    if (read_block_result != 0) {
      printf("Debug: Read block operation failed\n");
      return -1;
    }

    printf("Debug: read_block_result = %d\n", read_block_result);

    memcpy(buf, read_block + offset, bytes_to_read);

    addr += bytes_to_read;
    buf += bytes_to_read;
    len -= bytes_to_read;
    total_bytes_read += bytes_to_read;

    printf("Debug: Updated addr = %u, len = %u\n", addr, len);
  }

  printf("Debug: Read operation completed successfully\n");
  return total_bytes_read;
}
/*CWD /home/cmpsc311/sp24-lab2-AfroCash */
/*CWD /home/cmpsc311/sp24-lab2-AfroCash */
/*CWD /home/cmpsc311/sp24-lab2-AfroCash */
/*CWD /home/cmpsc311/sp24-lab2-AfroCash */
