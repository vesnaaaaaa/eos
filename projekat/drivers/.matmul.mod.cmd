cmd_/root/docs/eos/projekat/drivers/matmul.mod := printf '%s\n'   matmul.o | awk '!x[$$0]++ { print("/root/docs/eos/projekat/drivers/"$$0) }' > /root/docs/eos/projekat/drivers/matmul.mod
