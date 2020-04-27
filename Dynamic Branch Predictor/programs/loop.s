.text
main:
  li    $3, 0       # $3 = i = 0
  li    $5, 0       # $5 = sum = 0
  L1:
  add   $5, $5, $3  # sum += i
  addi  $3, $3, 1   # i++
  li    $6, 10
  bne   $3, $6, L1  # if(i != 10) goto L1
  b     exit
exit:
  li $2, 10
  syscall

.data

