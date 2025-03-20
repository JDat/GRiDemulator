struct {
    const char  *name,
    uint16_t    opMask,
    uint16_t    opValue,
    uint16_t    mnemonic
} translateMatrixEntry;

const struct translateMatrixEntry translateMatrix[54] = {
//Name          opmask                  opValue                 mnemonic
{'F2XM1',       0b1111111111111111,     0b1101100111110000,     0xD9F0},
{'FABS',        0b1111111111111111,     0b1101100111100001,     0xD9E1},


{'FINIT',       0b1111111111111111,     0b1101101111100011,     0xXXXX}

};
