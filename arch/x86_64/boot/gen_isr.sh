#!/bin/bash
echo ".text"
for i in {0..255}; do
    if [[ "$i" =~ ^(8|10|11|12|13|14|17|21)$ ]]; then
        echo "isr_stub_$i:"
        echo "    push \$$i"
        echo "    jmp isr_common"
    else
        echo "isr_stub_$i:"
        echo "    push \$0"
        echo "    push \$$i"
        echo "    jmp isr_common"
    fi
done

echo ".global isr_stub_table"
echo ".data"
echo ".align 8"
echo "isr_stub_table:"
for i in {0..255}; do
    echo "    .quad isr_stub_$i"
done
