.text
main:
  li    $3, 0       # $3 = i = 0
  li    $5, 0       # $5 = sum = 0
L1:
  li    $4, 0       # $4 = j = 0
L2:
  li    $8, 0
  add   $7, $4, $8
  beqz  $7, S0      # if(j == 0) goto S0
  li    $8, -1
  add   $7, $4, $8
  beqz  $7, S1      # if(j == 1) goto S1
  li    $8, -2
  add   $7, $4, $8
  beqz  $7, S2      # if(j == 2) goto S2
  li    $8, -3
  add   $7, $4, $8
  beqz  $7, S3      # if(j == 3) goto S3
  li    $8, -4
  add   $7, $4, $8
  beqz  $7, S4      # if(j == 4) goto S4
  li    $8, -5
  add   $7, $4, $8
  beqz  $7, S5      # if(j == 5) goto S5
  li    $8, -6
  add   $7, $4, $8
  beqz  $7, S6      # if(j == 6) goto S6
  li    $8, -7
  add   $7, $4, $8
  beqz  $7, S7      # if(j == 7) goto S7
S0:
  add   $5, $5, $4  # sum += j
  b     SEND
S1:
  add   $5, $5, $4  # sum += j
  b     SEND
S2:
  add   $5, $5, $4  # sum += j
  b     SEND
S3:
  add   $5, $5, $4  # sum += j
  b     SEND
S4:
  add   $5, $5, $4  # sum += j
  b     SEND
S5:
  add   $5, $5, $4  # sum += j
  b     SEND
S6:
  add   $5, $5, $4  # sum += j
  b     SEND
S7:
  add   $5, $5, $4  # sum += j
  b     SEND
SEND:
  addi  $4, $4, 1   # j++
  li    $6, 16
  bne   $4, $6, L2  # if(j != 16) goto L2
  addi  $3, $3, 1   # i++
  li    $6, 1024
  bne   $3, $6, L1  # if(i != 1024) goto L1
  b     exit
exit:
  li $2, 10
  syscall

.data
