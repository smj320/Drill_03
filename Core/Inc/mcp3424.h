//
// Created by kikuchi on 2023/06/23.
//

#ifndef DRILL03_MCP3424_H
#define DRILL03_MCP3424_H

int MCP3424_Read(uint8_t adr, uint8_t ch, uint16_t *data);
void MCP3424_dump(uint8_t adr, uint8_t ch) ;

#endif //DRILL03_MCP3424_H
