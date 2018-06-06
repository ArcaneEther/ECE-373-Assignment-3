/*
  ECE 373
  Instructors PJ Waskiewicz Jr. and Shannon Nelson
  Higgins, Jeremy
  Assignment #2 - Simple Reader
  
  The purpose of this program is to open the module created by
  "char_device.c" and print the value of "syscall_val" found within.
*/



/* Include Required Libraries. */
#include <sys/types.h>   /* Required for open() System Call. */
#include <sys/stat.h>    /* Required for open() System Call. */
#include <fcntl.h>       /* Required for open() System Call. */
#include <unistd.h>      /* Required for close(), read(), and write() System Calls. */
#include <stdio.h>       /* Required for printf() Function. */
#include <stdint.h>      /* Required for u32 datatype. */

/* Main function. */
int main(int argc, char *argv[]){
  /* LED On/Off values. */
  uint32_t led0_mask = 0xFFFFFFF0;
  uint32_t led0_on = 0xE;
  uint32_t led0_off = 0xF;
  
  /* File Descriptor for "/dev/pci_device" module. */
  int file_descriptor = 0;
  
  /* u32 Buffer for "led_status" u32. */
  uint32_t buffer;
  
  /* Open "/dev/char/pci_device" module. */
  file_descriptor = open("/dev/pci_device", O_RDWR);
  
  /* Open failed. */
  if(file_descriptor < 0){
    printf("Could not open pci_device.\n");
    perror("Error");
    return(-1);
  }
  
  /* Read the current led_status. */
  if(read(file_descriptor, &buffer, sizeof(uint32_t)) < 0){
    printf("Could not read pci_device.\n");
    perror("Error");
    return(-1);
  }
  
  /* Print the current led_status. */
  printf("Initial LED Status: %x.\n", buffer);
  
  /* Modify buffer to new value to turn LED0 on. */
  buffer = (buffer & led0_mask) | led0_on;
  
  /* Write new value to turn LED0 on. */
  if(write(file_descriptor, &buffer, sizeof(uint32_t)) < 0){
    printf("Could not write to pci_device.\n");
    perror("Error");
    return(-1);
  }

  /* Close and reopen the opened "/dev/pci_device" module. */
  close(file_descriptor);
  file_descriptor = open("/dev/pci_device", O_RDWR);
    
  /* Read the modified led_status. */
  if(read(file_descriptor, &buffer, sizeof(uint32_t)) < 0){
    printf("Could not read pci_device.\n");
    perror("Error");
    return(-1);
  }
  
  /* Print the current led_status. */
  printf("Modified LED Status: %x.\n", buffer);
  
  /* Sleep for two (2) seconds. */
  sleep(2);
  
  /* Modify buffer to new value to turn LED0 off. */
  buffer = (buffer & led0_mask) | led0_off;
  
  /* Write new value to turn LED0 off. */
  if(write(file_descriptor, &buffer, sizeof(uint32_t)) < 0){
    printf("Could not write to pci_device.\n");
    perror("Error");
    return(-1);
  }
  
  /* Close the opened "/dev/pci_device" module. */
  close(file_descriptor);
  
  /* Exit Program. */
  return(0);
}
