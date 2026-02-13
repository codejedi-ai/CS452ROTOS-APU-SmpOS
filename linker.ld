ENTRY(_start)
SECTIONS {
  . = 0x40000000;
  .text.boot : {
    KEEP(*(.text.boot))
  }
}
