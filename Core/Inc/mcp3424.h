//
// Created by kikuchi on 2023/06/23.
//

#ifndef DRILL03_MCP3424_H
#define DRILL03_MCP3424_H

#define MCP3424_PT100_ADDR 0x68
#define MCP3424_LVDT_ADDR 0x6A
#define MCP3424_BAT_ADDR 0x6C

int MCP3424_Read(uint8_t n_dev, uint8_t ch, uint16_t *data);
void MCP3424_dump();

#endif //DRILL03_MCP3424_H
