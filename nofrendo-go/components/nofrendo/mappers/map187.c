/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.  To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
**
** map187.c
**
** mapper 4 interface
** $Id: map004.c,v 1.2 2001/04/27 14:37:11 neil Exp $
*/

#include <nofrendo.h>
#include <nes_mmc.h>

static struct
{
   uint8 counter;
   uint8 latch;
   uint8 enabled;
} irq;

static uint8 reg;
static uint8 regs[8];
static uint8 reg8000;
static uint16 vrombase;

static const uint8 vlu187[4] = { 0x83, 0x83, 0x42, 0x00 };

// Shouldn't that be packed? (It wasn't packed in SNSS...)
typedef struct
{
   unsigned char irqCounter;
   unsigned char irqLatchCounter;
   unsigned char irqCounterEnabled;
   unsigned char last8000Write;
} mapper187Data;

/* mapper 187: MMC3 */

static void map187_write_reg0(uint32 address, uint8 value)
{
   regs[0] = value;
}


static void map187_write(uint32 address, uint8 value)
{
   switch (address & 0xE001)
   {
   case 0x8000:
      reg8000 = value;
      vrombase = (value & 0x80) ? 0x1000 : 0x0000;
      mmc_bankrom(8, (value & 0x40) ? 0x8000 : 0xC000, -2);
      break;

   case 0x8001:
      switch (reg8000 & 0x07)
      {
      case 0:
         value &= 0xFE;
         mmc_bankvrom(1, vrombase ^ 0x0000, value);
         mmc_bankvrom(1, vrombase ^ 0x0400, value + 1);
         break;

      case 1:
         value &= 0xFE;
         mmc_bankvrom(1, vrombase ^ 0x0800, value);
         mmc_bankvrom(1, vrombase ^ 0x0C00, value + 1);
         break;

      case 2:
         mmc_bankvrom(1, vrombase ^ 0x1000, value);
         break;

      case 3:
         mmc_bankvrom(1, vrombase ^ 0x1400, value);
         break;

      case 4:
         mmc_bankvrom(1, vrombase ^ 0x1800, value);
         break;

      case 5:
         mmc_bankvrom(1, vrombase ^ 0x1C00, value);
         break;

      case 6:
         mmc_bankrom(8, (reg8000 & 0x40) ? 0xC000 : 0x8000, value);
         break;

      case 7:
         mmc_bankrom(8, 0xA000, value);
         break;
      }
      break;

   case 0xA000:
      /* four screen mirroring crap */
      if (0 == (mmc_getinfo()->flags & ROM_FLAG_FOURSCREEN))
      {
         if (value & 1)
            ppu_setmirroring(PPU_MIRROR_HORI);
         else
            ppu_setmirroring(PPU_MIRROR_VERT);
      }
      break;

   case 0xA001:
      /* Save RAM enable / disable */
      /* Messes up Startropics I/II if implemented -- bah */
      break;

   case 0xC000:
      irq.latch = value - 1;
      break;

   case 0xC001:
      irq.counter = 0; // Trigger reload
      break;

   case 0xE000:
      irq.enabled = false;
      break;

   case 0xE001:
      irq.enabled = true;
      break;

   default:
      MESSAGE_DEBUG("map004: unhandled write: address=%p, value=0x%x\n", (void*)address, value);
      break;
   }
}

static void map187_hblank(int scanline)
{
   if (scanline < 241 && ppu_enabled())
   {
      if (irq.counter == 0)
      {
         irq.counter = irq.latch;
      }
      else
      {
         irq.counter--;
      }

      if (irq.enabled && irq.counter == 0)
      {
         nes6502_irq();
      }
   }
}

static void map187_getstate(void *state)
{
   ((mapper187Data*)state)->irqCounter = irq.counter;
   ((mapper187Data*)state)->irqLatchCounter = irq.latch;
   ((mapper187Data*)state)->irqCounterEnabled = irq.enabled;
   ((mapper187Data*)state)->last8000Write = reg8000;
}

static void map187_setstate(void *state)
{
   irq.counter = ((mapper187Data*)state)->irqCounter;
   irq.latch = ((mapper187Data*)state)->irqLatchCounter;
   irq.enabled = ((mapper187Data*)state)->irqCounterEnabled;
   map187_write(0x8000, ((mapper187Data*)state)->last8000Write);
}

static void map187_init(void)
{
   irq.counter = irq.latch = 0;
   irq.enabled = false;
   reg = reg8000 = vrombase = 0;
}

static mem_write_handler_t map187_memwrite[] =
{
   { 0x5000, 0x5000, map187_write_reg0 },
   { 0x6000, 0x6000, map187_write_reg0 },
   { 0x8000, 0xFFFF, map187_write },
   LAST_MEMORY_HANDLER
};

static uint8 map187_read(uint32 address)
{
	// if ((address < 0x5000) || (address > 0x5FFF)) {
	// 	return (openbus);
	// }

	return (vlu187[regs[0] & 0x03]);
}

static mem_read_handler_t map187_memread [] =
{
    { 0x4020, 0x5FFF, map187_read },
    LAST_MEMORY_HANDLER
};

mapintf_t map187_intf =
{
   187, /* mapper number */
   "MMC3 Kasheng", /* mapper name */
   map187_init, /* init routine */
   NULL, /* vblank callback */
   map187_hblank, /* hblank callback */
   map187_getstate, /* get state (snss) */
   map187_setstate, /* set state (snss) */
   map187_memread, /* memory read structure */
   map187_memwrite, /* memory write structure */
   NULL /* external sound device */
};
