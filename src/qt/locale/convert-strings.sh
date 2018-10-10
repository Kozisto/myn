#!/bin/bash

sed -i "s/\bbitcoin\b/mynt/g" *.ts
sed -i "s/\bBitcoin\b/Mynt/g" *.ts
sed -i "s/\bBITCOINS\b/MYNTS/g" *.ts
sed -i "s/\bbitcoins\b/mynts/g" *.ts
sed -i "s/\bBitcoins/Mynts/g" *.ts

sed -i "s/ bitcoin/ mynt/g" *.ts
sed -i "s/ Bitcoin/ Mynt/g" *.ts
sed -i "s/ BITCOIN/ MYNT/g" *.ts
