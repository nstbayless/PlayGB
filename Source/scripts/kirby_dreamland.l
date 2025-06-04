local function poke_verify(bank, addr, prev, val)
    addr = bank*0x4000 | (addr % 0x4000)
    if pgb.rom_peek(addr) ~= prev then
        error("SCRIPT ERROR -- is this the right ROM? Poke_verify failed at " .. string.format("0x%04X", addr) .. " expected " .. string.format("0x%02X", prev) .. " got " .. string.format("0x%02X", pgb.rom_peek(addr)))
    end
    
    pgb.rom_poke(addr, val)
end

-- Configuration mode with down+'B'
poke_verify(6, 0x4083, 0x86, 0x82)

-- Extra game mode with up+'A'
poke_verify(6, 0x4088, 0x45, 0x41)

-- can start game with 'A'
poke_verify(6, 0x4096, 0xE6, 0xFE)
poke_verify(6, 0x4097, 0x08, 0x01)
poke_verify(6, 0x4098, 0x28, 0x20)

function pgb.update()
    
end