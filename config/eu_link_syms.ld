/* Symbols the EU split names but no EU source file defines. */
/* __main walks a constructor table at the start of main.c's rodata (0x80016480 in the EU exe;
   in the US exe the same load names the _card_info stub). The table is empty, and the address
   coincides with the first string of main.c. */
introAnimName = 0x80016480;
