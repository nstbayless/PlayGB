if true then
    return
end

local function poke_verify(bank, addr, prev, val)
    addr = bank*0x4000 | (addr % 0x4000)
    if pgb.rom_peek(addr) ~= prev then
        error("SCRIPT ERROR -- is this the right ROM? Poke_verify failed at " .. string.format("0x%04X", addr) .. " expected " .. string.format("0x%02X", prev) .. " got " .. string.format("0x%02X", pgb.rom_peek(addr)))
    end
    
    pgb.rom_poke(addr, val)
end

local function code_replacement(bank, addr, tprev, tval, unsafe)
    -- verify that the lengths match
    if #tprev ~= #tval then
        error("SCRIPT ERROR -- tprev and tval must have the same length")
    end
    
    -- verify tprev matches what's in ROM
    local base_addr = bank * 0x4000 | (addr % 0x4000)
    for i = 1, #tprev do
        local current_addr = base_addr + i - 1
        local current_byte = pgb.rom_peek(current_addr)
        if current_byte ~= tprev[i] then
            error(string.format("SCRIPT ERROR -- is this the right ROM? Poke_verify failed at 0x%04X expected 0x%02X got 0x%02X", 
                current_addr, tprev[i], current_byte))
        end
    end
    
    local replacement = {
        bank = bank,
        addr = base_addr,
        unsafe = unsafe,
        tprev = tprev,
        tval = tval,
        length = #tprev,
        applied = false,
    }
    
    function replacement:apply(yes)
        if apply == null then
            apply = true
        end
        
        if self.applied == apply then
            return
        end
    
        local target = (yes == false)
            and self.tprev
            or self.tval
        
        if not self.unsafe then
            -- wait until PC is outside the replacement area
            while pgb.regs.pc >= self.addr and pgb.regs.pc < self.addr + self.length do
                pgb.step_cpu()
            end
        end
        
        -- Apply the changes
        for i = 1, self.length do
            pgb.rom_poke(self.addr + i - 1, target[i])
        end
    end
    
    return replacement
end

-- ram addr
-- y speed - d078
-- input - ff8b

local crank_angle = null;
local crank_delta = 0
local crank_delta_smooth = 0
local crank_hyst = null

local CRANK_DELTA_SMOOTH_FACTOR = 0.9
local MIN_RATE_CRANK_BEGIN_FLAP = 0.5
local MIN_RATE_CRANK_FLAP = 0.3
local MIN_HYST_CRANK_BEGIN_FLAP = 9
local CRANK_MAX_HYST = 10

-- no pausing
poke_verify(0, 0x22C, 0xCB, 0xAF)
poke_verify(0, 0x22D, 0x5F, 0xAF)

-- Configuration mode with down+'B'
poke_verify(6, 0x4083, 0x86, 0x82)

-- Extra game mode with up+'A'
poke_verify(6, 0x4088, 0x45, 0x41)

-- can start game with 'A'
poke_verify(6, 0x4096, 0xE6, 0xFE)
poke_verify(6, 0x4097, 0x08, 0x01)
poke_verify(6, 0x4098, 0x28, 0x20)

local fly_thrust = null;
local fly_max_speed = null;

-- set accel directly (tval[2] is accel value)
patch_fly_accel_down = code_replacement(
    0, 0x3C5, {0xFA, 0x7E, 0xD0}, {0x3E, 0, 0x00}
)

-- set accel directly (tval[2] is accel value)
patch_fly_accel_up = code_replacement(
    0, 0x3F8, {0xFA, 0x7E, 0xD0}, {0x3E, 0, 0x00}
)

-- continue flying via crank
pgb.rom_set_breakpoint(
    (1*0x4000) | (0x467E % 0x4000),
    function(n)
        -- TODO: if holding up/down, ignore crank
        
        if crank_delta_smooth > MIN_RATE_CRANK_FLAP then
            local rate = math.max(0, math.min(crank_delta_smooth, 30)) / 30
            fly_thrust = math.floor(-0x20 + 0x70 * rate);
            fly_max_speed = -0x200*rate
            local current_speed = (pgb.ram_peek(0xD078) << 8) | pgb.ram_peek(0xD079)
            if current_speed >= 0x8000 then
                current_speed = current_speed - 0x10000
            end
            if current_speed < fly_max_speed then
                fly_thrust = -0x20
            end
            --print(current_speed, fly_max_speed, fly_thrust, rate)
            if fly_thrust == 0 then
                patch_fly_accel_up.tval[2] = 0
                patch_fly_accel_up.applied = false
                patch_fly_accel_up:apply()
                patch_fly_accel_down.tval[2] = 0
                patch_fly_accel_down.applied = false
                patch_fly_accel_down:apply()
            elseif fly_thrust >= 0 then
                fly_thrust = math.floor((fly_thrust / 0x50) * (fly_thrust / 0x50) * 0x50)
                pgb.regs.a = pgb.regs.a | 0x40
                
                patch_fly_accel_up.tval[2] = math.max(fly_thrust, 0)
                patch_fly_accel_up.applied = false
                patch_fly_accel_up:apply()
            else
                patch_fly_accel_down.tval[2] = math.max(-fly_thrust, 0)
                patch_fly_accel_down.applied = false
                patch_fly_accel_down:apply()
            end
        else
            fly_thrust = null;
        end
    end
)

-- start flying via crank
pgb.rom_set_breakpoint(
    (1*0x4000) | (0x4494 % 0x4000),
    function(n)
        if crank_angle and crank_hyst then
            if circle_difference(crank_hyst, crank_angle) >= MIN_HYST_CRANK_BEGIN_FLAP then
                if crank_delta > MIN_RATE_CRANK_BEGIN_FLAP then
                    pgb.regs.a = pgb.regs.a | 0x40
                end
            end
        end
    end
)

-- can also start the game with 'start'
pgb.rom_set_breakpoint(
    (6*0x4000) | (0x4096 % 0x4000),
    function(n)
        if pgb.regs.a == 0x08 then
            pgb.regs.a = 1
        end
    end
)

-- force immediate unpause
pgb.rom_set_breakpoint(
    (6*0x4000) | (0x460E % 0x4000),
    function(n)
        pgb.regs.a = 0x8
    end
)

function circle_difference(a, b)
    return ((b - a + 180) % 360) - 180
end

function pgb.update()
    local new_crank_angle = pgb.get_crank()
    if new_crank_angle and crank_angle then
        crank_delta = circle_difference(crank_angle, new_crank_angle)
        if not crank_hyst then
            crank_hyst = new_crank_angle
        else
            local cd = circle_difference(crank_hyst, new_crank_angle)
            if cd > CRANK_MAX_HYST then
                crank_hyst = (new_crank_angle - CRANK_MAX_HYST) % 360
            elseif cd < -CRANK_MAX_HYST then
                crank_hyst = (new_crank_angle + CRANK_MAX_HYST) % 360
            end
        end
        
        crank_delta_smooth = crank_delta_smooth * CRANK_DELTA_SMOOTH_FACTOR
            + (1 - CRANK_DELTA_SMOOTH_FACTOR)* crank_delta
    else
        crank_delta = 0
        crank_hyst = new_crank_angle
    end
    
    crank_angle = new_crank_angle
    
    patch_fly_accel_down:apply(false)
    patch_fly_accel_up:apply(false)
end

print("Hello from Lua!")