/* Initialize CPU feature data.  Mach AArch64 version.
   This file is part of the GNU C Library.
   Copyright (C) 2017-2024 Free Software Foundation, Inc.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <array_length.h>
#include <cpu-features.h>
#include <elf/dl-hwcaps.h>
#include <dl-tunables-parse.h>
#include <mach/aarch64/mach_aarch64.h>

#define DCZID_DZP_MASK (1 << 4)
#define DCZID_BS_MASK (0xf)

/* The maximal set of permitted tags that the MTE random tag generation
   instruction may use.  We exclude tag 0 because a) we want to reserve
   that for the libc heap structures and b) because it makes it easier
   to see when pointer have been correctly tagged.  */
#define MTE_ALLOWED_TAGS (0xfffe << PR_MTE_TAG_SHIFT)

struct cpu_list
{
  const char *name;
  size_t len;
  uint64_t midr;
};

static const struct cpu_list cpu_list[] =
{
#define CPU_LIST_ENTRY(__str, __num) { __str, sizeof (__str) - 1, __num }
  CPU_LIST_ENTRY ("thunderxt88",    0x430F0A10),
  CPU_LIST_ENTRY ("thunderx2t99",   0x431F0AF0),
  CPU_LIST_ENTRY ("thunderx2t99p1", 0x420F5160),
  CPU_LIST_ENTRY ("ares",           0x411FD0C0),
  CPU_LIST_ENTRY ("emag",           0x503F0001),
  CPU_LIST_ENTRY ("kunpeng920",     0x481FD010),
  CPU_LIST_ENTRY ("a64fx",          0x460F0010),
  CPU_LIST_ENTRY ("generic",        0x0),
};

static uint64_t
get_midr_from_mcpu (const struct tunable_str_t *mcpu)
{
  for (int i = 0; i < array_length (cpu_list); i++)
    if (tunable_strcmp (mcpu, cpu_list[i].name, cpu_list[i].len))
      return cpu_list[i].midr;

  return UINT64_MAX;
}

static inline void
init_cpu_features (struct cpu_features *cpu_features)
{
  error_t err;
  uint32_t hwcaps[HWCAPS_COUNT];
  mach_msg_type_number_t hwcaps_size = HWCAPS_COUNT;
  uint64_t midr, revidr;

  err = __aarch64_get_hwcaps (__mach_host_self (), hwcaps,
			      &hwcaps_size, &midr, &revidr);
  assert_perror (err);

  if (hwcaps_size >= 1)
    GLRO (dl_hwcap) = hwcaps[0];
  if (hwcaps_size >= 2)
    GLRO (dl_hwcap2) = hwcaps[1];

  cpu_features->midr_el1 = midr;

  /* Get the tunable override.  */
  const struct tunable_str_t *mcpu = TUNABLE_GET (glibc, cpu, name,
						  struct tunable_str_t *,
						  NULL);
  if (mcpu != NULL)
    {
      midr = get_midr_from_mcpu (mcpu);
      if (midr != UINT64_MAX)
        cpu_features->midr_el1 = midr;
    }

  /* Check if ZVA is enabled.  */
  unsigned dczid;
  asm volatile ("mrs %0, dczid_el0" : "=r"(dczid));

  if ((dczid & DCZID_DZP_MASK) == 0)
    cpu_features->zva_size = 4 << (dczid & DCZID_BS_MASK);

  /* Check if BTI is supported.  */
  cpu_features->bti = GLRO (dl_hwcap2) & HWCAP2_BTI;

  /* Check if SVE is supported.  */
  cpu_features->sve = GLRO (dl_hwcap) & HWCAP_SVE;

  /* Check if MOPS is supported.  */
  cpu_features->mops = GLRO (dl_hwcap2) & HWCAP2_MOPS;
}
