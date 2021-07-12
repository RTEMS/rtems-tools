
/*
 * Qemu log file format.
 */

#ifndef QEMU_LOG_H
#define QEMU_LOG_H

#define QEMU_LOG_SECTION_END    "----------------"
#define QEMU_LOG_IN_KEY         "IN: "

/*!
 *   This structure breaks apart the log line information
 *   into the components address, instruction and data.
 */
typedef struct {
  unsigned long address;
  std::string   instruction;
  std::string   data;
}  QEMU_LOG_IN_Block_t;


#endif /* QEMU_LOG_H */
