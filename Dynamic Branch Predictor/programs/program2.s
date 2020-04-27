.text
main:
  li    $3, 0       # $3 = i = 0
  li    $5, 0       # $5 = sum = 0
L1:
  li    $9, 0       # $9 = j = 0
L2:
  li    $4, 0       # $4 = k = 0
L3:
  li    $8, 0
  add   $7, $4, $8
  beqz  $7, S0      # if(k == 0) goto S0
  li    $8, -1
  add   $7, $4, $8
  beqz  $7, S1      # if(k == 1) goto S1
  li    $8, -2
  add   $7, $4, $8
  beqz  $7, S2      # if(k == 2) goto S2
  li    $8, -3
  add   $7, $4, $8
  beqz  $7, S3      # if(k == 3) goto S3
  li    $8, -4
  add   $7, $4, $8
  beqz  $7, S4      # if(k == 4) goto S4
  li    $8, -5
  add   $7, $4, $8
  beqz  $7, S5      # if(k == 5) goto S5
  li    $8, -6
  add   $7, $4, $8
  beqz  $7, S6      # if(k == 6) goto S6
  li    $8, -7
  add   $7, $4, $8
  beqz  $7, S7      # if(k == 7) goto S7
  li    $8, -8
  add   $7, $4, $8
  beqz  $7, S0      # if(k == 8) goto S0
  li    $8, -9
  add   $7, $4, $8
  beqz  $7, S1      # if(k == 9) goto S1
  li    $8, -10
  add   $7, $4, $8
  beqz  $7, S2      # if(k == 10) goto S2
  li    $8, -11
  add   $7, $4, $8
  beqz  $7, S3      # if(k == 11) goto S3
  li    $8, -12
  add   $7, $4, $8
  beqz  $7, S4     # if(k == 12) goto S4
  li    $8, -13
  add   $7, $4, $8
  beqz  $7, S5     # if(k == 13) goto S5
  li    $8, -14
  add   $7, $4, $8
  beqz  $7, S6     # if(k == 14) goto S6
  li    $8, -15
  add   $7, $4, $8
  beqz  $7, S7     # if(k == 15) goto S7
S0:
  add   $5, $5, $4  # sum += k
  b     SEND
S1:
  add   $5, $5, $4  # sum += k
  b     SEND
S2:
  add   $5, $5, $4  # sum += k
  b     SEND
S3:
  add   $5, $5, $4  # sum += k
  b     SEND
S4:
  add   $5, $5, $4  # sum += k
  b     SEND
S5:
  add   $5, $5, $4  # sum += k
  b     SEND
S6:
  add   $5, $5, $4  # sum += k
  b     SEND
S7:
  add   $5, $5, $4  # sum += k
  b     SEND
SEND:
  addi  $4, $4, 1   # k++
  li    $6, 16
  bne   $4, $6, L3  # if(k != 16) goto L3
  addi  $9, $9, 1   # j++
  li    $6, 4
  bne   $9, $6, L2  # if(j != 4) goto L2
  addi  $3, $3, 1   # i++
  li    $6, 1024
  bne   $3, $6, L1  # if(i != 1024) goto L1
  b     exit
exit:
  li $2, 10
  syscall

.data
