### `pgb.rom_poke(addr, val)`
sets the value at the given rom address to the given value

### `pgb.rom_peek(addr)`
returns the value at the given rom address

### `pgb.get_crank()`
returns crank angle in degrees, or null if docked

### `pgb.setCrankSoundsDisabled(bool)`
see `playdate->system->setCrankSoundsDisabled`
    
### `pgb.setROMBreakpoint(addr, fn)`
Inserts a "hardware" execution breakpoint at the given address. Returns the breakpoint index (or null if an error occurred).

`fn(n)` will be invoked every time the instruction at this address runs, where `n` is the breakpoint index.

This is implemented by setting a special opcode, normally considered invalid, to the given address. If this address is modified afterward (e.g. by `pgb.rom_poke`), the breakpoint will no longer trigger.

A maximum of 128 breakpoints may be placed.